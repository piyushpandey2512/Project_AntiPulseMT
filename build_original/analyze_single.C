#include <iostream>
#include <cmath>
#include "TFile.h"
#include "TH1.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TStyle.h"

// Usage: root -l 'analyze_single.C("your_file_name.root")'
void analyze_single(TString filename = "output_angle_0.75_mrad.root") {
    
    // 1. Open the file
    TFile* file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cout << "[ERROR] Could not open file: " << filename << std::endl;
        return;
    }
    std::cout << "Analyzing file: " << filename << std::endl;

    // 2. Get the Profile Histogram (X-Projection)
    // We look for "MoireProfile" based on your latest RunAction.cc
    TH1D* h1 = (TH1D*)file->Get("MoireProfile");
    
    if (!h1) {
        std::cout << "[ERROR] Histogram 'MoireProfile' not found!" << std::endl;
        std::cout << "Available keys in file:" << std::endl;
        file->ls();
        return;
    }

    // 3. Setup the Fit
    // Range: -3.0 cm to 3.0 cm (Central region)
    TF1* fitFunc = new TF1("moireFit", "[0] + [1]*cos([2]*x + [3])", -3.0, 3.0);
    
    // Estimates for the fit parameters
    double meanVal = h1->Integral() / h1->GetNbinsX(); 
    double maxVal  = h1->GetMaximum();
    double ampGuess = (maxVal - meanVal) * 0.5; 
    
    // Pitch is 100um (0.01cm) -> Frequency k = 2*pi/0.01 ~ 628
    fitFunc->SetParameters(meanVal, ampGuess, 628.3, 0.0);
    
    // Constraints (Vital for clean fits)
    fitFunc->SetParLimits(0, 0.0, maxVal * 2.0); // Offset > 0
    fitFunc->SetParLimits(2, 550.0, 750.0);      // Frequency near 628

    // 4. Perform Fit & Draw
    TCanvas* c1 = new TCanvas("c1", "Single Angle Analysis", 1000, 600);
    c1->SetGrid();
    
    // Rebin slightly if visualization is too noisy (optional, affects visual only)
    // h1->Rebin(2); 
    
    h1->SetLineColor(kBlack);
    h1->SetTitle(Form("Moir#acute{e} Profile Analysis (%s);Position X [cm];Counts", filename.Data()));
    h1->Draw();
    
    h1->Fit(fitFunc, "R"); // "R" = Use specified range (-3 to 3)

    // 5. Calculate Contrast
    double offset    = fitFunc->GetParameter(0);
    double amplitude = std::abs(fitFunc->GetParameter(1));
    double contrast  = 0.0;
    
    if (offset > 0) contrast = amplitude / offset;

    std::cout << "------------------------------------------------" << std::endl;
    std::cout << " RESULTS FOR " << filename << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
    std::cout << " Offset    : " << offset << std::endl;
    std::cout << " Amplitude : " << amplitude << std::endl;
    std::cout << " CONTRAST  : " << contrast << std::endl;
    std::cout << "------------------------------------------------" << std::endl;
}