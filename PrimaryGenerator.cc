#include "PrimaryGenerator.hh"
#include "G4Event.hh"
#include <random>
#include "G4SystemOfUnits.hh"
#include "GeoConstruction.hh"
#include "G4RunManager.hh"

using namespace std;

// Add these as class members in PrimaryGenerator.hh or as static/global for quick testing
bool useThreeSourceCone = false;
bool useConeSourceTowardScintillator = false;
bool useTestSingleSource = false;
bool useConeSourceTowardSingleModule = false;
bool useConeSourceTowardFourModules = false;

// Antiproton beam options
bool useAntiprotonBeamParallel = false;
bool useAntiprotonBeamRandomAiming = false;
bool useAntiprotonBeamCone = true;

// Light source options
bool useLightBeamParallel = false;

// Moire Source Options
bool useMoireSourceUniform = false;
bool useMoireSourceDiagnostic = false;
bool useMoireSourceGaussian = false;
bool useMoireSourceGaussianIsotropic = false;
bool useMoireSourceRandomSource = false;

MyPrimaryParticles::MyPrimaryParticles()
    : G4VUserPrimaryGeneratorAction(), fDivergence(0.3 * mrad) // Default value
{
    fParticleGun = new G4ParticleGun(1);
    // NEW: Define the command /beam/setDivergence
    fMessenger = new G4GenericMessenger(this, "/beam/", "Beam Control");
    fMessenger->DeclareMethodWithUnit("setDivergence", "mrad", 
                                      &MyPrimaryParticles::SetDivergence, 
                                      "Set the Gaussian beam divergence.");
}

MyPrimaryParticles::~MyPrimaryParticles()
{
    delete fParticleGun;
}

void MyPrimaryParticles::GeneratePrimaries(G4Event* anEvent)
{
    if (anEvent->GetEventID() % 1000 == 0) {
        G4cout << "PrimaryGenerator: Generating event " << anEvent->GetEventID() << G4endl;
    }

    // Define the particle type
    G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
    G4ParticleDefinition* piPlus  = particleTable->FindParticle("pi+");
    G4ParticleDefinition* piMinus = particleTable->FindParticle("pi-");
    G4ParticleDefinition* piZero  = particleTable->FindParticle("pi0");
    G4ParticleDefinition* kPlus   = particleTable->FindParticle("kaon+");
    G4ParticleDefinition* kMinus  = particleTable->FindParticle("kaon-");
    G4ParticleDefinition* antiproton = particleTable->FindParticle("anti_proton");
    G4ParticleDefinition* photon = particleTable->FindParticle("gamma");

/***********************************************************************/

    if (useAntiprotonBeamCone) {
        if (!antiproton) {
            G4Exception("PrimaryGenerator::GeneratePrimaries",
                        "MyCode0001", FatalException,
                        "Particle 'anti_proton' not found.");
            return;
        }

        // --- 2. Define Beam Properties ---
        G4double beam_energy    = 10.0 * keV;
        
        // --- CHANGE: Tip of the cone (Point Source) ---
        G4double beam_radius    = 0.0 * mm; // 0.0 creates a single starting point
        
        G4double start_z        = -60.0 * cm; 
        
        // Divergence (Sigma): 0.3 mrad
        // Note: If you implemented the Messenger, change this to 'fDivergence'
        G4double divergence_sigma = fDivergence;
        // G4double divergence_sigma = 0.3 * mrad;

        // --- 3. Generate Position (Point Source) ---
        // Since radius is 0, x_pos and y_pos will be 0.
        G4double x_pos = 0.0;
        G4double y_pos = 0.0;
        
        // If radius was not 0, this logic would handle the disc:
        if (beam_radius > 0.0) {
             G4double r = beam_radius * std::sqrt(G4UniformRand());
             G4double phi = 2.0 * CLHEP::pi * G4UniformRand();
             x_pos = r * std::cos(phi);
             y_pos = r * std::sin(phi);
        }

        G4ThreeVector start_position(x_pos, y_pos, start_z);

        // --- 4. Generate Direction (Gaussian Divergence) ---
        // Generates a cone-like spread with Gaussian intensity profile
        G4double angleX = G4RandGauss::shoot(0.0, divergence_sigma);
        G4double angleY = G4RandGauss::shoot(0.0, divergence_sigma);

        G4ThreeVector beam_dir(angleX, angleY, 1.0);
        beam_dir = beam_dir.unit();

        // --- 5. Configure & Fire ---
        fParticleGun->SetParticleDefinition(antiproton);
        fParticleGun->SetParticleEnergy(beam_energy);
        fParticleGun->SetParticlePosition(start_position);
        fParticleGun->SetParticleMomentumDirection(beam_dir);
        
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }

/***********************************************************************/

    if (useLightBeamParallel) {
        // --- 1. Define the Particle Type ---

        G4ParticleDefinition* photon = particleTable->FindParticle("gamma");
        if (!photon) {
            G4Exception("PrimaryGenerator::GeneratePrimaries",
                        "MyCode0002", FatalException,
                        "Particle 'gamma' not found.");
            return;
        }

        // --- 2. Define Beam Properties ---
        G4double beam_energy   = 3.0 * eV; // Visible light ~413 nm
        G4double beam_radius   = 5.0 * cm; // A 5cm radius circle fully covers a 7cm x 7cm square
        G4double start_z       = -60.0 * cm; // Start 10cm before the first grating at z=-50cm
        G4ThreeVector beam_dir = G4ThreeVector(0, 0, 1); // Pointing along +Z axis

        // --- 3. Generate a Random Position within the Cylindrical Beam Area ---
        // We use sqrt(G4UniformRand()) to ensure a uniform spatial distribution
        // across the circular area, not bunched up in the center.
        G4double r = beam_radius * std::sqrt(G4UniformRand());
        G4double phi = 2.0 * CLHEP::pi * G4UniformRand();
        G4double x_pos = r * std::cos(phi);
        G4double y_pos = r * std::sin(phi);

        G4ThreeVector start_position(x_pos, y_pos, start_z);

        // --- 4. Configure the Particle Gun ---
        fParticleGun->SetParticleDefinition(photon);
        fParticleGun->SetParticleEnergy(beam_energy);
        fParticleGun->SetParticlePosition(start_position);
        fParticleGun->SetParticleMomentumDirection(beam_dir);
        
        // --- 5. Generate the Particle ---
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }

/***********************************************************************/

    if (useAntiprotonBeamParallel) {
        // --- 1. Define the Particle Type ---

        if (!antiproton) {
            G4Exception("PrimaryGenerator::GeneratePrimaries",
                        "MyCode0001", FatalException,
                        "Particle 'anti_proton' not found.");
            return;
        }

        // --- 2. Define Beam Properties ---
        G4double beam_energy   = 10.0 * keV;
        G4double beam_radius   = 5.0 * cm; // A 5cm radius circle fully covers a 7cm x 7cm square
        G4double start_z       = -60.0 * cm; // Start 10cm before the first grating at z=-50cm
        G4ThreeVector beam_dir = G4ThreeVector(0, 0, 1); // Pointing along +Z axis

        // --- 3. Generate a Random Position within the Cylindrical Beam Area ---
        // We use sqrt(G4UniformRand()) to ensure a uniform spatial distribution
        // across the circular area, not bunched up in the center.
        G4double r = beam_radius * std::sqrt(G4UniformRand());
        G4double phi = 2.0 * CLHEP::pi * G4UniformRand();
        G4double x_pos = r * std::cos(phi);
        G4double y_pos = r * std::sin(phi);

        G4ThreeVector start_position(x_pos, y_pos, start_z);

        // --- 4. Configure the Particle Gun ---
        fParticleGun->SetParticleDefinition(antiproton);
        fParticleGun->SetParticleEnergy(beam_energy);
        fParticleGun->SetParticlePosition(start_position);
        fParticleGun->SetParticleMomentumDirection(beam_dir);
        
        // --- 5. Generate the Particle ---
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }

/***********************************************************************/


        if (useAntiprotonBeamRandomAiming) {
        // --- 1. Define the Particle Type ---
        // (This part is unchanged)
        if (!antiproton) {
            G4Exception("PrimaryGenerator::GeneratePrimaries",
                        "MyCode0001", FatalException,
                        "Particle 'anti_proton' not found.");
            return;
        }

        // --- 2. Define Beam & Target Properties ---
        G4double beam_energy   = 10.0 * keV;

        // Source Disk Properties (where particles are born)
        G4double source_radius = 5.0 * cm;
        G4double source_z      = -60.0 * cm;

        // MODIFICATION: Define the Target Disk Properties
        // This should represent the area you want to guarantee is illuminated.
        // For example, a disk that covers your first grating.
        // You will need to adjust these values for your specific geometry.
        G4double target_radius = 7.0 * cm; // A radius big enough to cover the gratings
        G4double target_z      = -50.0 * cm; // The Z position of your first grating

        // --- 3. Generate a Random Position on the SOURCE Disk ---
        // This logic is excellent and remains unchanged. It correctly gives a uniform
        // distribution over the area of the circle.
        G4double r_source = source_radius * std::sqrt(G4UniformRand());
        G4double phi_source = 2.0 * CLHEP::pi * G4UniformRand();
        G4double x_source = r_source * std::cos(phi_source);
        G4double y_source = r_source * std::sin(phi_source);

        G4ThreeVector start_position(x_source, y_source, source_z);

        // --- 4. MODIFICATION: Generate a Random AIMING POINT on the TARGET Disk ---
        // We use the same technique to pick a random, uniformly distributed
        // point on our target circle.
        G4double r_target = target_radius * std::sqrt(G4UniformRand());
        G4double phi_target = 2.0 * CLHEP::pi * G4UniformRand();
        G4double x_target = r_target * std::cos(phi_target);
        G4double y_target = r_target * std::sin(phi_target);
        
        G4ThreeVector aim_point(x_target, y_target, target_z);

        // --- 5. MODIFICATION: Calculate the Final Momentum Direction ---
        // The direction is simply the vector pointing from the start position to the aim point.
        // We normalize it with .unit() to make it a direction vector of length 1.
        G4ThreeVector final_direction = (aim_point - start_position).unit();

        // --- 6. Configure the Particle Gun ---
        fParticleGun->SetParticleDefinition(antiproton);
        fParticleGun->SetParticleEnergy(beam_energy);
        fParticleGun->SetParticlePosition(start_position);
        fParticleGun->SetParticleMomentumDirection(final_direction); // Use the new random direction

        // --- 7. Generate the Particle ---
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }

/***********************************************************************/

    // --- 3-source cone emission with correct particle mix and energies ---
    if (useThreeSourceCone) {
        // G4ThreeVector stlPosition(-8.0 * cm, 3.5 * cm, 8.0 * cm);
        G4ThreeVector stlPosition(-8.0 * cm, 3.5 * cm, 0.0 * cm);   // Adjusted for better alignment with modules
        // G4ThreeVector stlPosition(0, 0, 0); // For testing, using origin


        // Define three fixed source positions
        std::vector<G4ThreeVector> sourcePositions = {
            stlPosition,
            stlPosition + G4ThreeVector(0, 0, 45.0 * cm),
            stlPosition + G4ThreeVector(0, 0, -45.0 * cm)
        };

        // Calculate average module center for cone axis
        // std::vector<G4ThreeVector> moduleCenters = {
        //     G4ThreeVector(15.8*cm, 0, 45*cm),
        //     G4ThreeVector(23.8*cm, 0, 45*cm),
        //     G4ThreeVector(15.8*cm, 0, -45*cm),
        //     G4ThreeVector(23.8*cm, 0, -45*cm)
        // };

        std::vector<G4ThreeVector> moduleCenters = {
            G4ThreeVector(10 * cm, 0, 30 * cm),
            G4ThreeVector(20 * cm, 0, 30 * cm),
            G4ThreeVector(10 * cm, 0, -30 * cm),
            G4ThreeVector(20 * cm, 0, -30 * cm)
        };

        G4ThreeVector avgModuleCenter(0, 0, 0);
        for (const auto& module : moduleCenters) {
            avgModuleCenter += module;
        }
        avgModuleCenter /= moduleCenters.size();

        G4double coneHalfAngle = 70.0 * deg;

        for (const auto& sourceCenter : sourcePositions) {
            // Set position
            fParticleGun->SetParticlePosition(sourceCenter);

            // Select particle type based on annihilation ratios (same as useMoireSource)
            G4ParticleDefinition* particle;
            G4double energy;
            G4double rand = G4UniformRand();

            if (rand < 0.60) { // Charged pions (60% of total)
                particle = (G4UniformRand() < 0.5) ? piPlus : piMinus;
                energy = 230 * MeV; // Average kinetic energy
            }
            else if (rand < 0.80) { // Neutral pions (40% of total)
                particle = particleTable->FindParticle("pi0");
                energy = 230 * MeV; // Same average energy as charged
            }
            else if (rand < 0.96) { // Charged kaons (16% of total)
                particle = (G4UniformRand() < 0.5) ? kPlus : kMinus;
                energy = 150*MeV + 250*MeV*G4UniformRand(); // Uniform distribution
            }
            else { // Eta mesons (4% - neutral, not detected)
                continue; // Skip neutral particles for charged detection
            }

            // Emit in cone toward module center
            G4ThreeVector coneAxis = (avgModuleCenter - sourceCenter).unit();

            G4double cosTheta = std::cos(coneHalfAngle);
            G4double randomCosTheta = cosTheta + (1 - cosTheta) * G4UniformRand();
            G4double sinTheta = std::sqrt(1 - randomCosTheta * randomCosTheta);
            G4double phi = 2 * M_PI * G4UniformRand();
            G4ThreeVector randomDirection(
                sinTheta * std::cos(phi),
                sinTheta * std::sin(phi),
                randomCosTheta);

            G4ThreeVector finalDirection = randomDirection.rotateUz(coneAxis);

            fParticleGun->SetParticleDefinition(particle);
            fParticleGun->SetParticleMomentumDirection(finalDirection.unit());
            fParticleGun->SetParticleEnergy(energy);
            fParticleGun->GeneratePrimaryVertex(anEvent);
        }
    }



/***********************************************************************/

    if (useConeSourceTowardSingleModule) {

        // Scintillator parameters
        G4double scinCenterX = 10.0 * cm;
        G4double scinCenterY = 0.0 * cm;
        G4double scinCenterZ = 0.0 * cm;
        G4double scinHalfX = 2.5 * cm / 2.0;
        G4double scinHalfY = 0.6 * cm / 2.0;
        G4double scinHalfZ = 50.0 * cm / 2.0;

        // Source at the origin
        G4ThreeVector sourcePos(0, 0, 0);

        // Randomly pick a point on the +y face of the single module
        G4double randX = scinCenterX + (2.0 * scinHalfX) * (G4UniformRand() - 0.5);
        G4double randY = scinCenterY + scinHalfY; // +y face
        G4double randZ = scinCenterZ + (2.0 * scinHalfZ) * (G4UniformRand() - 0.5);
        G4ThreeVector targetPoint(randX, randY, randZ);

        // Direction from source to target point
        G4ThreeVector direction = (targetPoint - sourcePos).unit();

        // Select particle type based on annihilation ratios
        G4ParticleDefinition* particle;
        G4double energy;
        G4double rand = G4UniformRand();

        if (rand < 0.60) { // Charged pions (60% of total)
            particle = (G4UniformRand() < 0.5) ? piPlus : piMinus;
            energy = 230 * MeV; // Average kinetic energy
        }
        else if (rand < 0.80) { // Neutral pions (40% of total)
            particle = particleTable->FindParticle("pi0");
            energy = 230 * MeV; // Same average energy as charged
        }
        else if (rand < 0.96) { // Charged kaons (16% of total)
            particle = (G4UniformRand() < 0.5) ? kPlus : kMinus;
            energy = 150*MeV + 250*MeV*G4UniformRand(); // Uniform distribution
        }
        else { // Eta mesons (4% - neutral, not detected)
            return; // Skip neutral particles for charged detection
        }

        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticleEnergy(energy);
        fParticleGun->SetParticlePosition(sourcePos);
        fParticleGun->SetParticleMomentumDirection(direction);
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }




/***********************************************************************/

    if (useConeSourceTowardScintillator) {
        // Scintillator parameters
        G4double scinCenterX = 10.0 * cm;
        G4double scinCenterY = 0.0 * cm;
        G4double scinCenterZ = 0.0 * cm;
        G4double scinHalfY = 0.3 * cm;      // half-width in y (0.6 cm total)
        G4double scinHalfZ = 25.0 * cm;     // half-length in z (50 cm total)
        G4double offset = 0.01 * cm;        // small offset to be just outside

        // Source at the origin
        G4ThreeVector sourcePos(0, 0, 0);

        // Randomly pick a point on the +y face of the scintillator
        G4double randZ = scinCenterZ + (2.0 * scinHalfZ) * (G4UniformRand() - 0.5);
        G4double randX = scinCenterX; // always center in x for this face
        G4double randY = scinCenterY + scinHalfY; // face at +y

        G4ThreeVector targetPoint(randX, randY, randZ);

        // Direction from source to target point
        G4ThreeVector direction = (targetPoint - sourcePos).unit();

        // Select particle type based on annihilation ratios
        G4ParticleDefinition* particle;
        G4double energy;
        G4double rand = G4UniformRand();
        
        if (rand < 0.60) { // Charged pions (60% of total)
            particle = (G4UniformRand() < 0.5) ? piPlus : piMinus;
            energy = 230 * MeV; // Average kinetic energy
        }
        else if (rand < 0.80) { // Neutral pions (40% of total)
            particle = particleTable->FindParticle("pi0");
            energy = 230 * MeV; // Same average energy as charged
        }
        else if (rand < 0.96) { // Charged kaons (16% of total)
            particle = (G4UniformRand() < 0.5) ? kPlus : kMinus;
            energy = 150*MeV + 250*MeV*G4UniformRand(); // Uniform distribution
        }
        else { // Eta mesons (4% - neutral, not detected)
            return; // Skip neutral particles for charged detection
        }
        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticleEnergy(energy);
        fParticleGun->SetParticlePosition(sourcePos);
        fParticleGun->SetParticleMomentumDirection(direction);
    
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }



/***********************************************************************/

    // Diagnostic Moire source


        if (useMoireSourceDiagnostic) {

        // =================================================================
        // --- SYMMETRY DIAGNOSTIC TEST ---
        // Instructions:
        // 1. Uncomment ONE of the two test cases below. Leave the other commented.
        // 2. In your terminal, go to your build directory.
        // 3. Force a clean re-compile by running:  make clean && make
        // 4. Run your simulation for a fixed number of events (e.g., 100,000).
        // 5. Run your reconstruction code and record the "Total extrapolated vertices found".
        // 6. Come back here, comment out the first test case, and uncomment the second one.
        // 7. Repeat steps 3, 4, and 5.
        // 8. Compare the number of vertices from both runs. If the code and geometry
        //    are symmetric, these two numbers should be nearly identical.

        // --- Test Case 1: Positive Z Source ONLY ---
        // const G4ThreeVector testSourceCenter(-8.0 * cm, 3.5 * cm, 50.0 * cm);
        // const G4ThreeVector testAimPoint(25.8 * cm, 0.0, 30.0 * cm); // Aims at right arm

        // --- Test Case 2: Negative Z Source ONLY ---
        const G4ThreeVector testSourceCenter(-8.0 * cm, 3.5 * cm, -50.0 * cm);
        const G4ThreeVector testAimPoint(25.8 * cm, 0.0, -30.0 * cm); // Aims at left arm
        // =================================================================


        // --- Common settings for the test ---
        std::vector<G4ThreeVector> sourcePositions = { testSourceCenter }; // Use only the selected test source
        G4double boxHalfX = 7.0 * cm / 2.0;
        G4double boxHalfY = 7.0 * cm / 2.0;
        G4double boxHalfZ = 250.0 * micrometer / 2.0;
        G4double coneHalfAngle = 40.0 * deg;


        // This loop will only run once per event, for the single source defined above.
        for (const auto& sourceCenter : sourcePositions) {
            // Generate a random position *within* the source box
            G4double rx = (G4UniformRand() - 0.5) * (2.0 * boxHalfX);
            G4double ry = (G4UniformRand() - 0.5) * (2.0 * boxHalfY);
            G4double rz = (G4UniformRand() - 0.5) * (2.0 * boxHalfZ);
            G4ThreeVector sourcePos = sourceCenter + G4ThreeVector(rx, ry, rz);

            // Select particle type and energy
            G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
            G4ParticleDefinition* piPlus  = particleTable->FindParticle("pi+");
            G4ParticleDefinition* piMinus = particleTable->FindParticle("pi-");
            G4ParticleDefinition* kPlus   = particleTable->FindParticle("kaon+");
            G4ParticleDefinition* kMinus  = particleTable->FindParticle("kaon-");
            
            G4ParticleDefinition* particle;
            G4double energy;
            G4double rand = G4UniformRand();
            
            if (rand < 0.60) {
                particle = (G4UniformRand() < 0.5) ? piPlus : piMinus;
                energy = 230 * MeV;
            }
            else if (rand < 0.80) {
                particle = particleTable->FindParticle("pi0");
                energy = 230 * MeV;
            }
            else if (rand < 0.96) {
                particle = (G4UniformRand() < 0.5) ? kPlus : kMinus;
                energy = 150*MeV + 250*MeV*G4UniformRand();
            }
            else {
                continue; // Skip eta mesons
            }

            // Define the cone axis from the particle's origin to the pre-defined aim point
            G4ThreeVector coneAxis = (testAimPoint - sourcePos).unit();

            // Sample a random direction within the defined cone
            G4double cosThetaMax = std::cos(coneHalfAngle);
            G4double cosTheta = cosThetaMax + (1.0 - cosThetaMax) * G4UniformRand();
            G4double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
            G4double phi = 2 * M_PI * G4UniformRand();
            
            G4ThreeVector randomDirection(sinTheta * std::cos(phi),
                                          sinTheta * std::sin(phi),
                                          cosTheta);

            // Rotate this randomly generated vector so that it is oriented along the coneAxis
            G4ThreeVector finalDirection = randomDirection.rotateUz(coneAxis);

            // Configure particle gun
            fParticleGun->SetParticleDefinition(particle);
            fParticleGun->SetParticlePosition(sourcePos);
            fParticleGun->SetParticleMomentumDirection(finalDirection);
            fParticleGun->SetParticleEnergy(energy);
            
            fParticleGun->GeneratePrimaryVertex(anEvent);
        }
    }



/***********************************************************************/


// Random source selection

    if (useMoireSourceRandomSource) {
            // 1. Define the position of the STL object in the world frame.
            G4ThreeVector stlPosition(-8.0 * cm, 3.5 * cm, 0.0 * cm);

            // 2. Define source center positions in the STL's *local* frame.
            std::vector<G4ThreeVector> localSourcePositions = {
                G4ThreeVector(0, 0, 0),       // Center of STL
                G4ThreeVector(0, 0, 50 * cm), // Offset along STL local +Z
                G4ThreeVector(0, 0, -50 * cm) // Offset along STL local -Z
            };

            // 3. Transform local positions to world coordinates.
            std::vector<G4ThreeVector> sourcePositions;
            for (const auto& localPos : localSourcePositions) {
                G4ThreeVector worldPos = localPos + stlPosition;
                sourcePositions.push_back(worldPos);
            }

            // Define the dimensions of the source volume (a thin box).
            G4double boxHalfX = 7.0 * cm / 2.0;
            G4double boxHalfY = 7.0 * cm / 2.0;
            G4double boxHalfZ = 250.0 * micrometer / 2.0;

            // Define the average centers of the LEFT and RIGHT detector arms for aiming.
            G4ThreeVector rightArmCenter(25.8 * cm, 0.0,  30.0 * cm);
            G4ThreeVector leftArmCenter (25.8 * cm, 0.0, -30.0 * cm);

            // Define the opening angle of the emission cone.
            G4double coneHalfAngle = 40.0 * deg;

            // Get the singleton instance of the G4AnalysisManager.
            auto analysisManager = G4AnalysisManager::Instance();
            auto particleTable = G4ParticleTable::GetParticleTable();

            //================================================================
            // PARTICLE GENERATION LOGIC
            //================================================================

            // --- KEY CHANGE: Select ONE source position randomly for this event ---
            // Instead of looping, we pick one source index (0, 1, or 2).
            // This ensures a balanced generation from all three sources over the entire run.
            G4int sourceID = static_cast<G4int>(G4UniformRand() * sourcePositions.size());
            const auto& sourceCenter = sourcePositions[sourceID];

            // Generate a random position *within* the chosen source box.
            G4double rx = (G4UniformRand() - 0.5) * (2.0 * boxHalfX);  
            G4double ry = (G4UniformRand() - 0.5) * (2.0 * boxHalfY);
            G4double rz = (G4UniformRand() - 0.5) * (2.0 * boxHalfZ);
            G4ThreeVector sourcePos = sourceCenter + G4ThreeVector(rx, ry, rz);

            int mySourceIDHist = analysisManager->GetH1Id("SourceIDHist");
            analysisManager->FillH1( mySourceIDHist , sourceID );

            // --- MODIFICATION: Fill ntuple with source vertex position AND the Source ID ---
            // We assume your ntuple ID is 2, and columns are X, Y, Z, SourceID.
            // The SourceID is crucial for creating color-coded plots later.
            analysisManager->FillNtupleDColumn(2, 0, sourcePos.x() / cm);
            analysisManager->FillNtupleDColumn(2, 1, sourcePos.y() / cm);
            analysisManager->FillNtupleDColumn(2, 2, sourcePos.z() / cm);
            analysisManager->FillNtupleIColumn(2, 3, sourceID); // Store which source it came from (0, 1, or 2)
            analysisManager->AddNtupleRow(2);

            // Fill the 3D histogram with the source position.
            auto h3id = analysisManager->GetH3Id("SourceXYZDistribution");
            analysisManager->FillH3(h3id, sourcePos.x() / cm, sourcePos.y() / cm, sourcePos.z() / cm);


            // Fill 1D histograms for X, Y, Z distributions.
            int zDistID = analysisManager->GetH1Id("ZDist");
            analysisManager->FillH1(zDistID, sourcePos.z()/cm);

            // Select particle type and energy.
            G4ParticleDefinition* particle;
            G4double energy;
            G4double rand = G4UniformRand();
            
            if (rand < 0.60) {
                particle = (G4UniformRand() < 0.5) ? particleTable->FindParticle("pi+") : particleTable->FindParticle("pi-");
                energy = 230 * MeV;
            }
            else if (rand < 0.80) {
                particle = particleTable->FindParticle("pi0");
                energy = 230 * MeV;
            }
            else if (rand < 0.96) {
                particle = (G4UniformRand() < 0.5) ? particleTable->FindParticle("kaon+") : particleTable->FindParticle("kaon-");
                energy = 150*MeV + 250*MeV*G4UniformRand();
            }
            else {
                // Skip this event if it's an eta meson or other unwanted particle.
                // Returning here means this event will have no primary particle.
                return; 
            }

            // --- AIMING LOGIC ---
            // Determine where to aim the cone based on which source was chosen.
            G4ThreeVector aimPoint;
            if (sourceCenter.z() > 4 * cm) { // This is the Z=+50 cm source (sourceID 1)
                aimPoint = rightArmCenter;
            } else if (sourceCenter.z() < -4 * cm) { // This is the Z=-50 cm source (sourceID 2)
                aimPoint = leftArmCenter; 
            } else { // This is the central Z=0 cm source (sourceID 0)
                // Randomly aim at either arm for symmetric illumination.
                aimPoint = (G4UniformRand() < 0.5) ? rightArmCenter : leftArmCenter;
            }

            // --- CONE EMISSION LOGIC ---
            // 1. Define the cone axis from the particle's actual origin to the aim point.
            G4ThreeVector coneAxis = (aimPoint - sourcePos).unit();

            // 2. Sample a random direction within the defined cone.
            G4double cosThetaMax = std::cos(coneHalfAngle);
            G4double cosTheta = cosThetaMax + (1.0 - cosThetaMax) * G4UniformRand();
            G4double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
            G4double phi = 2.0 * CLHEP::pi * G4UniformRand();
            
            G4ThreeVector randomDirection(sinTheta * std::cos(phi),
                                        sinTheta * std::sin(phi),
                                        cosTheta);

            // 3. Rotate this randomly generated vector so it's oriented along the coneAxis.
            G4ThreeVector finalDirection = randomDirection.rotateUz(coneAxis);

            // --- PARTICLE GUN CONFIGURATION ---
            fParticleGun->SetParticleDefinition(particle);
            fParticleGun->SetParticlePosition(sourcePos);
            fParticleGun->SetParticleMomentumDirection(finalDirection);
            fParticleGun->SetParticleEnergy(energy);
            
            // Generate the primary vertex for this event.
            fParticleGun->GeneratePrimaryVertex(anEvent);
        }



/***********************************************************************/

// Original one with cone toward average of both arms


    if (useMoireSourceUniform) {
        G4ThreeVector stlPosition(-8.0 * cm, 3.5 * cm, 0.0 * cm);

        
        // 2. Define source positions in the STL's *local* frame
        std::vector<G4ThreeVector> localSourcePositions = {
            G4ThreeVector(0, 0, 0),                  // Center of STL
            G4ThreeVector(0, 0, 45 * cm),          // Offset along STL local Z
            G4ThreeVector(0, 0, -45 * cm)          // Offset along STL local -Z
        };

        // 3. Transform local positions to world coordinates
        std::vector<G4ThreeVector> sourcePositions;
        for (const auto& localPos : localSourcePositions) {
            G4ThreeVector worldPos = localPos + stlPosition;
            sourcePositions.push_back(worldPos);
        }


        // Define the dimensions of the source box (using your physically correct thick source)
        G4double boxHalfX = 7.0 * cm / 2.0;
        G4double boxHalfY = 7.0 * cm / 2.0;
        G4double boxHalfZ = 250.0 * micrometer / 2.0;

        // 1. Define the average centers of the LEFT and RIGHT detector arms.
        G4ThreeVector rightArmCenter(25.8 * cm, 0.0,  30.0 * cm);
        G4ThreeVector leftArmCenter (25.8 * cm, 0.0, -30.0 * cm);

        // 2. Define the opening angle of the emission cone. This is a key parameter to tune.
        //    A larger angle ensures all detectors are illuminated, especially from the off-axis sources.
        G4double coneHalfAngle = 40 * deg;


        // --- MODIFICATION: Get the Analysis Manager ---
        // Get the singleton instance of the G4AnalysisManager
        auto analysisManager = G4AnalysisManager::Instance();
        int nPrimariesThisEvent = 0;

    // Loop over each of the three source locations
    for (const auto& sourceCenter : sourcePositions) {
        // Generate a random position *within* one of the source boxes
        G4double rx = (G4UniformRand() - 0.5) * (2.0 * boxHalfX);  
        G4double ry = (G4UniformRand() - 0.5) * (2.0 * boxHalfY);
        G4double rz = (G4UniformRand() - 0.5) * (2.0 * boxHalfZ);
        G4ThreeVector sourcePos = sourceCenter + G4ThreeVector(rx, ry, rz);

        // Select particle type and energy (this logic is unchanged)
        G4ParticleDefinition* particle;
        G4double energy;
        G4double rand = G4UniformRand();
        
        if (rand < 0.60) {
            particle = (G4UniformRand() < 0.5) ? piPlus : piMinus;
            energy = 230 * MeV;
        }
        else if (rand < 0.80) {
            particle = particleTable->FindParticle("pi0");
            energy = 230 * MeV;
        }
        else if (rand < 0.96) {
            particle = (G4UniformRand() < 0.5) ? kPlus : kMinus;
            energy = 150*MeV + 250*MeV*G4UniformRand();
        }
        else {
            continue; // Skip eta mesons
        }

        // Fill the ntuple with the source vertex position
        analysisManager->FillNtupleDColumn(2, 0, sourcePos.x() / cm);
        analysisManager->FillNtupleDColumn(2, 1, sourcePos.y() / cm);
        analysisManager->FillNtupleDColumn(2, 2, sourcePos.z() / cm);
        analysisManager->AddNtupleRow(2);

        auto h3id = analysisManager->GetH3Id("SourceXYZDistribution");
        analysisManager->FillH3(h3id, sourcePos.x() / cm, sourcePos.y() / cm, sourcePos.z() / cm);

        // --- THIS IS THE CORRECTED AIMING LOGIC ---
        G4ThreeVector aimPoint;
        if (sourceCenter.z() > 4 * cm) { // This is the Z=+50 cm source
            aimPoint = rightArmCenter;
        } else if (sourceCenter.z() < -4 * cm) { // This is the Z=-50 cm source
            aimPoint = leftArmCenter; // It now correctly aims at the left arm
        } else { // This is the Z=0 cm source
            // Randomly aim at either arm for symmetric illumination
            if (G4UniformRand() < 0.5) {
                aimPoint = rightArmCenter;
            } else {
                aimPoint = leftArmCenter;
            }
        }

        // --- REPLACED ISOTROPIC EMISSION WITH CONE EMISSION ---
        
        // 3. Define the cone axis: a vector from the particle's actual origin to the detector center.
        //    This must be calculated for each particle because `sourcePos` is randomized.
        G4ThreeVector coneAxis = (aimPoint - sourcePos).unit();

        // 4. Sample a random direction within the defined cone using the provided logic.
        G4double cosThetaMax = std::cos(coneHalfAngle);
        G4double cosTheta = cosThetaMax + (1.0 - cosThetaMax) * G4UniformRand();
        G4double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
        G4double phi = 2 * M_PI * G4UniformRand();
        
        G4ThreeVector randomDirection(sinTheta * std::cos(phi),
                                      sinTheta * std::sin(phi),
                                      cosTheta);

        // 5. Rotate this randomly generated vector so that it is oriented along the coneAxis.
        //    `rotateUz` finds the rotation that maps the Z-axis to the coneAxis and applies it.
        G4ThreeVector finalDirection = randomDirection.rotateUz(coneAxis);

        // Configure particle gun with the new directed momentum
        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticlePosition(sourcePos);
        fParticleGun->SetParticleMomentumDirection(finalDirection); // Use the new direction
        fParticleGun->SetParticleEnergy(energy);
        
        fParticleGun->GeneratePrimaryVertex(anEvent);
        nPrimariesThisEvent++;
        }
        int h1id = analysisManager->GetH1Id("PrimaryCount");
        analysisManager->FillH1(h1id, nPrimariesThisEvent);

}


/***********************************************************************/
    if (useMoireSourceGaussian) {
    G4ThreeVector stlPosition(-8.0 * cm, 3.5 * cm, 0.0 * cm);

    std::vector<G4ThreeVector> localSourcePositions = {
        G4ThreeVector(0, 0, 0),
        G4ThreeVector(0, 0, 45 * cm),
        G4ThreeVector(0, 0, -45 * cm)
    };

    std::vector<G4ThreeVector> sourcePositions;
    for (const auto& localPos : localSourcePositions) {
        G4ThreeVector worldPos = localPos + stlPosition;
        sourcePositions.push_back(worldPos);
    }

    // --- NEW PARAMETERS TO CONTROL THE GAUSSIAN BEAM SPOT ---
    // These are the parameters you can easily change.

    // 1. Center of the Gaussian spot (offset from the sourceCenter's X,Y).
    //    For example, to move the spot 1cm to the right (+X) and 0.5cm up (+Y).
    G4ThreeVector spotCenterOffset(1.0 * cm, 0.5 * cm, 0.0 * cm);

    // 2. The "width" (standard deviation, or sigma) of the Gaussian spot.
    //    About 68% of particles will be within +/- 1 sigma from the center.
    //    About 95% of particles will be within +/- 2 sigma.
    G4double spotSigmaX = 5.0 * mm;
    G4double spotSigmaY = 5.0 * mm;

    // The thickness of the source remains the same.
    G4double boxHalfZ = 250.0 * micrometer / 2.0;
    
    // (Detector and other definitions are unchanged)
    G4ThreeVector rightArmCenter(25.8 * cm, 0.0,  30.0 * cm);
    G4ThreeVector leftArmCenter (25.8 * cm, 0.0, -30.0 * cm);
    G4double coneHalfAngle = 40 * deg;

    auto analysisManager = G4AnalysisManager::Instance();


    // Loop over each of the three source locations
    for (const auto& sourceCenter : sourcePositions) {
        
        // --- MODIFICATION: GENERATE POSITION FROM A GAUSSIAN DISTRIBUTION ---

        // Generate random X and Y offsets from a Gaussian distribution.
        // G4RandGauss::shoot(mean, standard_deviation)
        G4double rx = G4RandGauss::shoot(spotCenterOffset.x(), spotSigmaX);
        G4double ry = G4RandGauss::shoot(spotCenterOffset.y(), spotSigmaY);

        // The Z position is still sampled uniformly within the source thickness.
        G4double rz = (G4UniformRand() - 0.5) * (2.0 * boxHalfZ);
        
        G4ThreeVector sourcePos = sourceCenter + G4ThreeVector(rx, ry, rz);

        // (The rest of the particle selection, aiming, and generation logic is unchanged)
        // ...
        
        // Select particle type and energy
        G4ParticleDefinition* particle;
        G4double energy;
        G4double rand = G4UniformRand();
        
        if (rand < 0.60) {
            particle = (G4UniformRand() < 0.5) ? piPlus : piMinus;
            energy = 230 * MeV;
        }
        else if (rand < 0.80) {
            particle = particleTable->FindParticle("pi0");
            energy = 230 * MeV;
        }
        else if (rand < 0.96) {
            particle = (G4UniformRand() < 0.5) ? kPlus : kMinus;
            energy = 150*MeV + 250*MeV*G4UniformRand();
        }
        else {
            continue; // Skip eta mesons
        }

        // (Analysis and aiming logic is unchanged)
        analysisManager->FillNtupleDColumn(2, 0, sourcePos.x() / cm);
        analysisManager->FillNtupleDColumn(2, 1, sourcePos.y() / cm);
        analysisManager->FillNtupleDColumn(2, 2, sourcePos.z() / cm);
        analysisManager->AddNtupleRow(2);

        auto h3id = analysisManager->GetH3Id("SourceXYZDistribution");
        analysisManager->FillH3(h3id, sourcePos.x() / cm, sourcePos.y() / cm, sourcePos.z() / cm);

        G4ThreeVector aimPoint;
        if (sourceCenter.z() > 4 * cm) {
            aimPoint = rightArmCenter;
        } else if (sourceCenter.z() < -4 * cm) {
            aimPoint = leftArmCenter;
        } else {
            if (G4UniformRand() < 0.5) {
                aimPoint = rightArmCenter;
            } else {
                aimPoint = leftArmCenter;
            }
        }
        
        G4ThreeVector coneAxis = (aimPoint - sourcePos).unit();
        G4double cosThetaMax = std::cos(coneHalfAngle);
        G4double cosTheta = cosThetaMax + (1.0 - cosThetaMax) * G4UniformRand();
        G4double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
        G4double phi = 2 * M_PI * G4UniformRand();
        G4ThreeVector randomDirection(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
        G4ThreeVector finalDirection = randomDirection.rotateUz(coneAxis);

        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticlePosition(sourcePos);
        fParticleGun->SetParticleMomentumDirection(finalDirection);
        fParticleGun->SetParticleEnergy(energy);
        
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }
}


/***********************************************************************/

if (useMoireSourceGaussianIsotropic) {
    G4ThreeVector stlPosition(-8.0 * cm, 3.5 * cm, 0.0 * cm);

    std::vector<G4ThreeVector> localSourcePositions = {
        G4ThreeVector(0, 0, 0),
        G4ThreeVector(0, 0, 45 * cm),
        G4ThreeVector(0, 0, -45 * cm)
    };

    std::vector<G4ThreeVector> sourcePositions;
    for (const auto& localPos : localSourcePositions) {
        G4ThreeVector worldPos = localPos + stlPosition;
        sourcePositions.push_back(worldPos);
    }

    // --- NEW PARAMETERS TO CONTROL THE GAUSSIAN BEAM SPOT ---
    G4ThreeVector spotCenterOffset(1.0 * cm, 0.5 * cm, 0.0 * cm);
    G4double spotSigmaX = 5.0 * mm;
    G4double spotSigmaY = 5.0 * mm;

    // The thickness of the source remains the same.
    G4double boxHalfZ = 250.0 * micrometer / 2.0;
    
    // (Detector and other definitions are unchanged, but we remove the aiming logic)
    // G4ThreeVector rightArmCenter(25.8 * cm, 0.0,  30.0 * cm); // REMOVED
    // G4ThreeVector leftArmCenter (25.8 * cm, 0.0, -30.0 * cm); // REMOVED
    // G4double coneHalfAngle = 40 * deg; // REMOVED

    auto analysisManager = G4AnalysisManager::Instance();


    // Loop over each of the three source locations
    for (const auto& sourceCenter : sourcePositions) {
        
        // --- MODIFICATION: GENERATE POSITION FROM A GAUSSIAN DISTRIBUTION ---

        G4double rx = G4RandGauss::shoot(spotCenterOffset.x(), spotSigmaX);
        G4double ry = G4RandGauss::shoot(spotCenterOffset.y(), spotSigmaY);
        G4double rz = (G4UniformRand() - 0.5) * (2.0 * boxHalfZ);
        
        G4ThreeVector sourcePos = sourceCenter + G4ThreeVector(rx, ry, rz);

        // Select particle type and energy
        G4ParticleDefinition* particle;
        G4double energy;
        G4double rand = G4UniformRand();
        
        if (rand < 0.60) {
            particle = (G4UniformRand() < 0.5) ? piPlus : piMinus;
            energy = 230 * MeV;
        }
        else if (rand < 0.80) {
            particle = particleTable->FindParticle("pi0");
            energy = 230 * MeV;
        }
        else if (rand < 0.96) {
            particle = (G4UniformRand() < 0.5) ? kPlus : kMinus;
            energy = 150*MeV + 250*MeV*G4UniformRand();
        }
        else {
            continue; // Skip eta mesons
        }

        // (Analysis and aiming logic is unchanged)
        analysisManager->FillNtupleDColumn(2, 0, sourcePos.x() / cm);
        analysisManager->FillNtupleDColumn(2, 1, sourcePos.y() / cm);
        analysisManager->FillNtupleDColumn(2, 2, sourcePos.z() / cm);
        analysisManager->AddNtupleRow(2);

        auto h3id = analysisManager->GetH3Id("SourceXYZDistribution");
        analysisManager->FillH3(h3id, sourcePos.x() / cm, sourcePos.y() / cm, sourcePos.z() / cm);

        // --- MODIFICATION: ISOTROPIC (4PI) EMISSION ---
        // Generate random direction on a unit sphere (isotropic)
        G4double cosTheta = 2.0 * G4UniformRand() - 1.0; // cos(theta) uniform in [-1, 1]
        G4double sinTheta = std::sqrt(1.0 - cosTheta * cosTheta);
        G4double phi = 2.0 * M_PI * G4UniformRand(); // phi uniform in [0, 2*pi]

        G4ThreeVector finalDirection(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
        // The previous cone/aiming logic is now completely replaced by the above

        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticlePosition(sourcePos);
        fParticleGun->SetParticleMomentumDirection(finalDirection); // Use the new isotropic direction
        fParticleGun->SetParticleEnergy(energy);
        
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }
}

/***********************************************************************/

    if (useTestSingleSource) {
        cout << "PrimaryGenerator: Using test single source emission" << endl;
        // Test source at a fixed position
        G4ThreeVector sourcePosition(0, 0, 0); // Center of the world volume
        fParticleGun->SetParticlePosition(sourcePosition);

        // Select particle type based on annihilation ratios
        G4ParticleDefinition* particle;
        G4double energy;
        G4double rand = G4UniformRand();
        
        if (rand < 0.60) { // Charged pions (60% of total)
            particle = (G4UniformRand() < 0.5) ? piPlus : piMinus;
            energy = 230 * MeV; // Average kinetic energy
        }
        else if (rand < 0.80) { // Neutral pions (40% of total)
            particle = particleTable->FindParticle("pi0");
            energy = 230 * MeV; // Same average energy as charged
        }
        else if (rand < 0.96) { // Charged kaons (16% of total)
            particle = (G4UniformRand() < 0.5) ? kPlus : kMinus;
            energy = 150*MeV + 250*MeV*G4UniformRand(); // Uniform distribution
        }
        else { // Eta mesons (4% - neutral, not detected)
            return; // Skip neutral particles for charged detection
        }

        // Set particle type and energy
        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticleEnergy(energy);

        fParticleGun->SetParticleMomentumDirection((G4ThreeVector(1, 0, 0)).unit()); // Forward direction
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }




/***********************************************************************/



    if (useConeSourceTowardFourModules) {
        cout << "PrimaryGenerator: Using cone emission toward four modules" << endl;
        // Module positions (same as geometry)
        std::vector<G4ThreeVector> moduleCenters = {
            G4ThreeVector(15.8*cm, 0,  30*cm),
            G4ThreeVector(25.8*cm, 0,  30*cm),
            G4ThreeVector(15.8*cm, 0, -30*cm),
            G4ThreeVector(25.8*cm, 0, -30*cm)
        };

        // Compute average module center for cone axis
        G4ThreeVector avgModuleCenter(0, 0, 0);
        for (const auto& module : moduleCenters) {
            avgModuleCenter += module;
        }
        avgModuleCenter /= moduleCenters.size();

        // Source at origin
        G4ThreeVector sourcePos(0, 0, 0);

        // Cone half angle (adjust as needed, e.g. 30 deg)
        G4double coneHalfAngle = 70.0 * deg;

        // Select particle type based on annihilation ratios
        G4ParticleDefinition* particle;
        G4double energy;
        G4double rand = G4UniformRand();

        if (rand < 0.60) {
            particle = (G4UniformRand() < 0.5) ? piPlus : piMinus;
            energy = 230 * MeV;
        } else if (rand < 0.80) {
            particle = particleTable->FindParticle("pi0");
            energy = 230 * MeV;
        } else if (rand < 0.96) {
            particle = (G4UniformRand() < 0.5) ? kPlus : kMinus;
            energy = 150*MeV + 250*MeV*G4UniformRand();
        } else {
            return; // skip neutral
        }

        // Cone axis: from source to avg module center
        G4ThreeVector coneAxis = (avgModuleCenter - sourcePos).unit();

        // Sample direction within cone
        G4double cosTheta = std::cos(coneHalfAngle);
        G4double randomCosTheta = cosTheta + (1 - cosTheta) * G4UniformRand();
        G4double sinTheta = std::sqrt(1 - randomCosTheta * randomCosTheta);
        G4double phi = 2 * M_PI * G4UniformRand();
        G4ThreeVector randomDirection(
            sinTheta * std::cos(phi),
            sinTheta * std::sin(phi),
            randomCosTheta);

        G4ThreeVector finalDirection = randomDirection.rotateUz(coneAxis);

        fParticleGun->SetParticleDefinition(particle);
        fParticleGun->SetParticleEnergy(energy);
        fParticleGun->SetParticlePosition(sourcePos);
        fParticleGun->SetParticleMomentumDirection(finalDirection.unit());
        fParticleGun->GeneratePrimaryVertex(anEvent);
    }


}
