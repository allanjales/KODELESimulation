// !------------------------
// ! KODELE simulation with Garfield++
// ! Author: Allan Jales
// !------------------------

#include <TApplication.h>

#include "ParallelPlate3D.hh"
#include "utils/Units.hh"

#define DEBUGLOG(x) std::cout << x << std::endl

using namespace Garfield;

Medium* CreateDielectricMedium(const double dielectricConstant)
{
	auto* medium = new Medium();
	medium->SetDielectricConstant(dielectricConstant);
	return medium;
}

int main(int argc, char* argv[])
{
	TApplication app("app", &argc, argv);

	float width  = 50 * cm;
	float height = 50 * cm;

	// !------------------------
	// ! Materials
	// !------------------------

	Medium* cooperMedium       = new MediumConductor();
	Medium* polyethileneMedium = CreateDielectricMedium(2.25);
	Medium* graphiteMedium     = new MediumConductor();
	Medium* bakeliteMedium     = CreateDielectricMedium(5.);
	Medium* airMedium          = CreateDielectricMedium(1.00058986);

	// Load the gas file so that macroscopic transport coefficients
	// (drift velocity, Townsend coefficient, etc.) are available for AvalancheGrid.
	const std::string gasFile = "cms_rpc_95.2_4.5_0.3_25-40kV.gas";
	MediumMagboltz* gasMedium = new MediumMagboltz();
	if (!gasMedium->LoadGasFile(gasFile))
	{
		std::cerr << "ERROR: could not load gas file '" << gasFile << "'.\n";
		return 1;
	}
	gasMedium->Initialise(true);

	// !------------------------
	// ! Detector stack
	// ! The stack is centred at z=0.
	// !------------------------

	std::vector<Layer> detectorStack = {
		{cooperMedium,       0.03*mm},
		{polyethileneMedium, 0.2*mm},
		{graphiteMedium,     0.2*mm},
		{bakeliteMedium,     1.4*mm},
		{gasMedium,          1.4*mm},
		{bakeliteMedium,     1.4*mm},
		{graphiteMedium,     0.2*mm},
		{polyethileneMedium, 0.2*mm},
		{cooperMedium,       0.03*mm},
		{airMedium,          1.0*mm},
		{cooperMedium,       0.03*mm},
		{polyethileneMedium, 0.2*mm},
		{graphiteMedium,     0.2*mm},
		{bakeliteMedium,     1.4*mm},
		{gasMedium,          1.4*mm},
		{bakeliteMedium,     1.4*mm},
		{graphiteMedium,     0.2*mm},
		{polyethileneMedium, 0.2*mm},
		{cooperMedium,       0.03*mm}
	};

	// RPC Setup
	ParallelPlate3D rpc;
	double electricField = 5.0 * kV/mm;
	rpc.Setup(detectorStack, electricField, width, height);
	rpc.signalTimeWindow.Set(0.*ns, 16*ns, 1000);
	DEBUGLOG("Detector setup completed.");

	// RPC Viewing
	// rpc.View3D();
	// rpc.PlotElectricFieldProfile();
	// rpc.PlotWeightingFieldProfile();
	// rpc.PlotDriftLines2D();
	rpc.PlotSignal();
	rpc.PlotCharge();

	// RPC Simulate
	int nsimulations = 1000;
	for (float electricField = 4.0; electricField <= 5.6; electricField += 0.1)
	{
		rpc.SetElectricField(electricField * kV/mm);
		for (int i = 0; i < nsimulations; i++)
		{
			rpc.SetOutputFolder(std::string("results/") + std::to_string(electricField) + "kVmm/" + std::to_string(i) + "/");
			rpc.AddTrack("e-", 100*GeV, Vector3D(0., 0., 0.372*cm), Vector3D(0., 0., -1.), 0.*ns);
			rpc.Simulate();
			printf("Simulation completed %d/%d for electric field %f kV/mm.\n", i+1, nsimulations, electricField);
		}
	}
	
	DEBUGLOG("End of program.");
	// app.Run(true);
	return 0;
}