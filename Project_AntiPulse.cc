#include <iostream>

#include "G4AnalysisManager.hh"
#include "G4HadronPhysicsFTFP_BERT.hh"
#include "G4PhysListFactory.hh"
#include "G4RunManager.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VisExecutive.hh"
#include "G4VisManager.hh"
#include "GeoConstruction.hh"
#include "PhysicsList.hh"
#include "UserActions.hh"

// #include "PrimaryGeneratorAction.hh"

#include "G4MTRunManager.hh"

int main(int argc, char **argv) {

  std::cout << "argc = " << argc << std::endl;
  for (int i = 0; i < argc; ++i) {
    std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
  }
  G4MTRunManager *runManager = new G4MTRunManager();
  runManager->SetNumberOfThreads(14);
  runManager->SetUserInitialization(new MyDetectorConstruction());

  // Create the physics list using G4PhysListFactory

  G4int verbose = 1;
  G4PhysListFactory factory;
  G4VModularPhysicsList *physlist =
      factory.GetReferencePhysList("FTFP_INCLXX_EMZ");
  // G4VModularPhysicsList* physlist =
  // factory.GetReferencePhysList("FTFP_BERT_HP");

  physlist->RegisterPhysics(new G4StepLimiterPhysics());
  physlist->SetVerboseLevel(verbose);
  // physlist->RegisterPhysics(new G4HadronPhysicsFTFP_BERT());
  runManager->SetUserInitialization(physlist);

  // runManager->SetUserInitialization(new MyPhysicsList());
  G4String outputFileName = "Project_AntiPulse";
  runManager->SetUserInitialization(new MyAction(outputFileName));
  runManager->Initialize();

  // User Interface manager
  G4UImanager *UImanager = G4UImanager::GetUIpointer();
  UImanager->ApplyCommand("/control/verbose 1");
  UImanager->ApplyCommand("/run/verbose 1");
  UImanager->ApplyCommand("/event/verbose 0");
  UImanager->ApplyCommand("/tracking/verbose 0");

  // Set visualization

  G4UIExecutive *ui = 0;
  if (argc == 1) {
    ui = new G4UIExecutive(argc, argv);
  }

  G4VisManager *visManager = new G4VisExecutive();
  visManager->Initialize();

  if (ui) {
    UImanager->ApplyCommand("/control/execute vis.mac");
    ui->SessionStart();
    delete ui; // Clean up UI. If you want to use visuals, comment this out
  }

  else {
    G4String command = "/control/execute ";
    G4String filename = argv[1];
    G4cout << "Executing macro file: " << filename << G4endl;
    UImanager->ApplyCommand(command + filename);
  }

  return 0;
}

// new change

// ********To get the visualization and geant4 interface, comment out the part
// of codes mentioned below******** G4UIExecutive *ui = 0; if (argc == 1)
// 	{
// 	 ui = new G4UIExecutive(argc, argv);
// 	}

// G4VisManager *visManager = new G4VisExecutive();
// visManager->Initialize();

// if(ui)
// {
// UImanager->ApplyCommand("/control/execute vis.mac");
// ui->SessionStart();
// delete ui;  // Clean up UI. If you want to use visuals, comment this out
// }
