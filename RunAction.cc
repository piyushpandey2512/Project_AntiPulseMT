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

    // Generate timestamp (Keep this for the text file if you want)
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm *parts = std::localtime(&now_c);
    std::ostringstream ts;
    ts << std::put_time(parts, "%Y%m%d_%H%M%S");
    fTimestamp = ts.str();

    // Open timestamped PionInteractions.dat file
    std::string pionFileName = "PionInteractions_" + fTimestamp + ".dat";
    fPionFile.open(pionFileName, std::ios::out | std::ios::trunc);

    // --- ROOT FILE SETUP ---
    G4AnalysisManager *manager = G4AnalysisManager::Instance();
    manager->SetDefaultFileType("root");
    manager->Reset();

    G4String rootFileName = fOutputFileName + "_" + fTimestamp + ".root";
    fCurrentFileName = rootFileName;
    manager->OpenFile(rootFileName);

    // Create ntuples (Keep your existing ntuples exactly as they are)
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

    manager->CreateNtuple("CounterHits", "Hits on the Final Counter Detector");
    manager->CreateNtupleDColumn("x_pos_cm");
    manager->CreateNtupleDColumn("y_pos_cm");
    manager->CreateNtupleDColumn("z_pos_cm");
    manager->CreateNtupleDColumn("edep_MeV");
    manager->FinishNtuple(3);

    // --- HISTOGRAMS ---

    // Keep your Source Distribution
    manager->CreateH3("SourceXYZDistribution", "Primary Particle Source Position",
                  50, -12.0, -4.0,
                  50,  0.0,  7.0,
                  101, -50.5, 50.5);

    CreateHistogramWithTitles(manager, "ScintillatorHits", "Scintillator Copy Numbers", 400, 0, 400);
    CreateHistogramWithTitles(manager, "PionEnergyDep", "Energy Deposition by Pions", 100, 0, 10);

    manager->CreateH2("Edep2DByProcess", "Energy Deposition by Process", 13, 0, 13, 100, 0, 10);
    manager->CreateH1("IntraModuleDeviation", "Intra-module angular deviation", 40, 0., 2.0);
    manager->CreateH1("InterModuleDeviation", "Inter-module angular deviation", 40, 0., 2.0);
    manager->CreateH1("SingleScintDeviation", "Angular Deviation in Test Scintillator", 40, 0, 2.0);
    manager->CreateH1("TwoScintB2BDeviation", "Angular Deviation Between B2B Scintillators", 50, 0., 25.);
    manager->CreateH1("SourceIDHist", "Source ID", 3, -0.5, 2.5);
    manager->CreateH1("ZDist", "Source Z Position", 200, -60, 60); 
    manager->CreateH1("GratingInteractions", "Grating Interactions", 6, 0, 6);
    manager->CreateH1("PrimaryCount", "Number of Primary Particles per Event", 10, 0, 10);    

    // FIX 2 & 3: ADD HIGH-RESOLUTION Y-PROJECTION
    // This is the histogram your analysis script "analyze_contrast.C" is looking for!
    // 7000 bins = 10 micrometer resolution over 7 cm.
    manager->CreateH1("MoirePatternY", "Moiré Pattern (Y-Projection);Y (cm);Counts", 
                      7000, -3.5, 3.5);

    // You can keep these for visual checks, but they are not for contrast analysis
    manager->CreateH2("MoirePattern", "Moiré Pattern (2D)", 200, -3.5, 3.5, 50, -3.5, 3.5);
    // Increase bins from 200 to 7000 to resolve 100um fringes!
    manager->CreateH1("MoireProfile", "Moiré Profile (X-Projection)", 7000, -3.5, 3.5);

    // ===== NEW: Secondary Particle Histograms from Antiproton-Silicon Collisions =====
    // Particle Type codes: 0=gamma, 1=pi+, 2=pi-, 3=pi0, 4=proton, 5=neutron, 6=other
    manager->CreateH1("SecondaryParticleType", "Secondary Particle Types from p-bar Si", 7, -0.5, 6.5);
    manager->CreateH1("SecondaryParticleCount", "Number of Secondary Particles per Collision", 50, 0, 50);
    manager->CreateH2("SecondaryParticleTypePerEvent", "Secondary Particles per Event", 2500, 0, 250000, 7, -0.5, 6.5);
    manager->CreateH1("SecondaryParticleKineticEnergy", "KE of Secondary Particles from p-bar Si", 100, 0, 1000);
    manager->CreateH1("AntiprotonGratingCollisionCount", "Antiproton Collisions in Silicon Gratings", 10, 0, 10);
    // --- NEW: ADD THIS HISTOGRAM ---
    // ID: SecondaryParticleSource
    // X-Axis: 7 Particle Types
    // Y-Axis: 4 Source Locations (0=Error, 1=Grating1, 2=Grating2, 3=Counter)
    manager->CreateH2("SecondaryParticleSource", "Particle Source;Type;Volume ID", 
                      7, -0.5, 6.5, 4, -0.5, 3.5);
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
               << " Primary particles absorbed by Grating 2 wall: " << fAbsorbedG2Counter << "\n"
               << " Primary particles absorbed by Grating 1 wall: " << fAbsorbedG1Counter << "\n"
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