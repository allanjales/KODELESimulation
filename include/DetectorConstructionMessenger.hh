#ifndef DetectorConstructionMessenger_hh
#define DetectorConstructionMessenger_hh

#include <G4UImessenger.hh>
#include <G4UIdirectory.hh>
#include <G4UIcmdWithADoubleAndUnit.hh>
#include <G4UIcmdWithAString.hh>

#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

class DetectorConstruction;
class DetectorConstructionMessenger: public G4UImessenger
{
public:
	DetectorConstructionMessenger(DetectorConstruction*);
	~DetectorConstructionMessenger();

private:
	DetectorConstruction* detector;
};

#endif