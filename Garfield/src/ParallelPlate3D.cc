// Author: Allan Jales

#include "ParallelPlate3D.hh"
#define DEBUGLOG(x) std::cout << x << std::endl

ParallelPlate3D::ParallelPlate3D()
{}

ParallelPlate3D::~ParallelPlate3D()
{}

void ParallelPlate3D::Setup(const std::vector<Layer>& detectorLayers, double electricField, double width, double height)
{
	OnPrepareBegin();

	// ---------------------
	// Geometry
	// ---------------------

	detectorWidth  = width;
	detectorHeight = height;

	// Compute total stack thickness and accumulate gas gap thickness
	stackSize = 0.;
	double totalGasThickness = 0.;
	for (const auto& layer : detectorLayers)
	{
		stackSize += layer.thickness;

		if (dynamic_cast<MediumMagboltz*>(layer.medium))
			totalGasThickness += layer.thickness;
	}
	printf("-------------------------------\n");
	printf("Stack size:          %.1f mm\n", stackSize / mm);
	printf("Total gas thickness: %.1f mm\n", totalGasThickness / mm);
	printf("Electric field:      %.1f kV/mm\n", electricField / (kV/mm));

	// Ignore if the is no gas
	if (totalGasThickness <= 0.)
	{
		std::cerr << "[ParallelPlate3D] ERROR: no gas layer found in detector stack!\n";
		return;
	}

	// Place geometry solids from top to bottom
	printf("Layers:\n");
	double currentZ = +stackSize / 2.;
	for (const auto& layer : detectorLayers)
	{
		currentZ -= layer.thickness;
		double posZ = currentZ + layer.thickness / 2.0;

		auto* solid = new SolidBox(0., 0., posZ, width / 2., height / 2., layer.thickness / 2.);
		geometry.AddSolid(solid, layer.medium);
		printf("z = (%+.3f, %+.3f) cm | %s\n", currentZ/cm, (currentZ/cm + layer.thickness/cm), layer.medium->GetName().c_str());
	}
	printf("-------------------------------\n");

	// ---------------------
	// Fields & Sensor
	// ---------------------

	auto eLinear = [electricField](const double x, const double y, const double z, double& ex, double& ey, double &ez)
	{
		ex = ey = 0.;
		ez = (z > 0) ? electricField : -electricField;
	};

	eField.SetGeometry(&geometry);
	eField.SetElectricField(eLinear);

	// Campo e Potencial de Peso
	auto wFieldLambda = [this](const double x, const double y, const double z, double& wx, double& wy, double& wz)
	{
		wx = wy = 0.;
		double halfStack = stackSize / 2.0;
		wz = (z > 0) ? (1.0 / halfStack) : (-1.0 / halfStack);
	};

	auto wPotLambda = [this](const double x, const double y, const double z) -> double
	{
		double halfStack = stackSize / 2.0;
		return 1.0 - std::abs(z) / halfStack;
	};

	wField.SetGeometry(&geometry);
	wField.SetWeightingField(wFieldLambda, "ReadoutPlane");
	wField.SetWeightingPotential(wPotLambda, "ReadoutPlane");

	// Sometimes ComponentUser is ignored in search of medium materials
	// Geometry Anchor is a dummy componet that hold visible heed geometry
	// Without it, it cannot create a track
	geomAnchor.SetGeometry(&geometry);
	geomAnchor.SetElectricField(0., 0., 0.);

	// Must be in this order. The last component stands!
	sensor.AddComponent(&geomAnchor);
	sensor.AddComponent(&eField);
	sensor.AddElectrode(&wField, "ReadoutPlane");

	OnPrepareEnd();
}

void ParallelPlate3D::Simulate()
{
	if (gasFilePath.empty())
	{
		std::cerr << "[ParallelPlate3D] WARNING: no gas file set. "
		          << "Call SetGasFile() before Simulate() to enable AvalancheGrid.\n";
		return;
	}

	sensor.ClearSignal();
	sensor.SetTimeWindow(signalTimeWindow.GetMin(), signalTimeWindow.GetStep(), signalTimeWindow.GetnBins());

	OnSimulateBegin();

	track.SetSensor(&sensor);
	track.SetParticle("mu-");
	track.SetMomentum(100*GeV);
	track.CrossInactiveMedia(true);
	
	// Start the track just inside the top gas gap, travelling in the -z direction
	// Hardcoded for a while
	Vector3D startPos(0., 0., 0.372*cm);
	printf("Starting track at (%+.3f, %+.3f, %+.3f) cm\n", startPos.x, startPos.y, startPos.z/cm);
	track.NewTrack(startPos.x, startPos.y, startPos.z, 0., 0., 0., -1.);
	PrintDebugAtPoint(startPos.x, startPos.y, startPos.z);

	avalanche.UseWeightingPotential();
	avalanche.SetSensor(&sensor);
	avalanche.SetTimeWindow(0., 50.); // up to 50 ns
	avalanche.EnableSignalCalculation();
	avalanche.EnableAvalancheSizeLimit(100); // Heinrich tip
	// avalanche.EnableIonisationMarkers(false);  // Dont show new excitation points in drift view
	
	// AvalancheGrid for the macroscopic drift/multiplication stage.
	AvalancheGrid avalgrid(&sensor);
	const int nxBins = 500;
	const int nyBins = 500;
	const int nzBins = 500;
	avalgrid.SetGrid(
		-detectorWidth  / 2., +detectorWidth  / 2., nxBins,
		-detectorHeight / 2., +detectorHeight / 2., nyBins,
		-stackSize      / 2., +stackSize      / 2., nzBins
	);
	avalgrid.SetSensor(&sensor);

	// Iterate over primary clusters produced by the muon track
	StopWatch timer;
	for (const auto& cluster : track.GetClusters())
	{
		if (shouldDebugClusters)
			printf("[DEBUG] Cluster at (%+.3f, %+.3f, %+.3f) cm | ne = %zu\n", cluster.x/cm, cluster.y/cm, cluster.z/cm, cluster.electrons.size());

		for (const auto& electron : cluster.electrons)
		{
			// Calculates the avalanche in a microscopic level up to the limit set by EnableAvalancheSizeLimit()
			if (shouldDebugClusters)
				printf("[DEBUG]  Microscopic avalanche at (%+.3f, %+.3f, %+.3f) cm\n", electron.x/cm, electron.y/cm, electron.z/cm);
			avalanche.AvalancheElectron(electron.x, electron.y, electron.z, electron.t, 0.1, 0., 0., 0.);
			
			// Transfer electrons to avalanche grid for calculating macroscopic avalanche
			if (shouldDebugClusters)
				printf("[DEBUG]  Transferred to macroscopic grid: ne = %zu\n", avalanche.GetElectrons().size());
			avalgrid.AddElectrons(&avalanche);
		}
	}
	std::cout << "[INFO] Microscopic part ended in " + timer.TimeElapsedString() + "\n";
	
	// Grid-based macroscopic avalanche
	printf("Switching to grid-based avalanche method\n");
	avalgrid.StartGridAvalanche();
	std::cout << "[INFO] Macroscopic part ended in " + timer.TimeElapsedString() + "\n";

	// Plot drift lines if requested
	if (plotDriftLines)
	{
		auto* cDrift = new TCanvas("cDrift", "Drift Lines", 800, 800);
		driftView.SetCanvas(cDrift);
		driftView.Plot2d();
		const std::string fname = "drift_lines.png";
		cDrift->SaveAs(fname.c_str());
		// std::cout << "[INFO] Saved drift lines plot: " << fname << "\n";
	}

	// Plot the signal on the single readout electrode
	if (plotSignal)
	{
		auto* cSignal = new TCanvas("cSignal", "Signal – readout plane", 800, 800);
		signalView.SetCanvas(cSignal);
		signalView.SetSensor(&sensor);
		signalView.PlotSignal("ReadoutPlane");

		const std::string fname = "signal_ReadoutPlane.png";
		cSignal->SaveAs(fname.c_str());
		// std::cout << "Saved signal plot: " << fname << "\n";
	}

	double qTotal = sensor.GetTotalInducedCharge("ReadoutPlane");
	std::cout << "[INFO] Total induced charge: " << qTotal << " fC\n";

	OnSimulateEnd();
}

void ParallelPlate3D::PlotElectricField2D()
{
	eFieldView.SetSensor(&sensor);
	eFieldView.SetPlaneXZ();
	eFieldView.SetArea(
		-detectorWidth / 2., -stackSize / 2.,
		+detectorWidth / 2., +stackSize / 2.);
	eFieldView.PlotContour("e");
}

void ParallelPlate3D::PlotElectricFieldProfile()
{
	auto* ceProfile = new TCanvas("ceProfile", "E-Field Profile", 800, 800);
	eFieldView.SetCanvas(ceProfile);
	eFieldView.SetComponent(&eField);
	eFieldView.PlotProfile(0., 0., -stackSize / 2., 0., 0., stackSize / 2, "ez");
	ceProfile->Update();
	ceProfile->SaveAs("efield_profile.png");
}

void ParallelPlate3D::PlotWeightingFieldProfile()
{
	auto* cwProfile = new TCanvas("cwProfile", "W-Field Profile", 800, 800);
	eFieldView.SetCanvas(cwProfile);
	eFieldView.SetSensor(&sensor);
	eFieldView.PlotProfileWeightingField("ReadoutPlane", 0., 0., -stackSize / 2., 0., 0., stackSize / 2., "v");
	cwProfile->SaveAs("wfield_profile.png");
}

void ParallelPlate3D::PlotDriftLines2D()
{
	if (stackSize <= 0.)
	{
		std::cerr << "[ParallelPlate3D] ERROR: call Setup() before PlotDriftLines2D().\n";
		return;
	}

	driftView.SetPlaneXZ();

	// Show only the top gas gap where the primary track starts
	// Hardcoded offset for a while. Later I will automatize this.
	const double zTop = stackSize / 2.
	                  - 0.03 * mm
	                  - 0.20 * mm
	                  - 0.20 * mm
	                  - 1.40 * mm;   // top face of top gas gap
	const double zBot = zTop - 1.40 * mm; // bottom face of top gas gap

	driftView.SetArea(-0.05 * cm, zBot, 0.05 * cm, zTop);

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
	printDiagnostic("geomAnchor", geomAnchor);
	printDiagnostic("Sensor", sensor);

	printf("---------------------------------------------\n");
}