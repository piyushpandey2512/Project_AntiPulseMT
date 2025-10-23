void analyze_contrast() {
    // Angles matching the macro loop (in mrad)
    std::vector<double> angles = {0.0, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0};
    
    TGraph* gContrast = new TGraph();
    gContrast->SetTitle("Moir#acute{e} Pattern Contrast vs Rotation;Rotation Angle [mrad];Contrast");
    
    int point = 0;
    for (double angle : angles) {
        // Construct filename: e.g., "output_angle_0.25_mrad.root"
        // %.2g formats it to remove trailing zeros if needed, or matches the loop string
        TString fname = Form("output_angle_%g_mrad.root", angle);
        
        TFile* f = TFile::Open(fname);
        if (!f || f->IsZombie()) {
            std::cout << "Warning: Could not open " << fname << std::endl;
            continue;
        }
        
        // Get the Y-hit distribution histogram
        // (Ensure "MoirePatternY" matches the name in your RunAction.cc)
        TH1D* h = (TH1D*)f->Get("MoirePatternY"); 
        if (!h) {
            std::cout << "Warning: Histogram not found in " << fname << std::endl;
            f->Close(); 
            continue; 
        }
        
        // Fit Sine Wave: A + B*cos(kx + phi)
        // Range adjusted to -3.5 to 3.5 cm (size of your detector)
        TF1* fit = new TF1("f", "[0] + [1]*cos([2]*x + [3])", -3.5, 3.5);
        
        // Initial Guesses (Crucial for convergence)
        // [0] Offset: Mean bin content
        // [1] Amplitude: Approx half height
        // [2] Frequency: 2*pi / pitch. Pitch is 100um = 0.01cm. So k ~ 628
        //     Note: With rotation, period changes, but 628 is a good start.
        fit->SetParameters(h->GetMean(), h->GetMaximum()/2.0, 628.0, 0.0); 
        
        h->Fit(fit, "Q"); // Q = Quiet mode
        
        double offset = fit->GetParameter(0);
        double amp = fit->GetParameter(1);
        
        // Calculate Contrast: C = Amplitude / Offset
        double contrast = 0.0;
        if (offset > 0) contrast = std::abs(amp) / offset;
        
        gContrast->SetPoint(point++, angle, contrast);
        
        f->Close();
    }
    
    // Plotting styles
    gContrast->SetMarkerStyle(20); // Full circle
    gContrast->SetMarkerColor(kBlue);
    gContrast->SetLineColor(kBlue);
    gContrast->SetLineWidth(2);
    
    TCanvas* c1 = new TCanvas("c1", "Contrast Study", 800, 600);
    gContrast->Draw("ALP"); // A=Axis, L=Line, P=Points
}