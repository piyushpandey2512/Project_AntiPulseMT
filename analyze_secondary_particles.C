// analyze_secondary_particles.C
// ROOT macro to analyze and visualize secondary particles from antiproton-silicon collisions

void analyze_secondary_particles(const char* rootFile = "Project_AntiPulse.root") {
    
    // Open the ROOT file
    TFile *f = new TFile(rootFile, "READ");
    if (!f || f->IsZombie()) {
        cerr << "Error: Could not open file " << rootFile << endl;
        return;
    }
    
    // Get histograms
    TH1F *hParticleType = (TH1F*)f->Get("SecondaryParticleType");
    TH1F *hParticleCount = (TH1F*)f->Get("SecondaryParticleCount");
    TH1F *hKineticEnergy = (TH1F*)f->Get("SecondaryParticleKineticEnergy");
    TH1F *hCollisionCount = (TH1F*)f->Get("AntiprotonGratingCollisionCount");
    TH2F *hTypePerEvent = (TH2F*)f->Get("SecondaryParticleTypePerEvent");
    
    // Check if histograms exist
    if (!hParticleType || !hParticleCount || !hKineticEnergy) {
        cerr << "Error: Could not find secondary particle histograms!" << endl;
        cerr << "Make sure you ran the simulation with the updated code." << endl;
        return;
    }
    
    // Print statistics
    cout << "\n========== SECONDARY PARTICLE ANALYSIS ==========" << endl;
    cout << "\nHistogram Statistics:" << endl;
    cout << "  Total Entries (all types): " << hParticleType->GetEntries() << endl;
    cout << "  Total Collision Events: " << hCollisionCount->Integral() << endl;
    
    // Particle type breakdown
    cout << "\nParticle Type Distribution:" << endl;
    cout << "  Gammas (γ):       " << hParticleType->GetBinContent(1) << " (" 
         << (100.0 * hParticleType->GetBinContent(1) / hParticleType->GetEntries()) << "%)" << endl;
    cout << "  π+:               " << hParticleType->GetBinContent(2) << " (" 
         << (100.0 * hParticleType->GetBinContent(2) / hParticleType->GetEntries()) << "%)" << endl;
    cout << "  π-:               " << hParticleType->GetBinContent(3) << " (" 
         << (100.0 * hParticleType->GetBinContent(3) / hParticleType->GetEntries()) << "%)" << endl;
    cout << "  π0:               " << hParticleType->GetBinContent(4) << " (" 
         << (100.0 * hParticleType->GetBinContent(4) / hParticleType->GetEntries()) << "%)" << endl;
    cout << "  Protons:          " << hParticleType->GetBinContent(5) << " (" 
         << (100.0 * hParticleType->GetBinContent(5) / hParticleType->GetEntries()) << "%)" << endl;
    cout << "  Neutrons:         " << hParticleType->GetBinContent(6) << " (" 
         << (100.0 * hParticleType->GetBinContent(6) / hParticleType->GetEntries()) << "%)" << endl;
    cout << "  Other:            " << hParticleType->GetBinContent(7) << " (" 
         << (100.0 * hParticleType->GetBinContent(7) / hParticleType->GetEntries()) << "%)" << endl;
    
    // Particle multiplicity statistics
    cout << "\nSecondary Particle Multiplicity:" << endl;
    cout << "  Mean particles per collision:   " << hParticleCount->GetMean() << endl;
    cout << "  RMS (standard deviation):       " << hParticleCount->GetRMS() << endl;
    cout << "  Max particles in single event:  " << hParticleCount->GetBinCenter(hParticleCount->GetMaximumBin()) << endl;
    
    // Kinetic energy statistics
    cout << "\nSecondary Particle Kinetic Energy:" << endl;
    cout << "  Mean KE:   " << hKineticEnergy->GetMean() << " MeV" << endl;
    cout << "  RMS KE:    " << hKineticEnergy->GetRMS() << " MeV" << endl;
    cout << "  Min KE:    " << hKineticEnergy->GetXaxis()->GetXmin() << " MeV" << endl;
    cout << "  Max KE:    " << hKineticEnergy->GetXaxis()->GetXmax() << " MeV" << endl;
    
    cout << "\n================================================\n" << endl;
    
    // Create visualization
    cout << "Creating visualization..." << endl;
    
    // Canvas 1: Particle types (bar chart)
    TCanvas *c1 = new TCanvas("c1_types", "Particle Type Distribution", 900, 600);
    c1->SetLogy();
    hParticleType->SetStats(0);
    hParticleType->SetTitle("Secondary Particle Types from p-bar Si Collisions");
    hParticleType->GetXaxis()->SetTitle("Particle Type");
    hParticleType->GetYaxis()->SetTitle("Count (log scale)");
    hParticleType->SetFillColor(kBlue);
    hParticleType->SetLineColor(kBlack);
    
    // Set custom labels
    hParticleType->GetXaxis()->SetBinLabel(1, "#gamma");
    hParticleType->GetXaxis()->SetBinLabel(2, "#pi^{+}");
    hParticleType->GetXaxis()->SetBinLabel(3, "#pi^{-}");
    hParticleType->GetXaxis()->SetBinLabel(4, "#pi^{0}");
    hParticleType->GetXaxis()->SetBinLabel(5, "proton");
    hParticleType->GetXaxis()->SetBinLabel(6, "neutron");
    hParticleType->GetXaxis()->SetBinLabel(7, "other");
    
    hParticleType->Draw("bar");
    c1->SaveAs("secondary_particle_types.png");
    c1->SaveAs("secondary_particle_types.pdf");
    
    // Canvas 2: Particle count distribution
    TCanvas *c2 = new TCanvas("c2_count", "Particle Count Distribution", 900, 600);
    hParticleCount->SetStats(1);
    hParticleCount->SetTitle("Number of Secondary Particles per Collision Event");
    hParticleCount->GetXaxis()->SetTitle("Number of Particles");
    hParticleCount->GetYaxis()->SetTitle("Count");
    hParticleCount->SetFillColor(kGreen);
    hParticleCount->SetLineColor(kBlack);
    hParticleCount->Draw();
    c2->SaveAs("secondary_particle_count.png");
    c2->SaveAs("secondary_particle_count.pdf");
    
    // Canvas 3: Kinetic energy distribution
    TCanvas *c3 = new TCanvas("c3_energy", "Kinetic Energy Distribution", 900, 600);
    hKineticEnergy->SetStats(1);
    hKineticEnergy->SetTitle("Kinetic Energy of Secondary Particles");
    hKineticEnergy->GetXaxis()->SetTitle("Kinetic Energy (MeV)");
    hKineticEnergy->GetYaxis()->SetTitle("Count");
    hKineticEnergy->SetFillColor(kRed);
    hKineticEnergy->SetLineColor(kBlack);
    hKineticEnergy->Draw();
    c3->SaveAs("secondary_particle_energy.png");
    c3->SaveAs("secondary_particle_energy.pdf");
    
    // Canvas 4: 2D correlation (if available)
    if (hTypePerEvent) {
        TCanvas *c4 = new TCanvas("c4_2d", "Particle Type per Event", 900, 600);
        hTypePerEvent->SetTitle("Secondary Particle Types vs Event Number");
        hTypePerEvent->GetXaxis()->SetTitle("Event Number (mod 100)");
        hTypePerEvent->GetYaxis()->SetTitle("Particle Type");
        hTypePerEvent->Draw("colz");
        c4->SaveAs("secondary_particle_2d.png");
        c4->SaveAs("secondary_particle_2d.pdf");
    }
    
    // Canvas 5: Combined summary
    TCanvas *c5 = new TCanvas("c5_summary", "Summary", 1200, 800);
    c5->Divide(2, 2);
    
    c5->cd(1);
    hParticleType->Draw("bar");
    
    c5->cd(2);
    hParticleCount->Draw();
    
    c5->cd(3);
    hKineticEnergy->Draw();
    
    c5->cd(4);
    if (hTypePerEvent) {
        hTypePerEvent->Draw("colz");
    } else {
        TText t;
        t.DrawText(0.2, 0.5, "2D histogram not available");
    }
    
    c5->SaveAs("secondary_particles_summary.png");
    c5->SaveAs("secondary_particles_summary.pdf");
    
    cout << "Plots saved:" << endl;
    cout << "  - secondary_particle_types.png/pdf" << endl;
    cout << "  - secondary_particle_count.png/pdf" << endl;
    cout << "  - secondary_particle_energy.png/pdf" << endl;
    cout << "  - secondary_particle_2d.png/pdf" << endl;
    cout << "  - secondary_particles_summary.png/pdf" << endl;
    
    f->Close();
}

// If you want to call it with the latest ROOT file automatically:
void analyze_latest() {
    // Find the most recent ROOT file
    system("ls -t Project_AntiPulse_*.root | head -1 > /tmp/latest_file.txt");
    
    ifstream file("/tmp/latest_file.txt");
    string latest;
    getline(file, latest);
    file.close();
    
    if (latest.empty()) {
        cerr << "No ROOT files found!" << endl;
        return;
    }
    
    cout << "Analyzing: " << latest << endl;
    analyze_secondary_particles(latest.c_str());
}
