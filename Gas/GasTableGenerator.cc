// !------------------------
// ! Gas Table Generator tool for Garfield++
// ! Author: Allan Jales
// ! Description: This tool allows users to create a .gas file for Garfield++ using Magboltz. It supports creating new gas tables, listing available gases, and merging multiple .gas files.
// !------------------------

#include <iostream>
#include "include/argparse.hpp"
#include "Garfield/MediumMagboltz.hh"

using namespace std;

std::unique_ptr<Garfield::MediumMagboltz> CreateMagboltzMedium(const string& mixtureStr)
{
	if (mixtureStr.empty())
	{
		cerr << "Mixture is null." << endl;
		exit(1);
	}

	stringstream ss(mixtureStr);
	string gasName;
	double fraction;

	vector<string> gases;
	vector<double> fractions;

	double fractionsSum = 0.0;

	while (ss >> gasName >> fraction)
	{
		gases.push_back(gasName);
		fractions.push_back(fraction);
		fractionsSum += fraction;
	}

	if (gases.size() > 6)
	{
		cerr << "Currently Magboltz supports up to 6 gases in a mixture. Aborting..." << endl;
		exit(1);
	}

	if (std::abs(fractionsSum - 1.0) > 1e-6)
	{
		cerr << "Error: The sum of the gas fractions is " << fractionsSum << ", but it must be equal to 1.0." << endl;
		exit(1);
	}

	// Filling missing gases and fractions with default values to avoid segmentation faults
	while (gases.size() < 6)
	{
		gases.push_back("");
		fractions.push_back(0.0);
	}

	cout << "Parsed gases    :";
	for (const auto& gas : gases)
		cout << " '" << gas << "'";
	cout << "\n";

	cout << "Parsed fractions:";
	for (const auto& fraction : fractions)
		cout << " "<< fraction;
	cout << "\n";

	auto gas = make_unique<Garfield::MediumMagboltz>();
	gas->SetComposition(gases[0].c_str(), fractions[0], gases[1].c_str(), fractions[1],
	                   gases[2].c_str(), fractions[2], gases[3].c_str(), fractions[3],
	                   gases[4].c_str(), fractions[4], gases[5].c_str(), fractions[5]);

	return gas;
}

int main(int argc, char *argv[])
{
	argparse::ArgumentParser program("GasTableGenerator", "1.0 - Allan Jales");
	program.add_description("Create a .gas file for Garfield++ using Magboltz.");



	// list subparser
	argparse::ArgumentParser list_command("list");
	list_command.add_description("List available gases in Magboltz");



	// create subparser
	argparse::ArgumentParser create_command("create");
	create_command.add_description("Create a gas table file for Garfield++ simulations using Magboltz");
	
	create_command.add_argument("-m", "--mixture")
	    .help("Gas mixture aside of its fractions in the [0, 1] range").metavar("\"GAS1 FRAC1 ... GAS6 FRAC6\"").required();
	create_command.add_argument("-e", "--ef")
	    .help("Electric field point where the calculations will be performed").scan<'g', double>().metavar("V/cm").required();
	create_command.add_argument("-o", "--output").help("Output gas file name").metavar("FILE.gas").required();
	
	create_command.add_argument("-t", "--temperature").help("Gas temperature in Kelvin")
	    .default_value(273.15).scan<'g', double>().nargs(1).metavar("KELVIN");
	create_command.add_argument("-p", "--pressure").help("Gas pressure in Torr")
	    .default_value(760.0).scan<'g', double>().nargs(1).metavar("TORR");
	create_command.add_argument("-n", "--collisions").help("Number of collisions in multiples of 10^7 for creating gas table")
	    .default_value(10).scan<'i', int>().nargs(1).metavar("N");

	create_command.add_argument("--no-thermal").help("Disable thermal calculation").flag();
	create_command.add_argument("-v", "--verbose").help("Enable verbose output").flag();
	
	create_command.add_epilog("Example usage:\n"
		"  ./GasTableGenerator create -m \"C2H2F4 0.952 iC4H10 0.045 SF6 0.003\" -e 1000 -o gasfile.gas");



	// merge subparser
	argparse::ArgumentParser merge_command("merge");
	merge_command.add_description("Merge multiple gas table files into a single file");

	merge_command.add_argument("output")
	    .help("Output gas file name").metavar("TARGET").required();
	merge_command.add_argument("files")
		.help("Table gas files to merge. Fileglobs (e.g.  *.gas) can be given to merge all matching files.").remaining()
		.nargs(argparse::nargs_pattern::at_least_one).metavar("SOURCE");
	merge_command.add_argument("-v", "--verbose").help("Enable verbose output").flag();


	program.add_subparser(list_command);
	program.add_subparser(create_command);
	program.add_subparser(merge_command);


	// Parsing arguments
	try
	{
		program.parse_args(argc, argv);
	}
	catch (const exception& err)
	{
		cerr << err.what() << endl;
		if (program.is_subcommand_used(create_command))
			cerr << create_command;
		else if (program.is_subcommand_used(merge_command))
			cerr << merge_command;
		else
			cerr << program;
		return 1;
	}

	// Showing list of gases
	if (program.is_subcommand_used(list_command))
	{
		cout << "Listing available gases in Magboltz...\n";
		Garfield::MediumMagboltz::PrintGases();
		cout << "The list of available gases has been printed.\n";
		return 0;
	}

	// Creating gas table
	if (program.is_subcommand_used(create_command))
	{
		// Gas creation
		std::unique_ptr<Garfield::MediumMagboltz> gas = CreateMagboltzMedium(create_command.get<string>("--mixture"));

		bool isThermalMotionEnabled = !create_command.get<bool>("--no-thermal");
		gas->SetTemperature(create_command.get<double>("--temperature"));
		gas->SetPressure(create_command.get<double>("--pressure"));
		gas->EnableThermalMotion(isThermalMotionEnabled);
		cout << "Gas created with the following properties:\n";
		cout << "  Temperature   : " << gas->GetTemperature() << " K\n";
		cout << "  Pressure      : " << gas->GetPressure() << " Torr\n";
		cout << "  Thermal motion: " << (isThermalMotionEnabled ? "enabled" : "disabled") << "\n";
		
		// Arguments reading
		double ef = create_command.get<double>("--ef");
		int collisions = create_command.get<int>("--collisions");
		bool verbose = create_command.get<bool>("--verbose");
		string filename = create_command.get<string>("--output");
		cout << "Generating gas table with the following parameters:\n";
		cout << "  Electric field: " << ef << " V/cm\n";
		cout << "  Collisions    : " << collisions << " x 10^7\n";
		cout << "  Verbose       : " << (verbose ? "enabled" : "disabled") << "\n";
		cout << "  Output file   : " << filename << "\n";
		cout << "Starting gas table generation...\n";

		gas->SetFieldGrid(ef, ef, 1, false);
		gas->GenerateGasTable(collisions, verbose);
		
		gas->WriteGasFile(filename);
		cout << "File created: " << filename << endl;
		return 0;
	}

	// Merge
	if (program.is_subcommand_used(merge_command))
	{
		auto files = merge_command.get<vector<string>>("files");
		string outputFile = merge_command.get<string>("output");
		Garfield::MediumMagboltz gas;

		for (size_t i = 0; i < files.size(); i++)
		{
			if (i == 0)
			{
				cout << "Loading '" << files[i] << "'...\n";
				if (!gas.LoadGasFile(files[i]))
				{
					cerr << "Error: Failed to load the first gas file: " << files[i] << "\n";
					return 1;
				}

				continue;
			}

			cout << "Merging '" << files[i] << "'...\n";
			if (!gas.MergeGasFile(files[i], false))
				cerr << "Warning: Failed to merge gas file: " << files[i] << "\n";
		}

		cout << "Writing final gas table to '" << outputFile << "'...\n";
		
		if (merge_command.get<bool>("--verbose"))
			gas.EnableDebugging();

		if (!gas.WriteGasFile(outputFile))
		{
			cerr << "Error: Failed to write merged gas file.\n";
			return 1;
		}
		
		cout << "Success: Merged gas file saved at '" << outputFile << "'\n";
		return 0;
	}
	
	cerr << program;
	return 1;
}