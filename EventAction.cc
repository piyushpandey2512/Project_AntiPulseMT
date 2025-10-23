#include "EventAction.hh"
#include "RunAction.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include "HitBuffer.hh"
#include <algorithm>
#include <cmath>
#include "GratingHit.hh"
#include "G4SDManager.hh"

MyEventAction::MyEventAction(MyRunAction* runAction)
: fRunAction(runAction)
{
    fEnergyDeposition = 0;
}

MyEventAction::~MyEventAction()
{}

void MyEventAction::BeginOfEventAction(const G4Event*)
{
    fEnergyDeposition = 0;
    hasFront = false;
    hasBack = false;

        // --- IMPORTANT: Clear all maps at the start of each event ---
    fIntraModuleMomentumMap.clear();
    fInterModuleMomentumMap.clear();
    fSingleScintMomentumMap.clear();
    fB2BFrontMomentumMap.clear();
}

void MyEventAction::EndOfEventAction(const G4Event* event)
{
    auto* manager = G4AnalysisManager::Instance();
    manager->FillNtupleDColumn(1, 0, fEnergyDeposition); // Column 0 in ntuple 1
    manager->AddNtupleRow(1);


    // ---- Print event summary ----
    auto eventID = event->GetEventID();
    auto printModulo = G4RunManager::GetRunManager()->GetPrintProgress();
    if ((printModulo > 0) && (eventID % printModulo == 0)) {
        G4cout << "--> Event " << eventID << " completed. Total energy: "
               << G4BestUnit(fEnergyDeposition, "Energy") << G4endl;
    }
// =======================================================================
    // --- TASK 5: HIERARCHICAL GRATING PARTICLE COUNTING (Your New Logic) ---
    // This is the new code to analyze the grating hits.
    
    G4SDManager* sdManager = G4SDManager::GetSDMpointer();
    G4int hcID = sdManager->GetCollectionID("GratingCounterHitsCollection");
    if (hcID < 0) return; // If the collection doesn't exist, do nothing.

    auto hitsCollection = static_cast<GratingHitsCollection*>(event->GetHCofThisEvent()->GetHC(hcID));
    if (!hitsCollection || hitsCollection->entries() == 0) return; // If no hits, do nothing.

    // --- Analysis Logic ---
    bool hitG1_passed = false;
    bool hitG2_passed = false;
    bool hitC3 = false;

    // --- ADD THESE NEW FLAGS ---
    bool hitG1_absorbed = false;
    bool hitG2_absorbed = false;

    // Loop through all the hits from the primary particle in this event
    for (size_t i = 0; i < hitsCollection->entries(); ++i) {
        GratingHit* hit = (*hitsCollection)[i];
        
        // We only care about pass-through events (positive detector ID)
        if (hit->GetDetectorNb() == 1) hitG1_passed = true;
        if (hit->GetDetectorNb() == 2) hitG2_passed = true;
        if (hit->GetDetectorNb() == 3) hitC3 = true;

        // --- NEW ABSORPTION CHECKS ---
        if (hit->GetDetectorNb() == -1) hitG1_absorbed = true;
        if (hit->GetDetectorNb() == -2) hitG2_absorbed = true;
    }

// 1. Check UPSTREAM Grating (G2, Z = -45) First
    if (hitG2_passed) {
        fRunAction->AddPassedG2(); // Counts particles passing the first filter

        // 2. Then Check DOWNSTREAM Grating (G1, Z = 0)
        if (hitG1_passed) {
            fRunAction->AddPassedG1(); // Counts particles passing both filters

            // 3. Finally Check Counter
            if (hitC3) {
                fRunAction->AddHitCounter();
            }
        }
        else if (hitG1_absorbed) {
            fRunAction->AddAbsorbedG1(); // Passed G2, but died in G1
        }
    }
    else if (hitG2_absorbed) {
        fRunAction->AddAbsorbedG2(); // Died in G2 (never reached G1)
    }
    // --- Histogram Filling ---
    G4int gratingHistId = manager->GetH1Id("GratingInteractions");

    // Fill the histogram based on what happened in the event
    // Bin 0: G1 Absorbed
    // Bin 1: G1 Passed
    // Bin 2: G2 Absorbed
    // Bin 3: G2 Passed
    // Bin 4: C3 Hit
    if (hitG1_absorbed) manager->FillH1(gratingHistId, 0);
    if (hitG1_passed)   manager->FillH1(gratingHistId, 1);
    
    if (hitG2_absorbed) manager->FillH1(gratingHistId, 2);
    if (hitG2_passed)   manager->FillH1(gratingHistId, 3);

    if (hitC3)          manager->FillH1(gratingHistId, 4);
    // Bin 5 is unused for now.
    // ----------------------------------------------------------------------
}

void MyEventAction::SetFrontPoint(const G4ThreeVector& front) {
    fFrontPosition = front;
    hasFront = true;
}

void MyEventAction::SetBackPoint(const G4ThreeVector& back) {
    fBackPosition = back;
    hasBack = true;
}


// --- IMPLEMENTATION OF MODIFIED AND NEW INTRA-MODULE FUNCTIONS ---

void MyEventAction::StoreIntraModuleMomentum(const G4Track* track, G4int copyNo, const G4ThreeVector& momentum)
{
    fIntraModuleMomentumMap[track][copyNo] = momentum;
}

G4ThreeVector MyEventAction::GetIntraModuleMomentum(const G4Track* track, G4int copyNo) const
{
    auto trackIt = fIntraModuleMomentumMap.find(track);
    if (trackIt != fIntraModuleMomentumMap.end()) {
        const auto& copyNoMap = trackIt->second;
        auto copyNoIt = copyNoMap.find(copyNo);
        if (copyNoIt != copyNoMap.end()) {
            return copyNoIt->second;
        }
    }
    return G4ThreeVector(); // Return zero vector if not found
}

void MyEventAction::ClearIntraModuleMomentum(const G4Track* track, G4int copyNo)
{
    auto trackIt = fIntraModuleMomentumMap.find(track);
    if (trackIt != fIntraModuleMomentumMap.end()) {
        trackIt->second.erase(copyNo);
    }
}

// --- CORRECTED IMPLEMENTATION OF INTER-MODULE FUNCTIONS ---

void MyEventAction::StoreInterModuleMomentum(const G4Track* track, const G4ThreeVector& momentum)
{
    // Use the track pointer as the key, not the track ID
    fInterModuleMomentumMap[track] = momentum;
}

G4ThreeVector MyEventAction::GetInterModuleMomentum(const G4Track* track) const
{
    // Use the track pointer to find the entry, not the track ID
    auto it = fInterModuleMomentumMap.find(track);
    if (it != fInterModuleMomentumMap.end()) {
        return it->second;
    }
    return G4ThreeVector(); // Return zero vector if not found
}

// --- IMPLEMENTATION OF SINGLE/B2B SCINTILLATOR FUNCTIONS ---

void MyEventAction::StoreSingleScintMomentum(G4int trackID, const G4ThreeVector& momentum)
{
    fSingleScintMomentumMap[trackID] = momentum;
}

G4ThreeVector MyEventAction::GetSingleScintMomentum(G4int trackID)
{
    auto it = fSingleScintMomentumMap.find(trackID);
    if (it != fSingleScintMomentumMap.end()) {
        return it->second;
    }
    return G4ThreeVector();
}

void MyEventAction::StoreB2BFrontMomentum(G4int trackID, const G4ThreeVector& momentum)
{
    fB2BFrontMomentumMap[trackID] = momentum;
}

G4ThreeVector MyEventAction::GetB2BFrontMomentum(G4int trackID) const
{
    auto it = fB2BFrontMomentumMap.find(trackID);
    if (it != fB2BFrontMomentumMap.end()) {
        return it->second;
    }
    return G4ThreeVector();
}