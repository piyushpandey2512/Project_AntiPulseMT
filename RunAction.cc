#include "RunAction.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh" 
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>



MyRunAction::MyRunAction()
    : fOutputFileName("Project_AntiPulse"),
    fPassedG1Counter(0),
    fPassedG2Counter(0),
    fHitCounterCounter(0),
    fAbsorbedG1Counter(0),
    fAbsorbedG2Counter(0)
{
}

MyRunAction::~MyRunAction()
{
    if (fPionFile.is_open()) {
        fPionFile.close();
    }
}

void MyRunAction::BeginOfRunAction(const G4Run*)
{

    fPassedG1Counter = 0;
    fPassedG2Counter = 0;
    fHitCounterCounter = 0;
    fAbsorbedG1Counter = 0;
    fAbsorbedG2Counter = 0;

    // Generate timestamp string
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm *parts = std::localtime(&now_c);

    std::ostringstream ts;
    ts << std::put_time(parts, "%Y%m%d_%H%M%S");
    fTimestamp = ts.str();

    // Open timestamped PionInteractions.dat file
    std::string pionFileName = "PionInteractions_" + fTimestamp + ".dat";
    fPionFile.open(pionFileName, std::ios::out | std::ios::trunc);

    // Open timestamped ROOT file
    G4AnalysisManager *manager = G4AnalysisManager::Instance();
    manager->SetDefaultFileType("root");
    manager->Reset();

    G4String rootFileName = fOutputFileName + "_" + fTimestamp + ".root";
    fCurrentFileName = rootFileName;
    manager->OpenFile(rootFileName);

    // Create ntuples
    manager->CreateNtuple("Output", "Output");
    manager->CreateNtupleIColumn("fEvent");
    manager->CreateNtupleDColumn("fXcor");
    manager->CreateNtupleDColumn("fYcor");
    manager->CreateNtupleDColumn("fZcor");
    manager->CreateNtupleIColumn("fParentID");
    manager->CreateNtupleIColumn("fTrackID");
    manager->CreateNtupleIColumn("fStepID");
    manager->CreateNtupleSColumn("fProcess");
    manager->CreateNtupleDColumn("fTimeL");
    manager->CreateNtupleDColumn("fTimeG");
    manager->CreateNtupleDColumn("fEnergyDeposition");
    manager->CreateNtupleIColumn("fCopyID");
    manager->CreateNtupleSColumn("fParticles");
    manager->FinishNtuple(0);

    manager->CreateNtuple("IntScoring", "IntScoring");
    manager->CreateNtupleDColumn("Energy Deposition");
    manager->FinishNtuple(1);

    manager->CreateNtuple("SourceVertices", "Source Vertex Positions");
    manager->CreateNtupleDColumn("vx");
    manager->CreateNtupleDColumn("vy");
    manager->CreateNtupleDColumn("vz");
    manager->CreateNtupleIColumn("sourceID");
    manager->FinishNtuple(2);

    // Counter Hits
    manager->CreateNtuple("CounterHits", "Hits on the Final Counter Detector");
    manager->CreateNtupleDColumn("x_pos_cm"); // Column 0
    manager->CreateNtupleDColumn("y_pos_cm"); // Column 1
    manager->CreateNtupleDColumn("z_pos_cm"); // Column 2
    manager->CreateNtupleDColumn("edep_MeV"); // Column 3
    manager->FinishNtuple(3); // Finish ntuple with ID 3

    // Create histograms


    // For three moire source positions
    manager->CreateH3("SourceXYZDistribution", "Primary Particle Source Position;X (cm);Y (cm);Z (cm)",
                  50, -12.0, -4.0,    // X-axis: 50 manager->CreateH1("SourceIDHist", "Source ID", 3, 0.5, 3.5); // Histogram for source IDs 1, 2, 3bins from -12 cm to -4 cm
                  50,  0.0,  7.0,     // Y-axis: 50 bins from 0 cm to 7 cm
                  101, -50.5, 50.5);   // Z-axis: 100 bins from -60 cm to 60 cm

    // For two moire source positions (adjusted Z range)
    // manager->CreateH3("SourceXYZDistribution", "Primary Particle Source Position;X (cm);Y (cm);Z (cm)",
    //               50, -12.0, -4.0,    // X-axis: 50 bins from -12 cm to -4 cm
    //               50,  0.0,  7.0,     // Y-axis: 50 bins from 0 cm to 7 cm
    //               101, -62.5, 17.5);   // Z-axis: 100 bins from -62.5 cm to 17.5 cm

    CreateHistogramWithTitles(manager, "ScintillatorHits", "Scintillator Copy Numbers (Copy Number)", 400, 0, 400);
    CreateHistogramWithTitles(manager, "PionEnergyDep", "Energy Deposition by Pions (MeV)", 100, 0, 10);
    // CreateHistogramWithTitles(manager, "AngularDeviation", "Angular Deviation of Pion Tracks (deg)", 100, 0, 20);
    manager->CreateH2("Edep2DByProcess", "Energy Deposition by Process;Process Index;Edep (MeV)",
                    13, 0, 13,    // 13 process types, bins 0-12
                    100, 0, 10);  // 0-10 MeV, 100 bins
    manager->CreateH1("IntraModuleDeviation", "Intra-module angular deviation;Deviation (deg);Counts", 40, 0., 2.0);
    manager->CreateH1("InterModuleDeviation", "Inter-module angular deviation;Deviation (deg);Counts", 40, 0., 2.0);
    manager->CreateH1("SingleScintDeviation", "Angular Deviation in Test Scintillator;Angle (degrees);Counts", 40, 0, 2.0);
    manager->CreateH1("TwoScintB2BDeviation", "Angular Deviation Between B2B Scintillators;Angle (degrees);Counts", 50, 0., 25.);
    manager->CreateH1("SourceIDHist", "Source ID", 3, -0.5, 2.5); // Histogram for source IDs 1, 2, 3
    manager->CreateH1("ZDist", "Source Z Position", 200, -60, 60); 

    // --- ADD HISTOGRAMS FOR GRATING INTERACTION COUNTS ---
    manager->CreateH1("GratingInteractions", "Grating Interactions;Type;Counts",
                      6, 0, 6);
    manager->CreateH1("PrimaryCount", "Number of Primary Particles per Event", 10, 0, 10);    

}

void MyRunAction::EndOfRunAction(const G4Run* run)
{
    G4AnalysisManager *manager = G4AnalysisManager::Instance();
    manager->Write();
    manager->CloseFile();

    if (fPionFile.is_open()) {
        fPionFile.close();
    }

    /*=======================================================================
    --- PRINT THE FINAL REPORT --- */
    G4int nofEvents = run->GetNumberOfEvent();
    if (nofEvents > 0) {
        G4cout << "\n-------------------- Grating Analysis Summary --------------------\n"
               << " Total Events Processed: " << nofEvents << "\n"
               << "--------------------------------------------------------------\n"
               << " Primary particles absorbed by Grating 1 wall: " << fAbsorbedG1Counter << "\n"
               << " Primary particles absorbed by Grating 2 wall: " << fAbsorbedG2Counter << "\n"
               << " --- \n"
               << " Primary particles that passed through Grating 1 opening: " << fPassedG1Counter << "\n"
               << " Primary particles that passed through G1 AND G2 openings: " << fPassedG2Counter << "\n"
               << " Primary particles that passed through G1, G2, AND hit the Counter: " << fHitCounterCounter << "\n"
               << "--------------------------------------------------------------\n"
               << G4endl;
    }
}

void MyRunAction::SetOutputFileName(const G4String& fileName)
{
    fOutputFileName = fileName;
}

std::ofstream& MyRunAction::GetPionFile()
{
    return fPionFile;
}

std::ofstream& MyRunAction::GetPionFile() const
{
    // const_cast is safe here because fPionFile is mutable in practice (file stream)
    return const_cast<std::ofstream&>(fPionFile);
}

void MyRunAction::CreateHistogramWithTitles(G4AnalysisManager* manager, const G4String& name, const G4String& title, G4int bins, G4double min, G4double max)
{
    manager->CreateH1(name, title, bins, min, max);
}