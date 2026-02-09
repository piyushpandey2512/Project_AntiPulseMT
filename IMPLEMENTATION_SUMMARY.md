# Secondary Particle Histogram Implementation - Summary

## What Was Done

Your code has been successfully updated to track and histogram secondary particles produced from antiproton-silicon grating collisions.

## Files Modified

1. **RunAction.cc** - Added 5 new histograms
2. **EventAction.hh** - Added public data structures and methods for secondary particle tracking
3. **EventAction.cc** - Added histogram filling logic and event tracking reset
4. **SteppingAction.cc** - Added secondary particle detection and storage logic

## New Histograms Available

All histograms are automatically created and filled. You can access them from your ROOT output file:

### 1. **SecondaryParticleType** (1D Histogram)
- **Bins:** 7 (one for each particle type)
- **What it shows:** Count of each particle type produced
- **Bin mapping:**
  - 0: Gamma (γ)
  - 1: π+
  - 2: π-
  - 3: π0
  - 4: Proton
  - 5: Neutron
  - 6: Other

### 2. **SecondaryParticleCount** (1D Histogram)
- **Range:** 0-50 particles
- **What it shows:** How many secondary particles were produced per collision event

### 3. **SecondaryParticleKineticEnergy** (1D Histogram)
- **Range:** 0-1000 MeV
- **What it shows:** Kinetic energy distribution of secondary particles

### 4. **SecondaryParticleTypePerEvent** (2D Histogram)
- **Axes:** Event number (0-100) vs Particle type (0-6)
- **What it shows:** Which particle types were produced in which events

### 5. **AntiprotonGratingCollisionCount** (1D Histogram)
- **What it shows:** Total number of collision events detected

## How It Works

### Detection Logic
1. **Every step** in the simulation, secondary particles are checked
2. **If a secondary particle** is created in a silicon grating volume:
3. **And it was created by** an inelastic collision (hadron interaction):
4. **Then it is recorded** with its name, energy, and momentum

### Key Data Structure
```cpp
struct SecondaryParticleInfo {
    G4String particleName;     // Name of the particle (e.g., "pi+", "gamma")
    G4double kineticEnergy;    // Kinetic energy in MeV
    G4ThreeVector momentum;    // 3D momentum vector
    G4int parentID;            // ID of the parent antiproton
};
```

## Quick Start to View Results

### After running your simulation:

```bash
root
root [0] TFile f("Project_AntiPulse_TIMESTAMP.root")
root [1] SecondaryParticleType->Draw()
root [2] SecondaryParticleCount->Draw()
root [3] SecondaryParticleKineticEnergy->Draw()
root [4] AntiprotonGratingCollisionCount->Draw()
```

### Or in a ROOT macro:

```cpp
void view_secondary_particles() {
    TFile f("Project_AntiPulse_TIMESTAMP.root");
    
    // Create a canvas with multiple histograms
    TCanvas c("secondary", "Secondary Particles", 800, 600);
    c.Divide(2, 2);
    
    c.cd(1); SecondaryParticleType->Draw("bar");
    c.cd(2); SecondaryParticleCount->Draw();
    c.cd(3); SecondaryParticleKineticEnergy->Draw();
    c.cd(4); AntiprotonGratingCollisionCount->Draw();
    
    c.SaveAs("secondary_particles.png");
}
```

## Testing Your Implementation

To verify the code is working:

1. **Ensure antiprotons reach the gratings** - Check with a simple test macro
2. **Run a small number of events** (e.g., 1000) to test quickly
3. **Open the ROOT file** and check if histograms are filled (not empty)
4. **Check the output** - You should see secondary particle counts > 0 if collisions occurred

## Expected Results

For a typical antiproton beam hitting silicon gratings:

- **Dominant particle types:** Pions (π+, π-, π0) and protons
- **Secondary particles:** Neutrons and gammas
- **Typical multiplicity:** 3-8 particles per collision
- **Energy range:** Most particles in 10-500 MeV range

## Customization Options

### Change Particle Type Codes
Edit in `EventAction.cc` to add more particle types:
```cpp
else if (particle.particleName == "your_particle") typeCode = 7;
```

### Adjust Energy Range
Edit in `RunAction.cc`:
```cpp
// Change from (100, 0, 1000) to your range
manager->CreateH1("SecondaryParticleKineticEnergy", "...", 100, 0, 2000);
```

### Track Additional Properties
Modify the `SecondaryParticleInfo` struct and add fields as needed.

## Compilation Status

✅ **Code compiles successfully**
✅ **All histograms created**
✅ **Secondary particle detection active**
✅ **Ready to run simulations**

## Next Steps

1. Rebuild if needed: `cd build && make`
2. Run your simulation: `./AntiPulse run.mac`
3. Open the ROOT file and analyze the histograms
4. Use the example ROOT macros to create plots and statistics

For more details, see: `SECONDARY_PARTICLE_ANALYSIS.md`
