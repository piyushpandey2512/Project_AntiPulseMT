#include <vector>
#include <iostream>
#include <cmath>
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLegend.h"

void analyze_contrast() {
    // Angles to analyze
    std::vector<double> angles = {0.0, 0.25, 0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00};
    
    TGraphErrors* gContrast = new TGraphErrors();
    gContrast->SetTitle("Contrast vs. Grating Rotation;Angle [mrad];Contrast");

    int pointIndex = 0;

    std::cout << "--- Starting Analysis for Vertical Fringes ---" << std::endl;

    for (double angle : angles) {
        TString filename = Form("output_angle_%g_mrad.root", angle);
        
        TFile* file = TFile::Open(filename);
        if (!file || file->IsZombie()) {
            std::cout << "Skipping missing file: " << filename << std::endl;
            continue;
        }

        TH1D* h1 = nullptr;
        
        // --- STEP 1: GET 2D DATA & PROJECT ONTO X ---
        TH2D* h2 = (TH2D*)file->Get("MoirePattern"); // Name from your RunAction
        if (h2) {
            // Project onto X-axis because fringes are VERTICAL
            // We sum up the counts in each vertical column
            h1 = h2->ProjectionX(Form("projX_%g", angle));
            std::cout << " -> Created X-projection from 2D plot for " << angle << " mrad." << std::endl;
        }
        else {
            // Fallback: Try looking for the profile if it exists
            h1 = (TH1D*)file->Get("MoireProfile");
        }

        if (!h1 || h1->GetEntries() == 0) {
            std::cout << "[Error] No valid data found in file: " << filename << std::endl;
            file->Close();
            continue;
        }

        // --- STEP 2: FIT THE SINE WAVE ---
        // Range: Central 6 cm (-3 to 3)
        TF1* fitFunc = new TF1("moireFit", "[0] + [1]*cos([2]*x + [3])", -3.0, 3.0);
        
        // Initial Guesses
        double meanVal = h1->Integral() / h1->GetNbinsX();
        double maxVal  = h1->GetMaximum();
        double ampGuess = (maxVal - meanVal) * 0.8; 
        
        // Pitch = 100um = 0.01cm. k = 2*pi/0.01 ~ 628
        fitFunc->SetParameters(meanVal, ampGuess, 628.3, 0.0); 
        fitFunc->SetParLimits(2, 400.0, 800.0); // Limit frequency
        fitFunc->SetParLimits(0, 0.0, maxVal * 2.0); // Offset > 0

        // Fit
        h1->Fit(fitFunc, "Q N R");

        // --- STEP 3: CALCULATE CONTRAST ---
        double offset    = fitFunc->GetParameter(0);
        double amplitude = std::abs(fitFunc->GetParameter(1));
        
        double contrast = 0.0;
        if (offset > 1e-5) contrast = amplitude / offset;

        std::cout << "Angle: " << angle << " mrad | Contrast: " << contrast << std::endl;

        gContrast->SetPoint(pointIndex, angle, contrast);
        gContrast->SetPointError(pointIndex, 0, 0.02); 
        pointIndex++;

        file->Close();
    }

    // --- STEP 4: PLOT ---
    TCanvas* c1 = new TCanvas("c1", "Contrast Curve", 800, 600);
    c1->SetGrid();
    gContrast->SetMarkerStyle(21);
    gContrast->SetMarkerColor(kBlue);
    gContrast->SetLineColor(kBlue);
    gContrast->SetLineWidth(2);
    gContrast->GetYaxis()->SetRangeUser(0, 1.1);
    gContrast->Draw("ALP");
}