#include "SensitiveDetector.hh"

SensitiveDetector::SensitiveDetector(G4String name, DataHandler* dataHandler)
: G4VSensitiveDetector(name), dataHandler(dataHandler)
{}

SensitiveDetector::~SensitiveDetector()
{}

G4bool SensitiveDetector::ProcessHits(G4Step *aStep, G4TouchableHistory *ROhist)
{
	G4ParticleDefinition* particle = aStep->GetTrack()->GetDefinition();
	if (aStep->GetTrack()->GetDefinition() == G4Electron::Electron())
	{
		auto touchable = aStep->GetPreStepPoint()->GetTouchableHandle();
		G4String volumeName = touchable->GetVolume()->GetName();

		int regionID = -1;
		
		if (volumeName == "GasGapPos")
			regionID = 0;
		else if (volumeName == "GasGapNeg")
			regionID = 1;

		dataHandler->FillData(aStep, regionID);
		aStep->GetTrack()->SetTrackStatus(fStopAndKill);
	}

	return true;
}