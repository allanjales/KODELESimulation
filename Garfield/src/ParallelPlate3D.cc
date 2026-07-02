// Author: Allan Jales
// Parallel Plate Chamber in 3D 

#include "ParallelPlate3D.hh"
#define DEBUGLOG(x) std::cout << x << std::endl

ParallelPlate3D::ParallelPlate3D()
{}

ParallelPlate3D::~ParallelPlate3D()
{}

std::tuple<double, double, double> ParallelPlate3D::ElectricField(const double x, const double y, const double z)
{
	double ez = (z > 0) ? this->electricField : -this->electricField;
	return {0., 0., ez};
}

std::tuple<double, double, double> ParallelPlate3D::WeightingField(const double x, const double y, const double z)
{
	double halfStack = this->stackSize / 2.0;
	double wz = (z > 0) ? (1.0 / halfStack) : (-1.0 / halfStack);
	return {0., 0., wz};
}

double ParallelPlate3D::WeightingPotentialField(const double x, const double y, const double z)
{
	double halfStack = this->stackSize / 2.0;

	// Outside the stack
	if (std::abs(z) > halfStack)
		return 0.;

	return 1. - std::abs(z) / halfStack;
}

void ParallelPlate3D::setupGeometry()
{
	// Compute total stack thickness and accumulate gas gap thickness
	this->stackSize = 0.;
	double totalGasThickness = 0.;
	for (const auto& layer : detectorLayers)
	{
		this->stackSize += layer.thickness;

		if (dynamic_cast<MediumMagboltz*>(layer.medium))
			totalGasThickness += layer.thickness;
	}
	printf("-------------------------------\n");
	printf("Stack size:          %.2f mm\n", stackSize / mm);
	printf("Electric field:      %.2f kV/mm\n", electricField / (kV/mm));

	// Ignore if the is no gas
	if (totalGasThickness <= 0.)
		std::cerr << "[WARNING] No gas layer found in detector stack!\n";

	// Place geometry solids from top to bottom
	printf("Layers:\n");
	double currentZ = +stackSize / 2.;
	for (const auto& layer : detectorLayers)
	{
		currentZ -= layer.thickness;
		double posZ = currentZ + layer.thickness / 2.0;

		auto* solid = new SolidBox(0., 0., posZ, this->detectorWidth / 2., this->detectorHeight / 2., layer.thickness / 2.);
		geometry.AddSolid(solid, layer.medium);
		printf("z = (%+.3f, %+.3f) cm | %s\n", currentZ/cm, (currentZ/cm + layer.thickness/cm), layer.medium->GetName().c_str());
	}
	printf("-------------------------------\n");
}

void ParallelPlate3D::setupFieldsAndSensors()
{
	auto electricFieldLambda = [this](const double x, const double y, const double z, double& ex, double& ey, double &ez)
	{
		auto [cx, cy, cz] = this->ElectricField(x, y, z);
		ex = cx;
		ey = cy;
		ez = cz;
	};

	eField.SetGeometry(&geometry);
	eField.SetElectricField(electricFieldLambda);

	auto weigthtingFieldLambda = [this](const double x, const double y, const double z, double& wx, double& wy, double &wz)
	{
		auto [cx, cy, cz] = this->WeightingField(x, y, z);
		wx = cx;
		wy = cy;
		wz = cz;
	};

	wField.SetGeometry(&geometry);
	wField.SetWeightingField(weigthtingFieldLambda, sensorLabel);

	auto weigthtingPotentialFieldLambda = [this](const double x, const double y, const double z) -> double
	{
		return this->WeightingPotentialField(x, y, z);
	};

	wField.SetWeightingPotential(weigthtingPotentialFieldLambda, sensorLabel);

	sensor.AddComponent(&eField);
	sensor.AddElectrode(&wField, sensorLabel);

	track.SetSensor(&sensor);
	track.CrossInactiveMedia(true); // Some Garfield++ version does not has this
}

/// @brief Sets up the parallel plate detector geometry, electric field, and sensor components.
/// @param detectorLayers 
/// @param electricField 
/// @param width 
/// @param height 
void ParallelPlate3D::Setup(const std::vector<Layer>& detectorLayers, double electricField, double width, double height)
{
	OnSetupBegin();

	this->detectorWidth  = width;
	this->detectorHeight = height;
	this->detectorLayers = detectorLayers;
	this->electricField = electricField;
	
	setupGeometry();
	setupFieldsAndSensors();

	OnSetupEnd();
}

/// @brief Deposits a charge in the detector by creating a new track for a particle
/// @param particleName i.e. e-, mu+
/// @param momentum by default in eV/c 
/// @param startPos 3D vector (x, y, z) in cm by default
/// @param direction 3D unitary vector (dx, dy, dz)
/// @param startTime start time of the track in ns
/// @param debug whether to print debug information at point
void ParallelPlate3D::depositCharge(string particleName, double momentum, Vector3D startPos, Vector3D direction, double startTime = 0., bool debug = false)
{
	track.SetParticle(particleName);
	track.SetMomentum(momentum);
	direction = direction.Normalize();

	printf("Creating track at (%+.3f, %+.3f, %+.3f) cm at t = %+.3f ns pointing towards (%+.3f, %+.3f, %+.3f)\n",
		startPos.x, startPos.y, startPos.z/cm, startTime, direction.x, direction.y, direction.z);
	track.NewTrack(startPos.x, startPos.y, startPos.z, startTime, direction.x, direction.y, direction.z);

	if (debug)
		PrintDebugAtPoint(startPos.x, startPos.y, startPos.z);
}

void ParallelPlate3D::depositCharge(TrackSettings trackSettings, bool debug = false)
{
	depositCharge(trackSettings.particleName, trackSettings.momentum, trackSettings.startPos, trackSettings.direction, trackSettings.startTime, debug);
}

void ParallelPlate3D::AddDebugTrack()
{
	string particleName = "e-";
	double momentum = 100*GeV;
	Vector3D startPos(0., 0., 0.372*cm);
	Vector3D direction(0., 0., -1.);
	AddTrack(particleName, momentum, startPos, direction, 0.);
}

void ParallelPlate3D::AddTrack(string particleName, double momentum, Vector3D startPos, Vector3D direction, double startTime)
{
	TrackSettings trackSettings;
	trackSettings.particleName = particleName;
	trackSettings.momentum = momentum;
	trackSettings.startPos = startPos;
	trackSettings.direction = direction;
	trackSettings.startTime = startTime;

	AddTrack(trackSettings);
}

void ParallelPlate3D::Simulate()
{
	printf("-------Simulation Started--------\n");
	sensor.ClearSignal();
	sensor.SetTimeWindow(signalTimeWindow.GetMin(), signalTimeWindow.GetStep(), signalTimeWindow.GetnBins());

	clearPlotsBeforeSimulation();
	OnSimulateBegin();

	avalanche.UseWeightingPotential();
	avalanche.SetSensor(&sensor);
	avalanche.SetTimeWindow(0.*ns, 1*ns);
	avalanche.EnableSignalCalculation();
	avalanche.EnableExcitationMarkers(false);
	
	// AvalancheGrid for the macroscopic drift/multiplication stage.
	const int nxBins = int(stackSize / (1.e-3*cm));
	const int nyBins = int(stackSize / (1.e-3*cm));
	const int nzBins = int(stackSize / (1.e-3*cm));
	avalgrid.SetGrid(
		-stackSize / 2., +stackSize / 2., nxBins,
		-stackSize / 2., +stackSize / 2., nyBins,
		-stackSize / 2., +stackSize / 2., nzBins
	);
	avalgrid.SetSensor(&sensor);
	avalgrid.Reset();
	// avalgrid.EnableDebugging();

	// Iterate over primary clusters produced by the muon track
	printf("Starting Avalanche Microscopic simulation...\n");
	StopWatch timer;

	for (const auto& trackSettings : tracks)
	{
		depositCharge(trackSettings, false);
		for (const auto& cluster : track.GetClusters())
		{
			for (const auto& electron : cluster.electrons)    
			{
				// Calculates the avalanche in a microscopic level
				avalanche.AvalancheElectron(electron.x, electron.y, electron.z, electron.t, 0.1, 0., 0., 0.);
				
				// Transfer electrons to avalanche grid for calculating macroscopic avalanche
				avalgrid.AddElectrons(&avalanche);
			}
		}
	}
	tracks.clear();
	printf("Avalanche Microscopic finished. Took %s\n", timer.TimeElapsedString().c_str());
	
	// Grid-based macroscopic avalanche
	timer.Start();
	avalgrid.StartGridAvalanche();
	printf("Avalanche Grid finished. Took %s\n", timer.TimeElapsedString().c_str());

	finishPlotsAfterSimulation();

	if (!sensor.IsIntegrated(sensorLabel))
		sensor.IntegrateSignal(sensorLabel);
	double qTotal = sensor.GetTotalInducedCharge(sensorLabel);
	printf("Total induced charge: %.3f fC\n", qTotal);

	OnSimulateEnd();
	printf("--------Simulation Ended--------\n");
}

void ParallelPlate3D::clearPlotsBeforeSimulation()
{
	if (plotDriftLines) { driftView.Clear(); }
}

void ParallelPlate3D::finishPlotsAfterSimulation()
{
	// Plot drift lines if requested
	if (plotDriftLines)
	{
		auto* cDrift = new TCanvas("cDrift", "Drift Lines Top", 800, 800);
		driftView.SetCanvas(cDrift);
		driftView.Plot2d();
		filesystem::create_directories(otuputFolder);
		cDrift->SaveAs((otuputFolder + "drift_lines_top.png").c_str());
	}

	// Plot the signal on the single readout electrode
	if (plotSignal)
	{
		auto* cSignal = new TCanvas("cSignal", "Signal - readout plane", 800, 800);
		signalView.SetCanvas(cSignal);
		signalView.SetSensor(&sensor);
		signalView.PlotSignal(sensorLabel);
		filesystem::create_directories(otuputFolder);
		cSignal->SaveAs((otuputFolder + "signal.png").c_str());
		sensor.ExportSignal(sensorLabel, (otuputFolder + "Signal").c_str());
	}
	
	if (plotCharge)
	{
		sensor.IntegrateSignal(sensorLabel);
		auto* cCharge = new TCanvas("cCharge", "Charge - readout plane", 800, 800);
		chargeView.SetCanvas(cCharge);
		chargeView.SetSensor(&sensor);
		chargeView.PlotSignal(sensorLabel);
		filesystem::create_directories(otuputFolder);
		cCharge->SaveAs((otuputFolder + "charge.png").c_str());
		sensor.ExportSignal(sensorLabel, (otuputFolder + "Charge").c_str());
	}
}

void ParallelPlate3D::PlotElectricField2D()
{
	eFieldView.SetSensor(&sensor);
	eFieldView.SetPlaneXZ();
	eFieldView.SetArea(-detectorWidth / 2., -stackSize / 2., +detectorWidth / 2., +stackSize / 2.);
	eFieldView.PlotContour("e");
}

void ParallelPlate3D::PlotElectricFieldProfile()
{
	auto* ceProfile = new TCanvas("ceProfile", "E-Field Profile", 800, 800);
	eFieldView.SetCanvas(ceProfile);
	// eFieldView.SetComponent(&eField);
	eFieldView.SetSensor(&sensor);
	eFieldView.PlotProfile(0., 0., -stackSize / 2., 0., 0., stackSize / 2, "ez");
	filesystem::create_directories(otuputFolder);
	ceProfile->SaveAs((otuputFolder + "efield_profile.png").c_str());
}

void ParallelPlate3D::PlotWeightingFieldProfile()
{
	auto* cwProfile = new TCanvas("cwProfile", "W-Field Profile", 800, 800);
	wFieldView.SetCanvas(cwProfile);
	wFieldView.SetSensor(&sensor);
	wFieldView.PlotProfileWeightingField(sensorLabel, 0., 0., -stackSize / 2., 0., 0., stackSize / 2., "v");
	filesystem::create_directories(otuputFolder);
	cwProfile->SaveAs((otuputFolder + "wfield_profile.png").c_str());
}

void ParallelPlate3D::PlotDriftLines2D()
{
	if (stackSize <= 0.)
	{
		std::cerr << "[ERROR] Should call Setup() before this.\n";
		return;
	}

	const double zTop = 0.373*cm;
	const double zBot = -0.373*cm;
	driftView.SetPlaneXZ();
	driftView.SetArea(-0.10*cm, zBot, +0.10*cm, zTop);
	track.EnablePlotting(&driftView);
	avalanche.EnablePlotting(&driftView);

	plotDriftLines = true;
}

void ParallelPlate3D::View3D()
{
	geometryView.SetGeometry(&geometry);
	geometryView.Plot();
}

void ParallelPlate3D::PlotSignal()
{
	plotSignal = true;
}

void ParallelPlate3D::PlotCharge()
{
	plotCharge = true;
}

void ParallelPlate3D::PrintDebugAtPoint(double x, double y, double z)
{
	printf("------------ DIAGNOSTIC AT POINT ------------\n");
	printf("Position                     = (%+.3f, %+.3f, %+.3f) cm\n", x/cm, y/cm, z/cm);

	// Lambda function for getting status messages
	auto getMessageFromStatus = [](int status) -> const char*
	{
		// From https://garfieldpp.docs.cern.ch/doxygen/classGarfield_1_1ComponentUser.html#af2c5cfe152cb77177ccf3c21c9637d41
		switch (status)
		{
			case 0:
				return "Inside an active medium";
			case -1:
			case -2:
			case -3:
			case -4:
				return "On the side of a plane where no wires are";
			case -5:
				return "Inside the mesh but not in an active medium";
			case -6:
				return "Outside the mesh";
			case -10:
				return "Unknown potential type (should not occur)";
			default:
				if (status > 0)
					return "Inside a wire";
				return "Other cases (should not occur)";
		}
	};

	// Lambda function
	auto printDiagnostic = [&](const std::string& name, auto& component)
	{
		double cx, cy, cz;
		Medium* c_medium = nullptr;
		int c_status = -10;
		
		component.ElectricField(x, y, z, cx, cy, cz, c_medium, c_status);
		std::string c_mediumName = c_medium ? c_medium->GetName() : "[Null]";
		
		double c_alpha = 0.; // Townsend coefficient
		double c_eta = 0.;   // Attachment coefficient
		
		if (c_medium && c_medium->IsGas())
		{
			c_medium->ElectronTownsend(cx, cy, cz, 0., 0., 0., c_alpha);
			c_medium->ElectronAttachment(cx, cy, cz, 0., 0., 0., c_eta);
		}

		printf("\n--- %s ---\n", name.c_str());
		printf("Medium found                 = %s\n", c_mediumName.c_str());
		printf("Status (%+3d)                 = %s\n", c_status, getMessageFromStatus(c_status));
		printf("Electric field (Ez)          = %.1f V/cm\n", cz);
		
		if (c_medium && c_medium->IsGas())
		{
			printf("Townsend coefficient (alpha) = %.3f pairs/cm\n", c_alpha);
			printf("Attachment coefficient (eta) = %.3f losses/cm\n", c_eta);
		}
	};

	printDiagnostic("eField", eField);
	printDiagnostic("Sensor", sensor);

	printf("---------------------------------------------\n");
}