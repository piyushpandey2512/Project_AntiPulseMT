#include <vector>
#include <iostream>
#include <cmath>
#include "TFile.h"
#include "TH1.h"
#include "TF1.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLegend.h"

void analyze_contrast() {
    // 1. Define the angles (X-axis)
    std::vector<double> angles = {0.0, 0.25, 0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00};

    // 2. Create Graph
    TGraph* gContrast = new TGraph();
    gContrast->SetTitle("SolidCounter Contrast vs Grating Angle;Rotation Angle [mrad];Contrast (Visibility)");

    int pointIndex = 0;

    std::cout << "--- Starting Contrast Analysis ---" << std::endl;

    for (double angle : angles) {
        // Construct filename. 
        // %g removes trailing zeros (e.g., 0.25 stays 0.25, but 2.00 becomes 2)
        // Check if your files are named "2_mrad.root" or "2.00_mrad.root" and adjust if necessary.
        TString filename = Form("output_angle_%g_mrad.root", angle);
        
        TFile* file = TFile::Open(filename);
        if (!file || file->IsZombie()) {
            std::cout << "[Error] File not found: " << filename << std::endl;
            continue;
        }

        // 3. Get the Histogram (defined in your RunAction.cc)
        TH1D* h1 = (TH1D*)file->Get("MoirePatternY");
        if (!h1) {
            std::cout << "[Error] Histogram 'MoirePatternY' not found in " << filename << std::endl;
            file->Close();
            continue;
        }

        // 4. Define Fit Function: Offset + Amplitude * cos( Frequency * x + Phase )
        // We fit over the central region where the pattern is clearest (-3cm to 3cm)
        TF1* fitFunc = new TF1("moireFit", "[0] + [1]*cos([2]*x + [3])", -3.0, 3.0);

        // --- INTELLIGENT INITIAL GUESSES ---
        // Offset [0]: Average counts per bin
        double meanVal = h1->Integral() / h1->GetNbinsX();
        
        // Amplitude [1]: Half the difference between Max and Min peaks
        // We smooth slightly to avoid picking up single-bin noise as the max
        double maxVal = h1->GetMaximum();
        double ampGuess = (maxVal - meanVal) * 0.8; 

        // Frequency [2]: 2*pi / Pitch
        // Pitch = 100 um = 0.01 cm. k = 628.3 rad/cm.
        // As we rotate, the apparent pitch changes, but 628 is the base.
        double freqGuess = 628.3;

        fitFunc->SetParameters(meanVal, ampGuess, freqGuess, 0.0);
        
        // Constrain Frequency to prevent fit failure (allow +/- 40% variation)
        fitFunc->SetParLimits(2, 400.0, 900.0);
        // Constrain Offset to be positive
        fitFunc->SetParLimits(0, 0.0, maxVal * 2.0);

        // 5. Perform Fit
        // "Q" = Quiet, "N" = Don't draw immediately, "R" = Use Range specified in TF1
        h1->Fit(fitFunc, "Q N R");

        // 6. Calculate Contrast
        // Contrast = Amplitude / Offset
        double offset = fitFunc->GetParameter(0);
        double amplitude = std::abs(fitFunc->GetParameter(1)); // Use Abs because fit amplitude can be negative (phase flip)

        double contrast = 0.0;
        if (offset > 0.001) { // Prevent division by zero
            contrast = amplitude / offset;
        }

        std::cout << "File: " << filename 
                  << " | Angle: " << angle << " mrad" 
                  << " | Contrast: " << contrast 
                  << " (Amp=" << amplitude << ", Off=" << offset << ")" << std::endl;

        gContrast->SetPoint(pointIndex, angle, contrast);
        pointIndex++;

        file->Close();
    }

    // 7. Draw Plot
    TCanvas* c1 = new TCanvas("c1", "Contrast Analysis", 900, 600);
    c1->SetGrid();
    
    // Cosmetics
    gContrast->SetMarkerStyle(21); // Squares
    gContrast->SetMarkerSize(1.5);
    gContrast->SetMarkerColor(kBlue+2);
    gContrast->SetLineColor(kBlue+2);
    gContrast->SetLineWidth(3);
    
    // Set Y-axis range (Contrast is 0 to 1, maybe slightly more due to noise)
    gContrast->GetYaxis()->SetRangeUser(0.0, 1.1);
    
    gContrast->Draw("ALP");

    // Add a legend or text box if needed
    // auto legend = new TLegend(0.7, 0.7, 0.9, 0.9);
    // legend->AddEntry(gContrast, "Simulated Contrast", "lp");
    // legend->Draw();
}