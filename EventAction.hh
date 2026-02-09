#ifndef EVENTACTION_HH
#define EVENTACTION_HH

#include "G4UserEventAction.hh"
#include "G4Event.hh"
#include "G4ThreeVector.hh"
#include "globals.hh" // Recommended for G4int, G4double, etc.
#include <map>

// Forward declarations
class MyRunAction;
class G4Track;

class MyEventAction : public G4UserEventAction
{
public:
    MyEventAction(MyRunAction*);
    ~MyEventAction();
    
    virtual void BeginOfEventAction(const G4Event*);
    virtual void EndOfEventAction(const G4Event*);
    
    void AddEnergyDep(G4double edep) { fEnergyDeposition += edep; }

    // These appear to be for a different calculation, leaving them as is
    void SetFrontPoint(const G4ThreeVector& front);
    void SetBackPoint(const G4ThreeVector& back);

    // --- MODIFIED & NEW: Methods for Intra-Module Deviation (per scintillator) ---
    void StoreIntraModuleMomentum(const G4Track* track, G4int copyNo, const G4ThreeVector& momentum);
    G4ThreeVector GetIntraModuleMomentum(const G4Track* track, G4int copyNo) const;
    void ClearIntraModuleMomentum(const G4Track* track, G4int copyNo);
    
    // --- Methods for Inter-Module Deviation (between front and back) ---
    void StoreInterModuleMomentum(const G4Track* track, const G4ThreeVector& momentum);
    G4ThreeVector GetInterModuleMomentum(const G4Track* track) const;

    // --- Methods for the single scintillator test ---
    void StoreSingleScintMomentum(G4int trackID, const G4ThreeVector& momentum);
    G4ThreeVector GetSingleScintMomentum(G4int trackID);

    // --- Methods for the back-to-back scintillator test ---
    void StoreB2BFrontMomentum(G4int trackID, const G4ThreeVector& momentum);
    G4ThreeVector GetB2BFrontMomentum(G4int trackID) const;
    
    // --- NEW: Secondary particle tracking (public for SteppingAction access) ---
    struct SecondaryParticleInfo {
        G4String particleName;
        G4double kineticEnergy;  // MeV
        G4ThreeVector momentum;
        G4int parentID;
        G4int volumeID;
    };
    
    std::vector<SecondaryParticleInfo>& GetSecondaryParticles(G4int parentID) 
    { return fSecondaryParticlesMap[parentID]; }
    
    void SetHadGratingCollision(bool val) { fHadAntiprotonSiliconCollision = val; }
    bool GetHadGratingCollision() const { return fHadAntiprotonSiliconCollision; }
    
    G4int fSecondaryParticleCount;
    std::map<G4int, std::vector<SecondaryParticleInfo>> fSecondaryParticlesMap;
    bool fHadAntiprotonSiliconCollision;
    
private:
    G4double fEnergyDeposition;

    // For a separate angular deviation calculation
    G4ThreeVector fFrontPosition;
    G4ThreeVector fBackPosition;
    G4bool hasFront = false;
    G4bool hasBack = false;

    // --- MODIFIED: Data structure for Intra-Module momentum ---
    //   Maps a track to another map that associates a copy number with its entry momentum
    std::map<const G4Track*, std::map<G4int, G4ThreeVector>> fIntraModuleMomentumMap;

    // --- MODIFIED: Data structure for Inter-Module momentum ---
    //   Maps a track to its initial entry momentum into a front module
    std::map<const G4Track*, G4ThreeVector> fInterModuleMomentumMap;

    // --- Data structures for single/B2B tests (using track ID is fine here) ---
    std::map<G4int, G4ThreeVector> fSingleScintMomentumMap;
    std::map<G4int, G4ThreeVector> fB2BFrontMomentumMap;

    MyRunAction* fRunAction;
};

#endif