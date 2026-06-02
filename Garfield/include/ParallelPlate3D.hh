// Author: Allan Jales

#pragma once

#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#include "Garfield/MediumGas.hh"
#include "Garfield/GeometrySimple.hh"
#include "Garfield/ComponentAnalyticField.hh"
#include "Garfield/ComponentUser.hh"
#include "Garfield/ComponentConstant.hh"
#include "Garfield/MediumConductor.hh"
#include "Garfield/MediumMagboltz.hh"
#include "Garfield/SolidBox.hh"
#include "Garfield/Sensor.hh"
#include "Garfield/AvalancheMC.hh"
#include "Garfield/TrackHeed.hh"
#include "Garfield/AvalancheMicroscopic.hh"
#include "Garfield/ViewGeometry.hh"
#include "Garfield/AvalancheGrid.hh"

#include "Garfield/ViewMedium.hh"
#include "Garfield/MediumConductor.hh"
#include "Garfield/ViewField.hh"
#include "Garfield/ViewDrift.hh"
#include "Garfield/ViewSignal.hh"

#include "utils/Layer.hh"
// #include "utils/Vectors4D.hh"
#include "utils/Vectors3D.hh"
// #include "utils/TimeWindow.hh"
// #include "utils/Helpers.hh"
#include "utils/Units.hh"

#include "TCanvas.h"

using namespace Garfield;

class ParallelPlate3D
{
protected:
	// Geometry and components
	GeometrySimple geometry;
	ComponentUser eField;
	ComponentUser wField;
	Sensor sensor;

	// Avalanche handlers
	AvalancheMicroscopic avalanche;
	TrackHeed track;

	// Plots and viewers
	ViewField eFieldView;
	ViewField wFieldView;
	ViewDrift driftView;
	ViewSignal signalView;
	ViewGeometry geometryView;

	// Simulation status
	bool plotDriftLines = false;
	bool plotSignal = false;

	// Geometry settings
	double stackSize = 0.;
	double detectorWidth = 0.;
	double detectorHeight = 0.;

	// Path to the .gas file
	std::string gasFilePath = "";

	virtual void OnPrepareBegin()  {}
	virtual void OnPrepareEnd()    {}
	virtual void OnSimulateBegin() {}
	virtual void OnSimulateEnd()   {}

	bool shouldDebugClusters = false;

public:
	ParallelPlate3D();
	~ParallelPlate3D();

	void SetGasFile(const std::string& path) { gasFilePath = path; }

	void Setup(const std::vector<Layer>& detectorLayers, double voltage_cm, double width, double height);

	void Simulate();

	// Visualisation helpers
	void PlotElectricFieldProfile();
	void PlotElectricField2D();
	void PlotDriftLines2D();
	void View3D();
	void PlotDriftVelocity();
	void PlotSignal();
	void PlotWeightingFieldProfile();

	// Debug
	void PrintDebugGasAtPoint(double x, double y, double z);
	void EnableClustersPrintDebug() { shouldDebugClusters = true; };
	void DisableClustersPrintDebug() { shouldDebugClusters = false; };
};
