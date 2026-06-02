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
	eField.SetArea(-width/2., -height/2., -stackSize/2., width/2., height/2., stackSize/2.);

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

	sensor.AddComponent(&eField);
	sensor.AddElectrode(&wField, "ReadoutPlane");

	// Sometimes ComponentUser is ignored in search of medium materials
	// Geometry Anchor is a dummy componet that hold visible heed geometry
	// Without it, it cannot create a track
	auto* geomAnchor = new ComponentConstant();
	geomAnchor->SetGeometry(&geometry);
	geomAnchor->SetElectricField(0., 0., 0.);
	sensor.AddComponent(geomAnchor);

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
	sensor.SetTimeWindow(0., 50., 1000);

	OnSimulateBegin();

	track.SetSensor(&sensor);
	track.SetParticle("mu-");
	track.SetMomentum(100*GeV);
	track.CrossInactiveMedia(true);
	
	// Start the track just inside the top gas gap, travelling in the -z direction
	// Hardcoded for a while
	Vector3D startPos(0., 0., 0.303*cm);
	printf("Starting track at (%+.3f, %+.3f, %+.3f) cm\n", startPos.x, startPos.y, startPos.z/cm);
	track.NewTrack(startPos.x, startPos.y, startPos.z, 0., 0., 0., -1.);
	PrintDebugGasAtPoint(startPos.x, startPos.y, startPos.z);

	avalanche.UseWeightingPotential();
	avalanche.SetSensor(&sensor);
	avalanche.SetTimeWindow(0., 50.); // up to 50 ns
	avalanche.EnableSignalCalculation();
	avalanche.EnableAvalancheSizeLimit(100); // Heinrich tip

	if (plotDriftLines)
	{
		track.EnablePlotting(&driftView);
		avalanche.EnablePlotting(&driftView);
	}
	
	// AvalancheGrid for the macroscopic drift/multiplication stage.
	AvalancheGrid avalgrid(&sensor);
	const int nxBins = 5;
	const int nyBins = 5;
	const int nzBins = 500;
	avalgrid.SetGrid(
		-detectorWidth  / 2., +detectorWidth  / 2., nxBins,
		-detectorHeight / 2., +detectorHeight / 2., nyBins,
		-stackSize      / 2., +stackSize      / 2., nzBins
	);
	avalgrid.SetSensor(&sensor);

	// Iterate over primary clusters produced by the muon track
	for (const auto& cluster : track.GetClusters())
	{
		if (shouldDebugClusters)
			printf("[DEBUG] Cluster at (%+.3f, %+.3f, %+.3f) cm | ne = %zu\n", cluster.x/cm, cluster.y/cm, cluster.z/cm, cluster.electrons.size());

		for (const auto& electron : cluster.electrons)
		{
			// Calculates the avalanche in a microscopic level up to the limit set by EnableAvalancheSizeLimit()
			if (shouldDebugClusters)
				printf("[DEBUG] Microscopic avalanche at (%+.3f, %+.3f, %+.3f) cm\n", electron.x/cm, electron.y/cm, electron.z/cm);
			avalanche.AvalancheElectron(electron.x, electron.y, electron.z, electron.t, 0.1, 0., 0., 0.);
			
			// Transfer electrons to avalanche grid for calculating macroscopic avalanche
			if (shouldDebugClusters)
				printf("[DEBUG] Transferred to macroscopic grid: ne = %zu\n", avalanche.GetElectrons().size());
			avalgrid.AddElectrons(&avalanche);
		}
	}
	
	// Grid-based macroscopic avalanche
	printf("Switching to grid-based avalanche method\n");
	avalgrid.StartGridAvalanche();
	
	// Plot drift lines if requested
	if (plotDriftLines)
		driftView.Plot2d();

	// Plot the signal on the single readout electrode
	if (plotSignal)
	{
		auto* cSignal = new TCanvas("cSignal", "Signal – readout plane", 800, 800);
		signalView.SetCanvas(cSignal);
		signalView.SetSensor(&sensor);
		signalView.PlotSignal("ReadoutPlane");

		const std::string fname = std::string("signal_") + "ReadoutPlane" + ".png";
		cSignal->SaveAs(fname.c_str());
		std::cout << "Saved signal plot: " << fname << "\n";
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

void ParallelPlate3D::PrintDebugGasAtPoint(double x, double y, double z)
{
	double ex, ey, ez;
	Medium* medium = nullptr;
	int status = 0;
	sensor.ElectricField(x, y, z, ex, ey, ez, medium, status);
	
	double alpha = 0.; // Townsend coefficient (multiplication)
	double eta = 0.;   // Attachment coefficient (loss)
	std::string mediumName = "[None]";
	
	if (medium)
	{
		medium->ElectronTownsend(ex, ey, ez, 0., 0., 0., alpha);
		medium->ElectronAttachment(ex, ey, ez, 0., 0., 0., eta);
		mediumName = medium->GetName();
	}
	
	// From https://garfieldpp.docs.cern.ch/doxygen/classGarfield_1_1ComponentUser.html#af2c5cfe152cb77177ccf3c21c9637d41

	std::string statusMsg;
	switch (status)
	{
		case 0:
			statusMsg = "Inside an active medium";
			break;
		case -5:
			statusMsg = "Inside the mesh but not in an active medium";
			break;
		case -6:
			statusMsg = "Outside the mesh";
			break;
		case -10:
			statusMsg = "Unknown potential type (should not occur)";
			break;
		default:
			if (status >= -4 && status <= -1)
			{
        		statusMsg = "On the side of a plane where no wires are";
				break;
			}

			if (status > 0)
			{
				statusMsg = "Inside a wire";
				break;
			}
			
			statusMsg = "Other cases (should not occur)";
			break;
	}

	printf("========== GAS DIAGNOSTIC AT POINT ==========\n");
	printf("Position                     = (%+.3f, %+.3f, %+.3f) cm\n", x/cm, y/cm, z/cm);
	printf("Medium name found            = %s\n", mediumName.c_str());
	printf("Status (%+3d)                 = %s\n", status, statusMsg.c_str());
	printf("Electric field (Ez)          = %.1f kV/mm\n", ez/(kV/mm));
	if (medium && medium->IsGas())
	{
		printf("Townsend coefficient (alpha) = %.3f pairs/cm\n", alpha);
		printf("Attachment coefficient (eta) = %.3f losses/cm\n", eta);
	}
	printf("=============================================\n");
}