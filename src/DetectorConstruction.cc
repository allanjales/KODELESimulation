#include "DetectorConstruction.hh"

DetectorConstruction::DetectorConstruction()
{
	detectorConstructionMessenger = new DetectorConstructionMessenger(this);

	dataHandler = new DataHandler();
	sensitiveDetector = new SensitiveDetector("SensitiveDetector", dataHandler);

	// Step Limiter for each material
	WorldUserLimits = new G4UserLimits();
}

DetectorConstruction::~DetectorConstruction()
{
	delete WorldUserLimits;

	delete detectorConstructionMessenger;

	delete sensitiveDetector;
}

G4VPhysicalVolume *DetectorConstruction::Construct()
{
	// ----------------
	// Materials
	// ----------------

	G4NistManager* nist = G4NistManager::Instance();

	G4Element* H = nist->FindOrBuildElement("H");
	G4Element* C = nist->FindOrBuildElement("C");
	G4Element* F = nist->FindOrBuildElement("F");
	G4Element* S = nist->FindOrBuildElement("S");
	G4Element* O = nist->FindOrBuildElement("O");

	// C2H2F4 Gas

	C2H2F4Material = new G4Material("C2H2F4", 4.25*kg/m3, 3, kStateGas, 295.*kelvin, 0.00168*bar);
	C2H2F4Material->AddElement(C, 2);
	C2H2F4Material->AddElement(H, 2);
	C2H2F4Material->AddElement(F, 4);

	// SF6 Gas

	SF6Material = new G4Material("SF6", 6.17*kg/m3, 2, kStateGas, 295.*kelvin, 0.00168*bar);
	SF6Material->AddElement(S, 1);
	SF6Material->AddElement(F, 6);

	// SF6 Gas

	CO2Material = new G4Material("CO2", 1.98*kg/m3, 2, kStateGas, 295.*kelvin, 0.00168*bar);
	CO2Material->AddElement(C, 1);
	CO2Material->AddElement(O, 2);

	// iC4H10 Gas

	iC4H10Material = new G4Material("iC4H10", 2.48*kg/m3, 2, kStateGas, 295.*kelvin, 0.00168*bar);
	iC4H10Material->AddElement(C, 4);
	iC4H10Material->AddElement(H, 10);

	// CMS Mixture
	CMSMixtureMaterial = new G4Material("CMSMixture", 4.25*kg/m3, 3, kStateGas, 295.*kelvin, 0.00168*bar);
	CMSMixtureMaterial->AddMaterial(C2H2F4Material, 95.2*perCent);
	CMSMixtureMaterial->AddMaterial(iC4H10Material,  4.5*perCent);
	CMSMixtureMaterial->AddMaterial(SF6Material,     0.3*perCent);


	// Aluminium Plate
	G4Material* AluminiumMaterial = nist->FindOrBuildMaterial("G4_Al");

	// Vacuum, Bakelite, Graphyte, Polyethylene
	G4Material* VacuumMaterial = nist->FindOrBuildMaterial("G4_Galactic");
	G4Material* AirMaterial = nist->FindOrBuildMaterial("G4_AIR");
	G4Material* BakeliteMaterial = nist->FindOrBuildMaterial("G4_BAKELITE");
	G4Material* GraphiteMaterial = nist->FindOrBuildMaterial("G4_GRAPHITE");
	G4Material* PolyethyleneMaterial = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
	G4Material* CooperMaterial = nist->FindOrBuildMaterial("G4_Cu");

	// ----------------
	// Setup
	// ----------------

	double width = 50*cm, height = 50*cm;

	// World
	auto WorldSolidVolume = new G4Box("WorldSolidVolume", width/2 + .5*cm, width/2 + .5*cm, 1*cm);
	WorldLogicalVolume = new G4LogicalVolume(WorldSolidVolume, VacuumMaterial, "WorldLogicalVolume", 0, 0, 0);
	{
		auto attr = new G4VisAttributes(G4Colour(1.,1.,1.));
		attr->SetForceWireframe();
		WorldLogicalVolume->SetVisAttributes(attr);
	}
	WorldLogicalVolume->SetUserLimits(WorldUserLimits);
	
	// Cooper Foil
	auto CooperFoilSolidVolume = new G4Box("CooperFoilSolidVolume", width/2, height/2, 0.015*mm);
	CooperFoilLogicalVolume = new G4LogicalVolume(CooperFoilSolidVolume, CooperMaterial, "CooperFoilLogicalVolume", 0, 0, 0);
	{
		auto attr = new G4VisAttributes(G4Colour(.0,.5,0.));
		attr->SetForceSolid();
		CooperFoilLogicalVolume->SetVisAttributes(attr);
	}

	// Polyethylene
	auto PolyethyleneSolidVolume = new G4Box("PolyethyleneSolidVolume", width/2, height/2, 0.1*mm);
	PolyethyleneLogicalVolume = new G4LogicalVolume(PolyethyleneSolidVolume, PolyethyleneMaterial, "PolyethyleneLogicalVolume", 0, 0, 0);
	{
		auto attr = new G4VisAttributes(G4Colour(.3,.7,.8));
		attr->SetForceSolid();
		PolyethyleneLogicalVolume->SetVisAttributes(attr);
	}

	// Graphites
	auto GraphiteSolidVolume = new G4Box("GraphiteSolidVolume", width/2, height/2, 0.1*mm);
	GraphiteLogicalVolume = new G4LogicalVolume(GraphiteSolidVolume, GraphiteMaterial, "GraphiteLogicalVolume", 0, 0, 0);
	{
		auto attr = new G4VisAttributes(G4Colour(.5,.5,.5));
		attr->SetForceSolid();
		GraphiteLogicalVolume->SetVisAttributes(attr);
	}

	// Bakelites
	auto BakeliteSolidVolume = new G4Box("BakeliteSolidVolume", width/2, height/2, 0.7*mm);
	BakeliteLogicalVolume = new G4LogicalVolume(BakeliteSolidVolume, BakeliteMaterial, "BakeliteLogicalVolume", 0, 0, 0);
	{
		auto attr = new G4VisAttributes(G4Colour(1.,0.,0.));
		attr->SetForceSolid();
		BakeliteLogicalVolume->SetVisAttributes(attr);
	}

	// Gas
	auto GasSolidVolume = new G4Box("GasSolidVolume", width/2, height/2, 0.7*mm);
	GasLogicalVolumePos = new G4LogicalVolume(GasSolidVolume, C2H2F4Material, "GasLogicalVolumePos", 0, 0, 0);
	GasLogicalVolumeNeg = new G4LogicalVolume(GasSolidVolume, C2H2F4Material, "GasLogicalVolumeNeg", 0, 0, 0);
	{
		auto attr = new G4VisAttributes(G4Colour(0.,1.,1.));
		attr->SetForceWireframe();
		GasLogicalVolumePos->SetVisAttributes(attr);
		GasLogicalVolumeNeg->SetVisAttributes(attr);
	}

	// Air
	auto AirSolidVolume = new G4Box("AirSolidVolume", width/2, height/2, 0.5*mm);
	AirLogicalVolume = new G4LogicalVolume(AirSolidVolume, AirMaterial, "AirLogicalVolume", 0, 0, 0);
	{
		auto attr = new G4VisAttributes(G4Colour(.0,.5,.5));
		attr->SetForceWireframe();
		AirLogicalVolume->SetVisAttributes(attr);
	}
	
	// Cooper Strips
	double stripPitch = 3*cm;
	auto CooperStripSolidVolume = new G4Box("CooperStripSolidVolume", width/2, stripPitch/2, 0.5*mm);
	CooperStripLogicalVolume = new G4LogicalVolume(CooperStripSolidVolume, CooperMaterial, "CooperStripLogicalVolume", 0, 0, 0);
	{
		auto attr = new G4VisAttributes(G4Colour(.9,.9,.1));
		attr->SetForceSolid();
		CooperStripLogicalVolume->SetVisAttributes(attr);
	}	

	// ----------------
	// Placing
	// ----------------

	// Place World
	G4VPhysicalVolume* WorldPhysicalVolume = new G4PVPlacement(0, G4ThreeVector(), WorldLogicalVolume, "WorldPhysicalVolume", 0, false, 0); 

	// Layers of the detector
	std::vector<Layer> detectorStack = {
		{CooperFoilLogicalVolume,   "CooperFoil",   0.03*mm},
		{PolyethyleneLogicalVolume, "Polyethylene", 0.2*mm},
		{GraphiteLogicalVolume,     "Graphite",     0.2*mm},
		{BakeliteLogicalVolume,     "Bakelite",     1.4*mm},
		{GasLogicalVolumePos,       "GasGapPos",    1.4*mm},
		{BakeliteLogicalVolume,     "Bakelite",     1.4*mm},
		{GraphiteLogicalVolume,     "Graphite",     0.2*mm},
		{PolyethyleneLogicalVolume, "Polyethylene", 0.2*mm},
		{CooperFoilLogicalVolume,   "CooperFoil",   0.03*mm},
		{AirLogicalVolume,          "Air"       ,   1.0*mm},
		{CooperFoilLogicalVolume,   "CooperFoil",   0.03*mm},
		{PolyethyleneLogicalVolume, "Polyethylene", 0.2*mm},
		{GraphiteLogicalVolume,     "Graphite",     0.2*mm},
		{BakeliteLogicalVolume,     "Bakelite",     1.4*mm},
		{GasLogicalVolumeNeg,       "GasGapNeg",    1.4*mm},
		{BakeliteLogicalVolume,     "Bakelite",     1.4*mm},
		{GraphiteLogicalVolume,     "Graphite",     0.2*mm},
		{PolyethyleneLogicalVolume, "Polyethylene", 0.2*mm},
		{CooperFoilLogicalVolume,   "CooperFoil",   0.03*mm}
	};
	
	// Get total thickness of the detector
	G4double currentZ = 0;
	for (auto& layer : detectorStack)
		currentZ += layer.thickness;
	currentZ /= -2.;

	// Place parts of the detector
	std::map<G4LogicalVolume*, G4int> copyCounters;
	for (auto& layer : detectorStack)
	{
		G4double posZ = currentZ + layer.thickness / 2.0;
		G4int currentCopy = copyCounters[layer.logic]++; 

		new G4PVPlacement(0, G4ThreeVector(0, 0, posZ), layer.logic, layer.name, WorldLogicalVolume, false, currentCopy, true);
		currentZ += layer.thickness;
	}

	// Place Cooper Strips
	G4VPhysicalVolume* AirGap = G4PhysicalVolumeStore::GetInstance()->GetVolume("Air");
	int stripsCount = 16;
	for (int i = 0; i < stripsCount; i++)
	{
		double posY = -height/2 + stripPitch/2 + i/(double)(stripsCount-1)*(height-stripPitch);
		new G4PVPlacement(0, G4ThreeVector(0, posY, 0), CooperStripLogicalVolume, "CooperStrip", AirGap->GetLogicalVolume(), true, i, true);
	}

	return WorldPhysicalVolume;
}

void DetectorConstruction::ConstructSDandField()
{
	ConstructElectricField();
	ConstructSensitiveDetector();
}

void DetectorConstruction::ConstructElectricField()
{
	// Set global eletric field
	G4UniformElectricField* globalField = new G4UniformElectricField(G4ThreeVector(0., 0., 0.*kilovolt/mm));
	G4FieldManager* globalFieldManager = G4TransportationManager::GetTransportationManager()->GetFieldManager();
	globalFieldManager->SetDetectorField(globalField);
	CreateChordFinder(globalFieldManager, globalField);

	// Set local eletric positive field
	G4UniformElectricField* localPosField = new G4UniformElectricField(G4ThreeVector(0., 0., -4.5*kilovolt/mm));
	G4FieldManager* localPosFieldManager = new G4FieldManager();
	localPosFieldManager->SetDetectorField(localPosField);
	CreateChordFinder(localPosFieldManager, localPosField);
	GasLogicalVolumePos->SetFieldManager(localPosFieldManager, true);

	// Set local eletric positive field
	G4UniformElectricField* localNegField = new G4UniformElectricField(G4ThreeVector(0., 0., 4.5*kilovolt/mm));
	G4FieldManager* localNegFieldManager = new G4FieldManager();
	localNegFieldManager->SetDetectorField(localNegField);
	CreateChordFinder(localNegFieldManager, localNegField);
	GasLogicalVolumeNeg->SetFieldManager(localNegFieldManager, true);
}

void DetectorConstruction::CreateChordFinder(G4FieldManager* fieldManager, G4ElectricField* field)
{
	auto globalFieldEq = new G4EqMagElectricField(field);

	auto stepper = new G4ClassicalRK4(globalFieldEq, 8);
	G4cout << "G4ClassicalRK4 (default) is called" << G4endl;

	float minStep  = 0.02/1000*mm;
	G4cout << "The minimal step in integral is equal to " << G4BestUnit(minStep, "Length") << G4endl;
	//auto intgrDriver = new G4IntegrationDriver<G4ClassicalRK4>(minStep, stepper, stepper->GetNumberOfVariables());
	auto intgrDriver = new G4MagInt_Driver(minStep, stepper, stepper->GetNumberOfVariables());
	auto chordFinder = new G4ChordFinder(intgrDriver);
	chordFinder->SetDeltaChord(minStep);
	fieldManager->SetChordFinder(chordFinder);
}

void DetectorConstruction::ConstructSensitiveDetector()
{
	GasLogicalVolumePos->SetSensitiveDetector(sensitiveDetector);
	GasLogicalVolumeNeg->SetSensitiveDetector(sensitiveDetector);
}

void DetectorConstruction::SetGasMaterial(G4Material* material)
{
	GasLogicalVolumePos->SetMaterial(material);
	GasLogicalVolumeNeg->SetMaterial(material);
	G4cout << "Gas material set to " + material->GetName() + "\n";
}