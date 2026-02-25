#include "SteppingAction.hh"
#include "EventAction.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4TouchableHandle.hh"
#include "G4Track.hh"
#include "G4UnitsTable.hh"
#include "G4VPhysicalVolume.hh"
#include "RunAction.hh"
#include <iomanip>

MySteppingAction::MySteppingAction(MyEventAction *eventAction)
    : fEventAction(eventAction) {}

MySteppingAction::~MySteppingAction() {}

void MySteppingAction::writeToFile(
    std::ofstream &out, G4double x, G4double y, G4double z, G4double time,
    G4double energyDep, const G4ThreeVector &momentum, G4int eventID,
    G4int parentID, G4int stepID, G4int trackID, const G4String &processName,
    const G4String &particleName, const G4String &volumeName, G4int copyNumber,
    const G4String &inout) {
  if (out.is_open()) {
    out << std::setw(8) << eventID << std::setw(8) << parentID << std::setw(8)
        << stepID << std::setw(8) << trackID << std::setw(20) << processName
        << std::setw(15) << particleName << std::setw(18) << volumeName
        << std::setw(8) << copyNumber << std::setw(6) << inout << std::fixed
        << std::setprecision(3) << std::right << std::setw(10) << x
        << std::setw(10) << y << std::setw(10) << z << std::setw(12)
        << time / ns << std::setw(15) << energyDep / MeV << std::setw(12)
        << momentum.x() / MeV << std::setw(12) << momentum.y() / MeV
        << std::setw(12) << momentum.z() / MeV << std::endl;
  } else {
    // If the file is not open (e.g., toggled off in RunAction), silently ignore
    // G4cerr << "[ERROR] PionInteractions output file is not open!" << G4endl;
  }
}

void MySteppingAction::UserSteppingAction(const G4Step *step) {
  // === NEW: Track secondary particles created from antiproton-silicon
  // collisions ===
  G4Track *track = step->GetTrack();
  G4String particleName = track->GetDefinition()->GetParticleName();

  // If this is a secondary particle (parentID > 0)
  if (track->GetParentID() > 0) {

    // --- CRITICAL FIX 1: Only count the FIRST step of the particle ---
    if (track->GetCurrentStepNumber() == 1) {

      // --- CRITICAL FIX 2: Check the BIRTH volume (Logical Volume at Vertex)
      // ---
      const G4LogicalVolume *vertexVolume = track->GetLogicalVolumeAtVertex();
      G4String vertexVolName = "";
      if (vertexVolume)
        vertexVolName = vertexVolume->GetName();

      // Check if the particle was BORN in a Wall or Counter
      if (vertexVolName.find("WallLog") != std::string::npos ||
          vertexVolName.find("SolidCounterLog") != std::string::npos) {

        // Filter: Exclude electrons, positrons, and neutrinos to avoid
        // delta-ray clutter But allow everything else (Deuterons, Alphas,
        // Kaons, Muons, etc.)
        // USER REQUEST: See ALL particles (removed filter for e-, e+, nu)
        if (true) {

          // --- DETERMINE SOURCE ID ---
          G4int sourceID = 0;
          const G4VTouchable *touchable =
              step->GetPreStepPoint()->GetTouchable();

          if (vertexVolName.find("SolidCounterLog") != std::string::npos) {
            sourceID = touchable->GetCopyNumber(0);
          } else if (vertexVolName.find("WallLog") != std::string::npos) {
            sourceID = touchable->GetCopyNumber(2);
          }

          // Only proceed if we identified a valid source
          if (sourceID > 0) {

            fEventAction->SetHadGratingCollision(true);
            fEventAction->fSecondaryParticleCount++;

            MyEventAction::SecondaryParticleInfo info;
            info.particleName = particleName;
            info.kineticEnergy = track->GetKineticEnergy() / MeV;
            info.momentum = track->GetMomentum();
            info.parentID = track->GetParentID();

            // --- 🔴 THE MISSING LINE IS HERE 🔴 ---
            info.volumeID = sourceID;
            // -------------------------------------

            // G4cout << "DEBUG: Saved Particle " << particleName
            //        << " with SourceID: " << info.volumeID << G4endl;

            fEventAction->GetSecondaryParticles(track->GetParentID())
                .push_back(info);
          }
        }
      }
    }
  }
  // === END: Secondary particle tracking ===

  G4StepPoint *preStepPoint = step->GetPreStepPoint();
  G4StepPoint *postStepPoint = step->GetPostStepPoint();
  G4VPhysicalVolume *volume = preStepPoint->GetPhysicalVolume();

  // --- DECLARE VOLUMES AT THE TOP ---
  G4VPhysicalVolume *preVolume = preStepPoint->GetPhysicalVolume();
  G4VPhysicalVolume *postVolume = postStepPoint->GetPhysicalVolume();

  G4String volumeName = volume->GetName();
  G4int copyNumber = volume->GetCopyNo();

  G4int trackID = track->GetTrackID();

  // Only process pions and kaons
  if (particleName != "pi+" && particleName != "pi-" && particleName != "pi0" &&
      particleName != "kaon+" && particleName != "kaon-") {
    return; // Skip all other particles
  }

  // ==============================================================================
  // --- NEW, CORRECTED LOGIC FOR ALL ANGULAR DEVIATION HISTOGRAMS ---
  // ==============================================================================
  auto manager = G4AnalysisManager::Instance();

  // --- Logic for the MULTI-MODULE setup ---
  if (postVolume && postVolume->GetName() == "Scintillator") {
    // postVolume is the volume the particle is entering and
    // preStepPoint->GetMomentum() is the momentum at entry to postVolume
    // preVolume is the volume the particle is exiting (or nullptr if none) and
    // postStepPoint->GetMomentum() is the momentum at exit from preVolume (or
    // same as entry if no step inside)
    bool isEnteringScint =
        (!preVolume || preVolume->GetName() != "Scintillator");

    if (isEnteringScint) {
      // --- Intra-Module Entry ---
      // Store momentum for the specific scintillator copy number
      G4int copyNumber = postVolume->GetCopyNo();
      fEventAction->StoreIntraModuleMomentum(track, copyNumber,
                                             preStepPoint->GetMomentum());

      // --- Inter-Module Entry ---
      bool isFront = (copyNumber >= 0 && copyNumber < 100) ||
                     (copyNumber >= 200 && copyNumber < 300);
      bool isBack = (copyNumber >= 100 && copyNumber < 200) ||
                    (copyNumber >= 300 && copyNumber < 400);

      if (isFront) {
        // Only store the momentum if it's the FIRST time entering a front
        // module zero momentum means it's the first entry and should be stored
        // (preStepPoint->GetMomentum() is the entry momentum)
        if (fEventAction->GetInterModuleMomentum(track).mag() == 0.) {
          fEventAction->StoreInterModuleMomentum(track,
                                                 preStepPoint->GetMomentum());
        }
      } else if (isBack) {
        // If entering a back module, we could also check if there's a stored
        // front momentum
        G4ThreeVector frontMomentum =
            fEventAction->GetInterModuleMomentum(track);
        if (frontMomentum.mag() > 0) {
          // if there's a valid front momentum, calculate deviation between
          // frontMomentum and current momentum on the back module entry.
          G4double deviation =
              frontMomentum.angle(preStepPoint->GetMomentum()) / deg;
          manager->FillH1(manager->GetH1Id("InterModuleDeviation"), deviation);
        }
      }
    }
  }

  if (preVolume && preVolume->GetName() == "Scintillator") {
    bool isExitingScint =
        (!postVolume || postVolume->GetName() != "Scintillator");

    if (isExitingScint) {
      // --- Intra-Module Exit ---
      G4int copyNumber = preVolume->GetCopyNo();
      G4ThreeVector entryMomentum =
          fEventAction->GetIntraModuleMomentum(track, copyNumber);
      if (entryMomentum.mag() > 0) {
        // Calculate deviation between entryMomentum and current momentum on
        // exit from the scintillator
        G4double deviation =
            entryMomentum.angle(postStepPoint->GetMomentum()) / deg;
        manager->FillH1(manager->GetH1Id("IntraModuleDeviation"), deviation);
        // Optional: Clear the stored momentum for this track/copy number pair
        fEventAction->ClearIntraModuleMomentum(track, copyNumber);
      }
    }
  }

  // --- SEPARATE Logic for the SINGLE SCINTILLATOR setup ---
  // This logic is more robust when written to explicitly check for transitions
  if (postVolume && postVolume->GetName() == "OneScintillator1" &&
      preVolume->GetName() != "OneScintillator1") {
    // Particle is ENTERING the volume
    fEventAction->StoreSingleScintMomentum(trackID,
                                           preStepPoint->GetMomentum());
  }
  if (preVolume && preVolume->GetName() == "OneScintillator1" &&
      (!postVolume || postVolume->GetName() != "OneScintillator1")) {
    // Particle is EXITING the volume
    G4ThreeVector entryMomentum = fEventAction->GetSingleScintMomentum(trackID);
    if (entryMomentum.mag() > 0) {
      G4double deviation =
          entryMomentum.angle(postStepPoint->GetMomentum()) / deg;
      manager->FillH1(manager->GetH1Id("SingleScintDeviation"), deviation);
    }
  }

  // --- Logic for the TWO BACK-TO-BACK SCINTILLATORS setup ---
  // This logic also benefits from being more explicit
  if (postVolume && postVolume->GetName() == "OneScintillator1" &&
      preVolume->GetName() != "OneScintillator1") {
    // Entering the front scintillator
    fEventAction->StoreB2BFrontMomentum(trackID, preStepPoint->GetMomentum());
  }
  if (postVolume && postVolume->GetName() == "OneScintillator2" &&
      preVolume->GetName() == "OneScintillator1") {
    // Entering the back scintillator FROM the front one
    G4ThreeVector frontMomentum = fEventAction->GetB2BFrontMomentum(trackID);
    if (frontMomentum.mag() > 0) {
      G4double deviation =
          frontMomentum.angle(preStepPoint->GetMomentum()) / deg;
      manager->FillH1(manager->GetH1Id("TwoScintB2BDeviation"), deviation);
    }
  }
  // ==============================================================================

  G4ThreeVector posPre = preStepPoint->GetPosition();
  G4ThreeVector posPost = postStepPoint->GetPosition();
  G4double x = posPre.x() / cm;
  G4double y = posPre.y() / cm;
  G4double z = posPre.z() / cm;
  G4double energyDep = step->GetTotalEnergyDeposit();

  G4int eventID =
      G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
  G4int parentID = track->GetParentID();
  G4int stepID = track->GetCurrentStepNumber();
  G4String processName = "None";
  if (step->GetPostStepPoint()->GetProcessDefinedStep())
    processName =
        step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();

  // Get output stream from RunAction (const-correct)
  auto runAction = static_cast<const MyRunAction *>(
      G4RunManager::GetRunManager()->GetUserRunAction());
  std::ofstream &out = runAction->GetPionFile();

  // Right Front: z in [20, 70] cm
  if (copyNumber >= 0 && copyNumber < 100) {
    if (preStepPoint->GetStepStatus() == fGeomBoundary) {
      writeToFile(out, x, y, z, preStepPoint->GetGlobalTime(), energyDep,
                  preStepPoint->GetMomentum(), eventID, parentID, stepID,
                  trackID, processName, particleName, volumeName, copyNumber,
                  "in");
    }
    if (postStepPoint->GetStepStatus() == fGeomBoundary) {
      writeToFile(out, posPost.x() / cm, posPost.y() / cm, posPost.z() / cm,
                  postStepPoint->GetGlobalTime(), 0.,
                  postStepPoint->GetMomentum(), eventID, parentID, stepID,
                  trackID, processName, particleName, volumeName, copyNumber,
                  "out");
    }
  }

  // Right Back: z in [20, 70] cm
  if (copyNumber >= 100 && copyNumber < 200) {
    if (preStepPoint->GetStepStatus() == fGeomBoundary) {
      writeToFile(out, x, y, z, preStepPoint->GetGlobalTime(), energyDep,
                  preStepPoint->GetMomentum(), eventID, parentID, stepID,
                  trackID, processName, particleName, volumeName, copyNumber,
                  "in");
    }
    if (postStepPoint->GetStepStatus() == fGeomBoundary) {
      writeToFile(out, posPost.x() / cm, posPost.y() / cm, posPost.z() / cm,
                  postStepPoint->GetGlobalTime(), 0.,
                  postStepPoint->GetMomentum(), eventID, parentID, stepID,
                  trackID, processName, particleName, volumeName, copyNumber,
                  "out");
    }
  }

  // Left Front: z in [-70, -20] cm
  if (copyNumber >= 200 && copyNumber < 300) {
    if (preStepPoint->GetStepStatus() == fGeomBoundary) {
      writeToFile(out, x, y, z, preStepPoint->GetGlobalTime(), energyDep,
                  preStepPoint->GetMomentum(), eventID, parentID, stepID,
                  trackID, processName, particleName, volumeName, copyNumber,
                  "in");
    }
    if (postStepPoint->GetStepStatus() == fGeomBoundary) {
      writeToFile(out, posPost.x() / cm, posPost.y() / cm, posPost.z() / cm,
                  postStepPoint->GetGlobalTime(), 0.,
                  postStepPoint->GetMomentum(), eventID, parentID, stepID,
                  trackID, processName, particleName, volumeName, copyNumber,
                  "out");
    }
  }

  // Left Back: z in [-70, -20] cm
  if (copyNumber >= 300 && copyNumber < 400) {
    if (preStepPoint->GetStepStatus() == fGeomBoundary) {
      writeToFile(out, x, y, z, preStepPoint->GetGlobalTime(), energyDep,
                  preStepPoint->GetMomentum(), eventID, parentID, stepID,
                  trackID, processName, particleName, volumeName, copyNumber,
                  "in");
    }
    if (postStepPoint->GetStepStatus() == fGeomBoundary) {
      writeToFile(out, posPost.x() / cm, posPost.y() / cm, posPost.z() / cm,
                  postStepPoint->GetGlobalTime(), 0.,
                  postStepPoint->GetMomentum(), eventID, parentID, stepID,
                  trackID, processName, particleName, volumeName, copyNumber,
                  "out");
    }
  }

  // Accumulate total energy deposition
  if (energyDep > 0.) {
    fEventAction->AddEnergyDep(energyDep);

    const G4VProcess *process = postStepPoint->GetProcessDefinedStep();
    G4String procName = process ? process->GetProcessName() : "None";

    // Map process name to bin index
    int procIndex = 0;
    if (procName == "Transportation")
      procIndex = 0;
    else if (procName == "compt")
      procIndex = 1;
    else if (procName == "conv")
      procIndex = 2;
    else if (procName == "hadElastic")
      procIndex = 3;
    else if (procName == "hIoni")
      procIndex = 4;
    else if (procName == "Decay")
      procIndex = 5;
    else if (procName == "CoulombScat")
      procIndex = 6;
    else if (procName == "msc")
      procIndex = 7;
    else if (procName == "phot")
      procIndex = 8;
    else if (procName == "pi+Inelastic")
      procIndex = 9;
    else if (procName == "pi-Inelastic")
      procIndex = 10;
    else if (procName == "kaon-Inelastic")
      procIndex = 11;
    else if (procName == "kaon+Inelastic")
      procIndex = 12;
    // else leave as 0 for unknown

    manager->FillH2(manager->GetH2Id("Edep2DByProcess"), procIndex,
                    energyDep / MeV);
  }
}