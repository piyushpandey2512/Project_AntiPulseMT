#include <vector>
#include <iostream>
#include <string>
#include <cmath>
#include "TFile.h"
#include "TH1.h"
#include "TF1.h"
#include "TGraphErrors.h"
#include "TMultiGraph.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TAxis.h"

// =========================================================
// HELPER: Calculates Contrast from a single file
// =========================================================
double GetContrast(TString filename, double &error) {
    TFile* file = TFile::Open(filename);
    
    // Check if file exists
    if (!file || file->IsZombie()) {
        std::cout << "   [MISSING] " << filename << " (Skipping point)" << std::endl;
        return -1.0; 
    }

    // Get the Histogram (X-Projection)
    TH1D* h1 = (TH1D*)file->Get("MoireProfile");
    if (!h1) {
        std::cout << "   [ERROR] 'MoireProfile' not found in " << filename << std::endl;
        file->Close();
        return -1.0;
    }

    // Define Fit Function: Offset + Amplitude * cos(freq*x + phase)
    // Range: -3.0 to 3.0 cm (Avoids edges of the detector)
    TF1* fitFunc = new TF1("moireFit", "[0] + [1]*cos([2]*x + [3])", -3.0, 3.0);
    
    // --- Smart Initial Guesses ---
    double meanVal = h1->Integral() / h1->GetNbinsX();
    double maxVal  = h1->GetMaximum();
    double ampGuess = (maxVal - meanVal) * 0.8;
    
    // Pitch 100um = 0.01cm -> Frequency k = 2*pi/0.01 ~ 628.3
    fitFunc->SetParameters(meanVal, ampGuess, 628.3, 0.0);
    fitFunc->SetParLimits(2, 550.0, 750.0); // Constrain frequency
    fitFunc->SetParLimits(0, 0.0, maxVal * 2.0); // Offset must be positive

    // Perform Fit (Q=Quiet, N=NoDraw, R=UseRange)
    h1->Fit(fitFunc, "Q N R");

    // Extract Results
    double offset = fitFunc->GetParameter(0);
    double amp    = std::abs(fitFunc->GetParameter(1));
    double contrast = 0.0;

    if (offset > 1e-5) {
        contrast = amp / offset;
    }

    // Assign a small constant error bar for visualization
    error = 0.005; 

    file->Close();
    return contrast;
}

// =========================================================
// MAIN FUNCTION
// =========================================================
void analyze_comparison() {
    
    // -----------------------------------------------------
    // 1. DEFINITIONS (Matches your simulation)
    // -----------------------------------------------------
    
    // The Misalignment Angles (X-Axis)
    std::vector<double> angles = {0.0, 0.5, 1.0};

    // The Beam Divergences (The different curves)
    std::vector<double> alphas = {0.3, 0.5, 1.0}; 

    // Visual Styling
    int colors[] = {kBlue+1, kRed+1, kGreen+2, kMagenta+1};
    int markers[] = {20, 21, 22, 23}; // Circle, Square, Triangle...

    // -----------------------------------------------------
    // 2. CANVAS SETUP
    // -----------------------------------------------------
    TCanvas* c1 = new TCanvas("c1", "Contrast Analysis", 900, 700);
    c1->SetGrid();
    gStyle->SetOptStat(0);

    TMultiGraph* mg = new TMultiGraph();
    mg->SetTitle("Contrast vs. Misalignment;Misalignment Angle #delta(#beta) [mrad];Contrast (C)");

    // Legend Position (Upper Right)
    TLegend* leg = new TLegend(0.60, 0.70, 0.88, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillColor(kWhite);
    leg->SetHeader("Beam Divergence #alpha");

    // -----------------------------------------------------
    // 3. ANALYSIS LOOP
    // -----------------------------------------------------
    int curveIdx = 0;

    for (double alpha : alphas) {
        std::cout << "\n========================================" << std::endl;
        std::cout << " Processing Curve: Alpha = " << alpha << " mrad" << std::endl;
        std::cout << "========================================" << std::endl;

        TGraphErrors* gr = new TGraphErrors();
        int pointIdx = 0;

        for (double angle : angles) {
            
            // Construct filename (e.g., alpha_0.3_angle_1.5.root)
            // %g removes trailing zeros (1.0 -> 1) which matches your renaming style
            TString filename = Form("alpha_%g_angle_%g.root", alpha, angle);
            
            double err = 0;
            double contrast = GetContrast(filename, err);

            if (contrast >= 0) {
                gr->SetPoint(pointIdx, angle, contrast);
                gr->SetPointError(pointIdx, 0.0, err);
                
                std::cout << "  File: " << filename 
                          << " | Angle: " << angle 
                          << " | Contrast: " << contrast << std::endl;
                pointIdx++;
            }
        }

        // Apply Styles
        gr->SetMarkerStyle(markers[curveIdx % 4]);
        gr->SetMarkerSize(1.3);
        gr->SetMarkerColor(colors[curveIdx % 4]);
        gr->SetLineColor(colors[curveIdx % 4]);
        gr->SetLineWidth(2);
        
        mg->Add(gr);
        leg->AddEntry(gr, Form("#alpha = %g mrad", alpha), "lp");
        
        curveIdx++;
    }
// -----------------------------------------------------
    // 4. DRAW
    // -----------------------------------------------------
    if (mg->GetListOfGraphs()) {
        mg->Draw("ALP");

        // Use SetRangeUser for BOTH axes to force the view
        mg->GetYaxis()->SetRangeUser(0.0, 0.3); // Y-axis: 0 to 0.3
        mg->GetXaxis()->SetRangeUser(0.0, 1.0); // X-axis: 0 to 1.0 (Zoom in)

        leg->Draw();

        // CRITICAL: Force the canvas to refresh so changes apply immediately
        c1->Modified();
        c1->Update();
    } else {
        std::cout << "\n[ERROR] No graphs were created. Check your filenames!" << std::endl;
    }

}