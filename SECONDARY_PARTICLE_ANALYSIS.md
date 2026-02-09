# Secondary Particle Analysis from Antiproton-Silicon Collisions

## Overview
This guide explains how to generate and analyze histograms showing the number and types of secondary particles produced when antiprotons collide with the silicon gratings in your simulation.

## What Was Added

### 1. Histograms Created

The following histograms are now automatically created in your ROOT output file:

#### Particle Type Histogram
- **Name:** `SecondaryParticleType`
- **Type:** 1D Histogram (7 bins)
- **What it shows:** Distribution of secondary particle types produced
- **Bin meanings:**
  - Bin 0: Gamma (γ)
  - Bin 1: Positive pion (π+)
  - Bin 2: Negative pion (π-)
  - Bin 3: Neutral pion (π0)
  - Bin 4: Proton (p)
  - Bin 5: Neutron (n)
  - Bin 6: Other particles

#### Particle Count Histogram
- **Name:** `SecondaryParticleCount`
- **Type:** 1D Histogram (50 bins, 0-50 particles)
- **What it shows:** Number of secondary particles produced per collision event

#### Kinetic Energy Histogram
- **Name:** `SecondaryParticleKineticEnergy`
- **Type:** 1D Histogram (100 bins, 0-1000 MeV)
- **What it shows:** Kinetic energy distribution of secondary particles

#### 2D Event Histogram
- **Name:** `SecondaryParticleTypePerEvent`
- **Type:** 2D Histogram
- **What it shows:** Correlation between event number and particle types produced

#### Collision Count Histogram
- **Name:** `AntiprotonGratingCollisionCount`
- **Type:** 1D Histogram
- **What it shows:** Total number of collision events (those with secondary particles)

### 2. Code Changes Made

#### EventAction.hh
Added data structure to store secondary particle information:
```cpp
struct SecondaryParticleInfo {
    G4String particleName;
    G4double kineticEnergy;  // MeV
    G4ThreeVector momentum;
    G4int parentID;
};
```

Added tracking variables:
- `fSecondaryParticlesMap`: Maps parent track ID to list of secondary particles
- `fSecondaryParticleCount`: Counts secondary particles in current event
- `fHadAntiprotonSiliconCollision`: Flag to track if collision occurred

#### EventAction.cc
- Reset tracking variables at beginning of each event
- Added histogram filling code in `EndOfEventAction()` to populate the secondary particle histograms

#### SteppingAction.cc
Added logic to:
1. Detect secondary particles created in silicon gratings
2. Check if they were created by inelastic collisions (not just passing through)
3. Store secondary particle information (name, energy, momentum)
4. Trigger the collision flag

#### RunAction.cc
Added 5 new histograms to be created at the start of each run

## How to Use

### Step 1: Rebuild Your Code
```bash
cd build/
make clean && make
```

### Step 2: Run Your Simulation
```bash
./AntiPulse path/to/macro.mac
```

This will generate a ROOT file with the new histograms.

### Step 3: View Histograms in ROOT

Open ROOT and load your output file:
```bash
root
root [0] TFile f("Project_AntiPulse_TIMESTAMP.root")
root [1] f.ls()
```

Then draw the histograms:
```bash
# View particle type distribution
root [2] SecondaryParticleType->Draw()

# View particle count distribution
root [3] SecondaryParticleCount->Draw()

# View kinetic energy distribution
root [4] SecondaryParticleKineticEnergy->Draw()

# View 2D event correlation
root [5] SecondaryParticleTypePerEvent->Draw("colz")

# View collision count
root [6] AntiprotonGratingCollisionCount->Draw()
```

### Step 4: Create Custom Analysis Scripts

Create a ROOT macro (e.g., `analyze_secondary.C`) to extract more detailed statistics:

```cpp
void analyze_secondary() {
    TFile f("Project_AntiPulse_TIMESTAMP.root");
    
    TH1F* particleType = (TH1F*)f.Get("SecondaryParticleType");
    TH1F* particleCount = (TH1F*)f.Get("SecondaryParticleCount");
    TH1F* kinEnergy = (TH1F*)f.Get("SecondaryParticleKineticEnergy");
    
    // Print statistics
    std::cout << "Total Secondary Particles: " << particleType->GetEntries() << std::endl;
    std::cout << "Average Particles per Event: " << particleCount->GetMean() << std::endl;
    std::cout << "Average KE: " << kinEnergy->GetMean() << " MeV" << std::endl;
    
    // Print breakdown by type
    std::cout << "\nParticle Type Breakdown:" << std::endl;
    std::cout << "Gammas: " << particleType->GetBinContent(1) << std::endl;
    std::cout << "Pi+: " << particleType->GetBinContent(2) << std::endl;
    std::cout << "Pi-: " << particleType->GetBinContent(3) << std::endl;
    std::cout << "Pi0: " << particleType->GetBinContent(4) << std::endl;
    std::cout << "Protons: " << particleType->GetBinContent(5) << std::endl;
    std::cout << "Neutrons: " << particleType->GetBinContent(6) << std::endl;
    std::cout << "Other: " << particleType->GetBinContent(7) << std::endl;
    
    // Draw histograms
    TCanvas c;
    c.Divide(2, 3);
    
    c.cd(1); particleType->Draw();
    c.cd(2); particleCount->Draw();
    c.cd(3); kinEnergy->Draw();
    
    c.SaveAs("secondary_particles_analysis.png");
}
```

Run it with:
```bash
root analyze_secondary.C
```

## How It Works (Technical Details)

### Detection Logic
1. **Every step**, the code checks if a particle is a secondary (parentID > 0)
2. **If secondary**, it checks if the particle was created in a silicon grating volume
3. **If in grating**, it verifies the creation process was an inelastic collision
4. **If collision confirmed**, it:
   - Sets the `fHadAntiprotonSiliconCollision` flag
   - Increments the secondary particle counter
   - Stores particle information (name, energy, momentum)

### Histogram Filling
At the end of each event, if any collisions occurred:
- The secondary particle count is filled in `SecondaryParticleCount`
- Each secondary particle's type is filled in `SecondaryParticleType`
- Each particle's kinetic energy is filled in `SecondaryParticleKineticEnergy`
- The 2D event correlation is filled
- The collision count is incremented

## Important Notes

1. **Particle Detection**: Only secondary particles born in the grating volumes are tracked. Particles created elsewhere are not included.

2. **Collision Types**: The code looks for inelastic collisions (hadron interactions, decays, etc.). Simple Coulomb scattering is ignored.

3. **Antiproton Requirement**: The secondary particles must be created by an antiproton (or its decay products) for them to count. Direct definition of this could be enhanced in future versions.

4. **ROOT File**: All histograms are automatically saved in your ROOT output file and persist for later analysis.

5. **Performance**: The overhead of tracking secondary particles is minimal (one string comparison per secondary particle per step).

## Customization

### To Track Different Particles
Edit the particle type codes in `EventAction.cc` (EndOfEventAction function):
```cpp
if (particle.particleName == "your_particle_name") typeCode = X;
```

### To Change Energy Ranges
Modify the histogram creation in `RunAction.cc`:
```cpp
// Change from (100, 0, 1000) to your desired range
manager->CreateH1("SecondaryParticleKineticEnergy", "...", 100, 0, 2000);
```

### To Track Additional Properties
Add fields to the `SecondaryParticleInfo` struct and modify the storage code:
```cpp
struct SecondaryParticleInfo {
    G4String particleName;
    G4double kineticEnergy;
    G4ThreeVector momentum;
    G4int parentID;
    G4double position_x;  // NEW
    G4double position_z;  // NEW
};
```

## Expected Results

For a typical antiproton beam on silicon gratings, you should expect:
- **Dominant products**: Pions (π±, π0) and protons
- **Secondary products**: Neutrons and gammas (from nuclear excitation)
- **Average multiplicity**: Typically 3-8 particles per inelastic collision
- **Energy range**: Most particles in the 10-500 MeV range

## Troubleshooting

### No Secondary Particles Detected
- Check that antiprotons are actually being generated in your simulation
- Verify that antiprotons reach the grating (they might be stopped by upstream material)
- Check that the volume names in your geometry match the code (look for "Wall" or "SolidCounter")

### Compilation Errors
- Make sure you're using a modern C++ compiler (C++11 or later)
- Verify that G4 headers are correctly included
- Check that structs are properly defined with correct namespace

### ROOT File Issues
- Ensure the ROOT output file was created successfully
- Check that the histograms exist: `TFile f("your_file.root"); f.ls();`
- Verify histogram names match exactly when calling `GetH1Id()` and `GetH2Id()`

## Future Enhancements

Possible improvements to this system:
1. Track the exact creation location (x, y, z) of secondary particles
2. Add angular distribution histograms
3. Correlate secondary particles with primary antiproton properties
4. Add multiplicity vs. antiproton energy correlation
5. Create invariant mass histograms for particle pair analysis
