#ifndef GeoConstruction_hh
#define GeoConstruction_hh

#include "G4NistManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SystemOfUnits.hh"
#include "G4GenericMessenger.hh"
#include "SensitiveDet.hh"

class MyDetectorConstruction : public G4VUserDetectorConstruction
{
 public:	
	MyDetectorConstruction();
	~MyDetectorConstruction();

	virtual G4VPhysicalVolume *Construct();

	const std::vector<G4ThreeVector>& GetModulePositions() const { return modulePositions; }
    G4double GetModuleHalfY() const { return moduleHalfY; }
    G4double GetScinHalfY() const { return scinHalfY; }
    G4double GetFullScinY() const { return fullScinY; }
    G4double GetGap() const { return gap; }
    const std::vector<G4ThreeVector>& GetSources() const { return sources; }
	void SetGratingAngle(G4double angle);

private:
	G4LogicalVolume* logicBGO;
	G4LogicalVolume* fScinLog = nullptr;
	G4LogicalVolume* fScinLogInModule = nullptr;
	G4LogicalVolume* fScinLogInModule_2 = nullptr;
	G4LogicalVolume* l5TTrapLogic = nullptr;
	G4LogicalVolume* moLogic = nullptr;
	G4LogicalVolume* fScintLogical=nullptr;
	G4Material* fSiMaterial = nullptr;
	G4LogicalVolume* fGratingWallLogical = nullptr;
	G4LogicalVolume* fSolidCounterLogical = nullptr;
	

	G4LogicalVolume* fGratingOpeningLogical = nullptr;
 
	std::vector<G4ThreeVector> modulePositions;
    std::vector<G4ThreeVector> sources;
    G4double moduleHalfY;
    G4double scinHalfY;
    G4double fullScinY;
    G4double gap;

	G4LogicalVolume* fWorldLogical = nullptr;
	virtual void ConstructSDandField();	
	G4Material* Gal_mat = nullptr;

	void DefineMaterials();
	G4Material *mat304steel;


	G4Material* fScinMaterial = nullptr;

	G4double wRadius;
	G4double wLength;
	G4String wName;
	G4GenericMessenger* fMessenger = nullptr;

	G4Box *oneScinBox, *scinBox;
	G4VSolid* wSolid;
	G4VPhysicalVolume* physWorld;
	G4LogicalVolume *oneScinLogical, *scinLogical, *wLogic;
	G4bool isFourModuleSetup, isTestScintillator, isSTLGeometry;

	G4VisAttributes* visAttScin = nullptr;
	G4VisAttributes* visAttScinInModule = nullptr;
    G4double fGratingAngle = 0.0; // Variable to control rotation
    void DefineCommands();

};

#endif
