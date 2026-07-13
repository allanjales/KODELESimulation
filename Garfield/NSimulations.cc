// !------------------------
// ! KODELE N simulation with Garfield++
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
	// !------------------------
	// ! Command-line arguments parsing
	// !------------------------
	if (argc < 4)
	{
		std::cerr << "Usage: " << argv[0] << " <electric_field_kVmm> <number_of_simulations> <gas_file>\n";
		std::cerr << "Example: " << argv[0] << " 4.5 1000 CO2_30_SF6_1.gas\n";
		return 1;
	}

	double electricField = std::stod(argv[1]);
	int nsimulations     = std::stoi(argv[2]);
	std::string gasFile  = argv[3];

	std::string gasName = gasFile;
	size_t lastSlash = gasName.find_last_of("/\\");
	if (lastSlash != std::string::npos)
	{
		gasName = gasName.substr(lastSlash + 1);
	}
	size_t dotPos = gasName.rfind(".gas");
	if (dotPos != std::string::npos)
	{
		gasName = gasName.substr(0, dotPos);
	}

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
	rpc.Setup(detectorStack, electricField * kV/mm, width, height);
	rpc.signalTimeWindow.Set(0.*ns, 20*ns, 1000);
	DEBUGLOG("Detector setup completed for E = " << electricField << " kV/mm.");

	// RPC Simulate
	rpc.SetElectricField(electricField * kV/mm);

	std::stringstream ss;
	ss << std::fixed << std::setprecision(1) << electricField;
	std::string efStr = ss.str();
	
	std::string outputFolder = "results/" + gasName;
	filesystem::create_directories(outputFolder);
	std::ofstream outputFile(outputFolder + "/tic_" + efStr + "kVmm.csv");
	
	if (!outputFile.is_open())
	{
		std::cerr << "Error: Could not create or open the file!" << std::endl;
		return 1;
	}

	outputFile << "The total induced charge.\n";
	outputFile << "id,Total Charge [fC]\n";
	for (int i = 0; i < nsimulations; i++)
	{
		rpc.AddTrack("e-", 100*GeV, Vector3D(0., 0., 0.372*cm), Vector3D(0., 0., -1.), 0.*ns);
		rpc.Simulate();
		outputFile << i << "," << rpc.GetTotalInducedCharge() << "\n";
		printf("Simulation completed %d/%d for electric field %f kV/mm.\n", i+1, nsimulations, electricField);
	}
	
	outputFile.close();

	DEBUGLOG("End of program for E = " << efStr << " kV/mm.");
	return 0;
}