#ifndef HITBUFFER_HH
#define HITBUFFER_HH

#include "G4ThreeVector.hh"
#include <map>

struct HitPoint {
  G4ThreeVector pos_cm; // Position in cm
  G4int eventID;
  G4int trackID;
};

// Specific maps for top and bottom modules
// Specific maps for top and bottom modules removed

void ClearHitBuffers();

#endif
