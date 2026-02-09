#include <iostream>
#include <cmath>
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TStyle.h"

void analyze_from_ntuple(TString filename = "output_angle_0.25_mrad.root") {
    
    // 1. Open the file
    TFile* file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cout << "[ERROR] Could not open file: " << filename << std::endl;
        return;
    }
    std::cout << "Reading raw hits from: " << filename << std::endl;

    // 2. Get the Ntuple (Name "CounterHits" from RunAction.cc)
    TTree* tree = (TTree*)file->Get("CounterHits");
    if (!tree) {
        std::cout << "[ERROR] Ntuple 'CounterHits' not found!" << std::endl;
        return;
    }

    // 3. Set up the Reader
    double x_pos;
    tree->SetBranchAddress("x_pos_cm", &x_pos);

    // 4. Create a High-Resolution Histogram (on the fly!)
    // 7000 bins ensures 10 micron resolution
    TH1D* hProfile = new TH1D("hRescue", "Reconstructed Moir#acute{e} Profile;Position X [cm];Counts", 7000, -3.5, 3.5);

    // 5. Fill Histogram from Ntuple
    Long64_t nEntries = tree->GetEntries();
    std::cout << "Processing " << nEntries << " hits..." << std::endl;
    
    for (Long64_t i = 0; i < nEntries; i++) {
        tree->GetEntry(i);
        hProfile->Fill(x_pos);
    }

    // 6. Setup the Fit
    TF1* fitFunc = new TF1("moireFit", "[0] + [1]*cos([2]*x + [3])", -3.0, 3.0);
    
    // Estimates
    double meanVal = hProfile->Integral() / hProfile->GetNbinsX(); 
    double maxVal  = hProfile->GetMaximum();
    double ampGuess = (maxVal - meanVal) * 0.5; 
    
    fitFunc->SetParameters(meanVal, ampGuess, 628.3, 0.0);
    fitFunc->SetParLimits(0, 0.0, maxVal * 2.0); // Offset > 0
    fitFunc->SetParLimits(2, 550.0, 750.0);      // Frequency near 628

    // 7. Draw & Fit
    TCanvas* c1 = new TCanvas("c1", "Ntuple Analysis", 1000, 600);
    c1->SetGrid();
    
    // Rebin purely for visualization (makes it easier to see the sine wave)
    // The fit still uses the high-res underlying data if we didn't rebin, 
    // but rebinning helps the eye. Let's create a clone for drawing.
    TH1D* hDraw = (TH1D*)hProfile->Clone("hDraw");
    hDraw->Rebin(4); // Combine 4 bins (40 microns) for smoother look
    hDraw->SetLineColor(kBlack);
    hDraw->Draw();
    
    // Fit the original (or the clone)
    hDraw->Fit(fitFunc, "R"); 

    // 8. Results
    double offset    = fitFunc->GetParameter(0);
    double amplitude = std::abs(fitFunc->GetParameter(1));
    double contrast  = 0.0;
    
    if (offset > 0) contrast = amplitude / offset;

    std::cout << "\n================================================" << std::endl;
    std::cout << " CONTRAST RESULT" << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << " Hits Analyzed : " << nEntries << std::endl;
    std::cout << " Offset        : " << offset << std::endl;
    std::cout << " Amplitude     : " << amplitude << std::endl;
    std::cout << " CONTRAST      : " << contrast << std::endl;
    std::cout << "================================================" << std::endl;
}