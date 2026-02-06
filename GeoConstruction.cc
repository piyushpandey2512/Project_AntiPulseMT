#include "GeoConstruction.hh"
#include "G4RotationMatrix.hh"
#include "G4VisAttributes.hh"
#include <G4Tubs.hh>
#include <G4Box.hh>
#include <vector>
#include "CADMesh.hh"
#include "G4PVReplica.hh"
#include "G4SDManager.hh"
#include "G4UserLimits.hh"


#include "G4UniformGravityField.hh"
#include "G4FieldManager.hh"
#include "G4TransportationManager.hh"
#include "G4ChordFinder.hh"
#include "G4PropagatorInField.hh"

using namespace std;

G4bool overlapCheck = true;


// Toggle these to select the setup you want
bool useSTLGeometry = true;
bool useFourModuleSetupNewFEE = true;
bool useTestScintillator = false;
bool useTestModulesSetup = false;
bool useTwoScinB2B = false;
bool useOneModule = false;
bool useTwoB2BModules = false;
bool useMoireGratingSetup = true;


MyDetectorConstruction::MyDetectorConstruction() {
    DefineCommands();
}

MyDetectorConstruction::~MyDetectorConstruction() {}

// Define the command
void MyDetectorConstruction::DefineCommands() {
    fMessenger = new G4GenericMessenger(this, "/detector/", "Detector control");
    fMessenger->DeclareMethodWithUnit("setGratingAngle", "deg", 
                                      &MyDetectorConstruction::SetGratingAngle, 
                                      "Set rotation of the first grating.");
}

// The Setter
void MyDetectorConstruction::SetGratingAngle(G4double angle) {
    fGratingAngle = angle;
    G4RunManager::GetRunManager()->ReinitializeGeometry(); // Trigger rebuild
}

G4VPhysicalVolume* MyDetectorConstruction::Construct()
{
    // --- World Volume ---
    wRadius = 300*cm;
    wLength = 530*cm;
    wName = "World";

    // Materials and colors
    DefineMaterials();

    G4VSolid* wSolid = new G4Tubs(wName, 0, wRadius, wLength/2, 0, 360);
    G4LogicalVolume* wLogic = new G4LogicalVolume(wSolid, Gal_mat, "World");
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0, G4ThreeVector(), wLogic, "World", 0, false, 0, overlapCheck);

/*****************************************************************************************************************************/

    // --- STL Geometry ---
    if (useSTLGeometry) {
        cout << "GeoConstruction: Using STL geometry" << endl;
        G4String stlFile = "/home/piyush/Desktop/PhD_Work/Trento_Project/Project_AntiPulse/stl_geometry/TotalGravModuleV2.stl";
        CADMesh *cadMesh = new CADMesh(const_cast<char*>(stlFile.c_str()));
        cadMesh->SetScale(1.0);
        cadMesh->SetReverse(false);

        G4VSolid *stlSolid = cadMesh->TessellatedMesh();
        // G4VisAttributes* gray = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
        // Change the color to a bright yellow
        G4VisAttributes* visAttributes = new G4VisAttributes(G4Colour(1.0, 1.0, 0.0));
        G4LogicalVolume* stlLogical = new G4LogicalVolume(stlSolid, mat304steel, "STLVolume");
        stlLogical->SetVisAttributes(visAttributes);

        G4RotationMatrix* rotation = new G4RotationMatrix();
        rotation->rotateX(90.0*deg);
        G4ThreeVector stlPosition(-8.0*cm, 3.5*cm, 8.0*cm);
        new G4PVPlacement(rotation, stlPosition, stlLogical, "STLVolume", wLogic, false, 0, overlapCheck);
    }


/*****************************************************************************************************************************/
    // --- Moiré Grating Setup ---
    if (useMoireGratingSetup) {
        cout << "GeoConstruction: Using 2 HORIZONTAL Moiré gratings + 1 Solid Counter setup" << endl;

        // --- 1. Define Dimensions (Unchanged) ---
        G4double grating_halfX = 7.0 * cm / 2.0;
        G4double grating_halfY = 7.0 * cm / 2.0;
        G4double grating_halfZ = 250.0 * micrometer / 2.0;

        // --- This section now builds a SINGLE reusable grating object ---
        // --- It is completely unchanged. ---
        G4double pitch         = 100.0 * micrometer;
        G4double opening_width = 40.0 * micrometer;
        G4double wall_width    = pitch - opening_width;

        // Empty container for the whole grating
        G4Box* gratingMother_box = new G4Box("GratingMotherBox", grating_halfX, grating_halfY, grating_halfZ);
        G4LogicalVolume* gratingMother_log = new G4LogicalVolume(gratingMother_box, Gal_mat, "GratingMotherLog");

        // Build one slice of the grating (one pitch) and replicate it
        G4Box* slice_box = new G4Box("SliceBox", grating_halfX, pitch / 2.0, grating_halfZ);
        G4LogicalVolume* slice_log = new G4LogicalVolume(slice_box, Gal_mat, "SliceLog");

        // Create the replicas along Y
        G4int num_replicas = G4int( (2.0 * grating_halfY) / pitch );
        new G4PVReplica("GratingReplica", slice_log, gratingMother_log, kYAxis, num_replicas, pitch);

        // Now build the wall and opening within the slice
        G4Box* wall_box = new G4Box("WallBox", grating_halfX, wall_width / 2.0, grating_halfZ);
        fGratingWallLogical = new G4LogicalVolume(wall_box, fSiMaterial, "WallLog");
        G4Box* opening_box = new G4Box("OpeningBox", grating_halfX, opening_width / 2.0, grating_halfZ);
        fGratingOpeningLogical = new G4LogicalVolume(opening_box, Gal_mat, "GratingOpeningLog");

        // Place the wall and opening within the slice
        // G4double wall_pos_y = -opening_width / 2.0;
        // G4double opening_pos_y = wall_width / 2.0;
        G4double opening_pos_y = -pitch/2.0 + opening_width/2.0; // Results in -30 um
        G4double wall_pos_y    =  pitch/2.0 - wall_width/2.0;    // Results in +20 um
        new G4PVPlacement(0, G4ThreeVector(0, wall_pos_y, 0), fGratingWallLogical, "Wall", slice_log, false, 0, true);
        new G4PVPlacement(0, G4ThreeVector(0, opening_pos_y, 0), fGratingOpeningLogical, "Opening", slice_log, false, 0, true);
        fGratingWallLogical->SetVisAttributes(new G4VisAttributes(G4Colour::Magenta()));
        fGratingOpeningLogical->SetVisAttributes(new G4VisAttributes(G4Colour::White()));

        // --- THE CRITICAL FIX: Set User Limits to force small steps ---
        
        // 1. Define a maximum allowed step size. It should be smaller than your
        //    smallest geometric feature (which is the 40 um opening).
        G4double maxStepInGrating = 4.0 * micrometer;
        G4UserLimits* gratingStepLimits = new G4UserLimits(maxStepInGrating);

        // 2. Attach these step limits to BOTH the walls and the openings.
        //    This is important because it forces small steps even when the particle
        //    is in the vacuum OPENING, ensuring it doesn't step over the next wall.
        fGratingWallLogical->SetUserLimits(gratingStepLimits);
        fGratingOpeningLogical->SetUserLimits(gratingStepLimits);

 // =======================================================================
        // --- MODIFICATION: Create a THICK Solid Counter Detector ---
        // We make it 10 cm thick along the Z-axis to act as a beam dump.
        G4double counter_halfZ = 5.0 * cm / 2.0;
        G4Box* solidCounter_box = new G4Box("SolidCounterBox", grating_halfX, grating_halfY, counter_halfZ);
        fSolidCounterLogical = new G4LogicalVolume(solidCounter_box, fSiMaterial, "SolidCounterLog");

        // --- MODIFICATION: Create new, SOLID visualization attributes for the counter ---
        G4VisAttributes* visAttCounter = new G4VisAttributes(G4Colour::Red());
        visAttCounter->SetForceSolid(true); // Make it solid
        fSolidCounterLogical->SetVisAttributes(visAttCounter);
        // =======================================================================

        // --- MODIFIED: Final Placement of the components ---
        G4ThreeVector stlPosition(0.1*cm, 0.0*cm, 0.55*cm);
        G4ThreeVector center_grating1 = stlPosition;
        G4ThreeVector center_grating2 = stlPosition + G4ThreeVector(0, 0, -45*cm);
        G4ThreeVector center_grating3 = stlPosition + G4ThreeVector(0, 0, 45*cm);

        // Adjust the counter's Z position to account for its new thickness
        G4ThreeVector center_counter  = stlPosition + G4ThreeVector(0, 0, 45*cm + counter_halfZ);


        G4RotationMatrix* rot = new G4RotationMatrix();
        rot->rotateZ(fGratingAngle);
        // rot->rotateZ(0.0 * mrad);
        // rot->rotateZ(0.25 * mrad);
        // rot->rotateZ(0.50 * mrad);
        // rot->rotateZ(0.75 * mrad);
        // rot->rotateZ(1.0 * mrad);
        // rot->rotateZ(1.25 * mrad);
        // rot->rotateZ(1.50 * mrad);
        // rot->rotateZ(1.75 * mrad);
        // rot->rotateZ(2.0 * mrad);
        // rot->rotateZ(2.0 * deg);

        // Place the first grating (at Z=0) with copy number 1
        new G4PVPlacement(rot, center_grating1, gratingMother_log, "MoireGrating", wLogic, false, 1, true);

        // Place the second grating (at Z=-45) with copy number 2
        new G4PVPlacement(0, center_grating2, gratingMother_log, "MoireGrating", wLogic, false, 2, true);

        // Place the third grating (at Z=+45) with copy number 3
        // new G4PVPlacement(0, center_grating3, gratingMother_log, "MoireGrating", wLogic, false, 3, true);
        
        // Place the SOLID COUNTER (at Z=+45) with copy number 3
        new G4PVPlacement(0, center_counter, fSolidCounterLogical, "SolidCounter", wLogic, false, 3, true);
    }

/*****************************************************************************************************************************/


    // --- Four Module Setup (Modified with new FEE) ---
    if (useFourModuleSetupNewFEE) {
        cout << "GeoConstruction: Using four module setup with new FEE" << endl;
        G4double scinHalfX = 2.5*cm / 2.0;
        // G4double scinHalfX = 2.5*cm; /// test for energy deposition with double width
        // G4double scinHalfX = 1.25*cm / 2.0; // test for energy deposition with half width
        G4double scinHalfY = 0.6*cm / 2.0;
        G4double scinHalfZ = 50.0*cm / 2.0;

        G4Box* scinBox = new G4Box("ScintillatorBox", scinHalfX, scinHalfY, scinHalfZ);
        fScintLogical = new G4LogicalVolume(scinBox, fScinMaterial, "ScintillatorLV");
        fScintLogical->SetVisAttributes(visAttScin);

        G4double gap = 0.1*cm;
        G4double fullScinY = 2*scinHalfY;
        G4double moduleTotalY = 13*(fullScinY + gap) - gap;
        G4double moduleHalfY = moduleTotalY/2.0;

        std::vector<G4ThreeVector> modulePositions = {
            G4ThreeVector(20.8*cm, 0, 30*cm),  
            G4ThreeVector(30.8*cm, 0, 30*cm),
            G4ThreeVector(20.8*cm, 0, -30*cm),
            G4ThreeVector(30.8*cm, 0, -30*cm)
        };

        for (size_t m = 0; m < modulePositions.size(); m++) {
            G4ThreeVector modCenter = modulePositions[m];
            for (G4int j = 0; j < 13; j++) {
                G4double localY = -moduleHalfY + scinHalfY + j * (fullScinY + gap);
                G4ThreeVector scintPos = modCenter + G4ThreeVector(0, localY, 0);
                new G4PVPlacement(0, scintPos, fScintLogical, "Scintillator", wLogic, false, m*100 + j, overlapCheck);
            }
        }
    }


/*****************************************************************************************************************************/
    // --- Single Test Scintillator ---
    if (useTestScintillator) {
        cout << "GeoConstruction: Using single test scintillator" << endl;
        G4double scinHalfX = 2.5*cm / 2.0;
        // G4double scinHalfX = 2.5*cm; // test for energy deposition with double width
        // G4double scinHalfX = 1.25*cm / 2.0; // test for energy deposition with half width
        G4double scinHalfY = 0.6*cm / 2.0;
        G4double scinHalfZ = 50.0*cm / 2.0;

        G4Box* oneScinBox = new G4Box("ScintillatorBox", scinHalfX, scinHalfY, scinHalfZ);
        oneScinLogical = new G4LogicalVolume(oneScinBox, fScinMaterial, "ScintillatorSingleLV");
        oneScinLogical->SetVisAttributes(visAttScin);

        G4ThreeVector oneScinPos1(10*cm, 0, 0);
        // G4ThreeVector oneScinPos2(20*cm, 0, 0);

        new G4PVPlacement(0, oneScinPos1, oneScinLogical, "OneScintillator1", wLogic, false, 0, overlapCheck);
        // new G4PVPlacement(0, oneScinPos2, oneScinLogical, "OneScintillator2", wLogic, false, 0, overlapCheck);
    }

/*****************************************************************************************************************************/

    if (useTestModulesSetup) {
        cout << "GeoConstruction: Using test modules setup" << endl;
        G4double scinHalfX = 2.5*cm / 2.0;
        // // G4double scinHalfX = 2.5*cm; /// test for energy deposition with double width
        // G4double scinHalfX = 1.25*cm / 2.0; // test for energy deposition with half width
        G4double scinHalfY = 0.6*cm / 2.0;
        G4double scinHalfZ = 50.0*cm / 2.0;

        G4Box* scinBox = new G4Box("ScintillatorBox", scinHalfX, scinHalfY, scinHalfZ);
        fScintLogical = new G4LogicalVolume(scinBox, fScinMaterial, "ScintillatorLV");
        fScintLogical->SetVisAttributes(visAttScin);

        G4double gap = 0.1*cm;
        G4double fullScinY = 2*scinHalfY;
        G4double moduleTotalY = 13*(fullScinY + gap) - gap;
        G4double moduleHalfY = moduleTotalY/2.0;

        std::vector<G4ThreeVector> modulePositions = {
            G4ThreeVector(10*cm, 0, 30*cm),  
            G4ThreeVector(20*cm, 0, 30*cm),
            G4ThreeVector(10*cm, 0, -30*cm),
            G4ThreeVector(20*cm, 0, -30*cm)
        };

            // Front modules at x = 17 cm, back modules at x = 32 cm (15 cm apart)
        // std::vector<G4ThreeVector> modulePositions = {
        //     G4ThreeVector(17*cm, 0,  30*cm), // front right
        //     G4ThreeVector(32*cm, 0,  30*cm), // back right
        //     G4ThreeVector(17*cm, 0, -30*cm), // front left
        //     G4ThreeVector(32*cm, 0, -30*cm)  // back left
        // };

        for (size_t m = 0; m < modulePositions.size(); m++) {
            G4ThreeVector modCenter = modulePositions[m];
            for (G4int j = 0; j < 13; j++) {
                G4double localY = -moduleHalfY + scinHalfY + j * (fullScinY + gap);
                G4ThreeVector scintPos = modCenter + G4ThreeVector(0, localY, 0);
                new G4PVPlacement(0, scintPos, fScintLogical, "Scintillator", wLogic, false, m*100 + j, overlapCheck);
            }
        }
    }

/*****************************************************************************************************************************/

    if (useTwoScinB2B) {
        cout << "GeoConstruction: Using two scintillators back-to-back" << endl;
        // G4double scinHalfX = 2.5*cm / 2.0;
        // G4double scinHalfX = 2.5*cm; /// test for energy deposition with double width
        G4double scinHalfX = 1.25*cm / 2.0; // test for energy deposition with half width
        G4double scinHalfY = 0.6*cm / 2.0;
        G4double scinHalfZ = 50.0*cm / 2.0;

        G4Box* scinBox = new G4Box("ScintillatorBox", scinHalfX, scinHalfY, scinHalfZ);
        fScintLogical = new G4LogicalVolume(scinBox, fScinMaterial, "ScintillatorLV");
        fScintLogical->SetVisAttributes(visAttScin);

        G4ThreeVector oneScinPos1(10*cm, 0, 0);
        G4ThreeVector oneScinPos2(20*cm, 0, 0);

        new G4PVPlacement(0, oneScinPos1, fScintLogical, "OneScintillator1", wLogic, false, 0, overlapCheck);
        new G4PVPlacement(0, oneScinPos2, fScintLogical, "OneScintillator2", wLogic, false, 0, overlapCheck);

    }

    /*****************************************************************************************************************************/

    if (useOneModule) {
        cout << "GeoConstruction: Using one module" << endl;
        G4double scinHalfX = 2.5*cm / 2.0;
        // G4double scinHalfX = 2.5*cm; /// test for energy deposition with double width
        // G4double scinHalfX = 1.25*cm / 2.0; // test for energy deposition with half width
        G4double scinHalfY = 0.6*cm / 2.0;
        G4double scinHalfZ = 50.0*cm / 2.0;
        
        G4Box* scinBox = new G4Box("ScintillatorBox", scinHalfX, scinHalfY, scinHalfZ);
        fScintLogical = new G4LogicalVolume(scinBox, fScinMaterial, "ScintillatorLV");
        fScintLogical->SetVisAttributes(visAttScin);

        
        G4double gap = 0.1*cm;
        G4double fullScinY = 2*scinHalfY;
        G4double moduleTotalY = 13*(fullScinY + gap) - gap;
        G4double moduleHalfY = moduleTotalY/2.0;

        std::vector<G4ThreeVector> modulePositions = {
        G4ThreeVector(11.25*cm, 0, 0)
        };


        for (size_t m = 0; m < modulePositions.size(); m++) {
        G4ThreeVector modCenter = modulePositions[m];
        for (G4int j = 0; j < 13; j++) {
            G4double localY = -moduleHalfY + scinHalfY + j * (fullScinY + gap);
            G4ThreeVector scintPos = modCenter + G4ThreeVector(0, localY, 0);
            new G4PVPlacement(0, scintPos, fScintLogical, "Scintillator", wLogic, false, m*100 + j, overlapCheck);
        }
    }

    }

    /*****************************************************************************************************************************/

    if (useTwoB2BModules){
        cout << "GeoConstruction: Using two modules back-to-back" << endl;

        // G4double scinHalfX = 2.5*cm / 2.0;
        // G4double scinHalfX = 2.5*cm; /// test for energy deposition with double width
        G4double scinHalfX = 1.25*cm / 2.0; // test for energy deposition with half width
        G4double scinHalfY = 0.6*cm / 2.0;
        G4double scinHalfZ = 50.0*cm / 2.0;

                G4Box* scinBox = new G4Box("ScintillatorBox", scinHalfX, scinHalfY, scinHalfZ);
        fScintLogical = new G4LogicalVolume(scinBox, fScinMaterial, "ScintillatorLV");
        fScintLogical->SetVisAttributes(visAttScin);

        
        G4double gap = 0.1*cm;
        G4double fullScinY = 2*scinHalfY;
        G4double moduleTotalY = 13*(fullScinY + gap) - gap;
        G4double moduleHalfY = moduleTotalY/2.0;

        std::vector<G4ThreeVector> modulePositions = {
        G4ThreeVector(11.25*cm, 0, 0),
        G4ThreeVector(21.25*cm, 0, 0) // second module at x = 21.25 cm
        };


        for (size_t m = 0; m < modulePositions.size(); m++) {
        G4ThreeVector modCenter = modulePositions[m];
        for (G4int j = 0; j < 13; j++) {
            G4double localY = -moduleHalfY + scinHalfY + j * (fullScinY + gap);
            G4ThreeVector scintPos = modCenter + G4ThreeVector(0, localY, 0);
            new G4PVPlacement(0, scintPos, fScintLogical, "Scintillator", wLogic, false, m*100 + j, overlapCheck);
        }
    }
}

    return physWorld;
}


void MyDetectorConstruction::DefineMaterials()
{
    // --- Material and Element Definitions ---
    G4NistManager *nist = G4NistManager::Instance();

    // Elements
    G4Element* elH  = nist->FindOrBuildElement("H");
    G4Element* elC  = nist->FindOrBuildElement("C");
    G4Element* elO  = nist->FindOrBuildElement("O");
    G4Element* elSi = nist->FindOrBuildElement("Si");
    G4Element* elMn = nist->FindOrBuildElement("Mn");
    G4Element* elCr = nist->FindOrBuildElement("Cr");
    G4Element* elNi = nist->FindOrBuildElement("Ni");
    G4Element* elFe = nist->FindOrBuildElement("Fe");

    // Materials
    G4Material *volMatWorld = nist->FindOrBuildMaterial("G4_AIR");
    Gal_mat     = nist->FindOrBuildMaterial("G4_Galactic");

    // Define Stainless steel 304
    G4double density = 7.999*g/cm3;
    mat304steel = new G4Material("Stainless steel 304", density, 6);
    mat304steel->AddElement(elMn, 0.02);
    mat304steel->AddElement(elSi, 0.01);
    mat304steel->AddElement(elCr, 0.19);
    mat304steel->AddElement(elNi, 0.10);
    mat304steel->AddElement(elFe, 0.6792);
    mat304steel->AddElement(elC, 0.0008);

    // Define plastic scintillator material (vinyl-toluene)
    fScinMaterial = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");    

    // Colors for visualization
    G4VisAttributes* blue = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0));
    visAttScin = new G4VisAttributes(G4Colour(0.105, 0.210, 0.810));
    visAttScin->SetForceWireframe(true);
    visAttScin->SetForceSolid(true);

    // Silicon material for the grating walls
    fSiMaterial = new G4Material("Silicon", 2.3290*g/cm3, 1);
    fSiMaterial->AddElement(elSi, 1);
}



void MyDetectorConstruction::ConstructSDandField()
{
    // =======================================================================
    // --- MODIFICATION: Use ONE Sensitive Detector for EVERYTHING ---
    // Create one single instance of MySensitiveDetector
    MySensitiveDetector* masterSD = new MySensitiveDetector("MasterSD");
    G4SDManager::GetSDMpointer()->AddNewDetector(masterSD);

    // Assign this single instance to all logical volumes that should be sensitive.
    if (fScintLogical != nullptr) SetSensitiveDetector(fScintLogical, masterSD);
    if (oneScinLogical != nullptr) SetSensitiveDetector(oneScinLogical, masterSD);
    if (fGratingWallLogical != nullptr) SetSensitiveDetector(fGratingWallLogical, masterSD);
    if (fGratingOpeningLogical != nullptr) SetSensitiveDetector(fGratingOpeningLogical, masterSD);
    if (fSolidCounterLogical != nullptr) SetSensitiveDetector(fSolidCounterLogical, masterSD);
    // =======================================================================


    // // 1. Define the gravity field
    // G4UniformGravityField* gravityField = new G4UniformGravityField(G4ThreeVector(0, -9.81*m/s/s, 0));

    // // 2. Get the Global Field Manager
    // G4FieldManager* fieldMgr = G4TransportationManager::GetTransportationManager()->GetFieldManager();
    // fieldMgr->SetDetectorField(gravityField);

    // // 3. Set accuracy parameters (no ChordFinder needed for uniform gravity)
    // fieldMgr->SetDeltaIntersection(0.1*mm);
    // fieldMgr->SetDeltaOneStep(0.1*mm);
    

}