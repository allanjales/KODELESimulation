#ifndef DetectorConstruction_hh
#define DetectorConstruction_hh

#include <G4VUserDetectorConstruction.hh>
#include <globals.hh>
#include <G4UserLimits.hh>
#include <G4FieldManager.hh>
#include <G4ElectricField.hh>
#include <G4PhysicalVolumeStore.hh>
#include <map>
#include "DetectorConstructionMessenger.hh"
#include "SensitiveDetector.hh"

#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>
#include <G4NistManager.hh>
#include <G4Box.hh>
#include <G4VisAttributes.hh>
#include <G4PVPlacement.hh>
#include <G4UniformElectricField.hh>
#include <G4TransportationManager.hh>
#include <G4EqMagElectricField.hh>
#include <G4ClassicalRK4.hh>

#include <G4MagIntegratorDriver.hh>
#include <G4ChordFinder.hh>

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
	DetectorConstruction();
	~DetectorConstruction();

	G4VPhysicalVolume* Construct();
	
	void ConstructSDandField();

	//const G4VPhysicalVolume* GetWorld() {return WorldPhysicalVolume;};
	//const G4VPhysicalVolume* GetGasRPC() {return GasPhysicalVolume;};
	void SetGasMaterial(G4Material* material);

	G4Material* C2H2F4Material;
	G4Material* SF6Material;
	G4Material* CO2Material;
	G4Material* iC4H10Material;
	G4Material* CMSMixtureMaterial;

	G4UserLimits* WorldUserLimits;

	DataHandler* dataHandler;
private:
	G4LogicalVolume*   WorldLogicalVolume;
	G4LogicalVolume*   CooperFoilLogicalVolume;
	G4LogicalVolume*   PolyethyleneLogicalVolume;
	G4LogicalVolume*   GraphiteLogicalVolume;
	G4LogicalVolume*   BakeliteLogicalVolume;
	G4LogicalVolume*   GasLogicalVolumePos;
	G4LogicalVolume*   GasLogicalVolumeNeg;
	G4LogicalVolume*   AirLogicalVolume;
	G4LogicalVolume*   CooperStripLogicalVolume;
	
	DetectorConstructionMessenger* detectorConstructionMessenger;

	SensitiveDetector* sensitiveDetector;

	struct Layer {
		G4LogicalVolume* logic;
		G4String name;
		G4double thickness;
	};

	void ConstructElectricField();
	void CreateChordFinder(G4FieldManager*, G4ElectricField*);
	void ConstructSensitiveDetector();
};

#endif