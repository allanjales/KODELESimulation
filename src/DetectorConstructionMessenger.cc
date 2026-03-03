#include "DetectorConstructionMessenger.hh"
#include "DetectorConstruction.hh"

DetectorConstructionMessenger::DetectorConstructionMessenger(DetectorConstruction* detector)
: detector(detector)
{}

DetectorConstructionMessenger::~DetectorConstructionMessenger()
{}