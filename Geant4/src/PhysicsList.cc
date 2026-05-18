#include "PhysicsList.hh"

#include <G4PAIModel.hh>
#include <G4PAIPhotModel.hh>
#include <G4EmConfigurator.hh>
#include <G4LossTableManager.hh>
#include <G4ProcessManager.hh>
#include <G4ParticleTable.hh>
#include <G4Gamma.hh>
#include <G4Electron.hh>
#include <G4Positron.hh>
#include <G4MuonPlus.hh>
#include <G4MuonMinus.hh>
#include <G4Proton.hh>
#include <G4GenericIon.hh>
#include <G4Region.hh>
#include <G4RegionStore.hh>

PhysicsList::PhysicsList()
{
	defaultCutValue = 0.001*mm;
	SetVerboseLevel(1);

	// Most accurate EM models and settings
	RegisterPhysics(new G4EmStandardPhysics_option4());

	// Hadronic physics
	RegisterPhysics(new G4HadronPhysicsQGSP_BERT_HP());

	RegisterPhysics(new G4IonElasticPhysics());
	RegisterPhysics(new G4DecayPhysics());
	
	RegisterPhysics(new G4StepLimiterPhysics());
}

PhysicsList::~PhysicsList() {}

void PhysicsList::SetCuts()
{
	// Set the range cuts for secondary production 
	// low energy limit on particle production
	if (verboseLevel > 0)
	{
		G4cout << "\nPhysicsList::SetCuts:";
		G4cout << "\n> CutLength : " << G4BestUnit(defaultCutValue, "Length") << "\n\n";
	}

	SetCutValue(defaultCutValue, "gamma");
	SetCutValue(defaultCutValue, "e-");
	SetCutValue(defaultCutValue, "e+");

	// AddPAIModel("PAI");
	AddPAIModel("PAIPhot");
}

void PhysicsList::AddPAIModel(const G4String& modname)
{
	G4EmConfigurator* config = G4LossTableManager::Instance()->EmConfigurator();

	const G4String regionName1 = "GasGapNeg";
	const G4String regionName2 = "GasGapPos";

	//electrons and positrons
	auto applyPAI = [&](const G4String& particle) {
		if (modname == "PAIPhot") {
			G4PAIPhotModel* model = new G4PAIPhotModel(nullptr, "PAIPhot");
			config->SetExtraEmModel(particle, "eBrem",   model, regionName1, 0.0, 1e+8*MeV, model);
			config->SetExtraEmModel(particle, "eBrem",   model, regionName2, 0.0, 1e+8*MeV, model);
			G4PAIPhotModel* model2 = new G4PAIPhotModel(nullptr, "PAIPhot");
			config->SetExtraEmModel(particle, "eIoni",   model2, regionName1, 0.0, 1e+8*MeV, model2);
			config->SetExtraEmModel(particle, "eIoni",   model2, regionName2, 0.0, 1e+8*MeV, model2);
		} else {
			G4PAIModel* model = new G4PAIModel(nullptr, "PAI");
			config->SetExtraEmModel(particle, "eIoni",   model, regionName1, 0.0, 1e+8*MeV, model);
			config->SetExtraEmModel(particle, "eIoni",   model, regionName2, 0.0, 1e+8*MeV, model);
		}
	};

	applyPAI("e-");
	applyPAI("e+");

	// protons and ions
	if (modname == "PAIPhot") {
		G4PAIPhotModel* mP = new G4PAIPhotModel(nullptr, "PAIPhot");
		config->SetExtraEmModel("proton", "hIoni", mP, regionName1, 0.0, 1e+8*MeV, mP);
		config->SetExtraEmModel("proton", "hIoni", mP, regionName2, 0.0, 1e+8*MeV, mP);
	} else {
		G4PAIModel* mP = new G4PAIModel(nullptr, "PAI");
		config->SetExtraEmModel("proton", "hIoni", mP, regionName1, 0.0, 1e+8*MeV, mP);
		config->SetExtraEmModel("proton", "hIoni", mP, regionName2, 0.0, 1e+8*MeV, mP);
	}
}