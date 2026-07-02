// Author: Allan Jales
// Parallel Plate Chamber in 3D 

#pragma once

#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <filesystem>

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
#include "AvalancheGrid.hh"

#include "Garfield/ViewMedium.hh"
#include "Garfield/MediumConductor.hh"
#include "Garfield/ViewField.hh"
#include "Garfield/ViewDrift.hh"
#include "Garfield/ViewSignal.hh"

#include "utils/Layer.hh"
#include "utils/Vectors3D.hh"
#include "utils/TimeWindow.hh"
#include "utils/Units.hh"
#include "utils/StopWatch.hh"
#include "utils/TrackSettings.hh"

#include "TCanvas.h"

using namespace Garfield;

class ParallelPlate3D
{
private:
	void setupGeometry();
	void setupFieldsAndSensors();
	void finishPlotsAfterSimulation();
	void clearPlotsBeforeSimulation();

	void depositCharge(string particleName, double momentum, Vector3D startPos, Vector3D direction, double startTime, bool debug);
	void depositCharge(TrackSettings trackSettings, bool debug);

protected:
	// Geometry settings
	double stackSize = 0.;
	double detectorWidth = 0.;
	double detectorHeight = 0.;
	std::vector<Layer> detectorLayers;
	double electricField = 0.;
	
	// Geometry and components
	GeometrySimple geometry;
	ComponentUser eField;
	ComponentUser wField;
	Sensor sensor;

	string sensorLabel = "ReadoutPlane";
	string otuputFolder = "results/";

	// Avalanche handlers
	std::vector<TrackSettings> tracks;
	AvalancheMicroscopic avalanche;
	AvalancheGrid avalgrid;
	TrackHeed track;

	// Plots and viewers
	ViewField eFieldView;
	ViewField wFieldView;
	ViewDrift driftView;
	ViewSignal signalView;
	ViewSignal chargeView;
	ViewGeometry geometryView;

	// Simulation status
	bool plotDriftLines = false;
	bool plotSignal = false;
	bool plotCharge = false;

	virtual void OnSetupBegin()  {}
	virtual void OnSetupEnd()    {}
	virtual void OnSimulateBegin() {}
	virtual void OnSimulateEnd()   {}

public:
	ParallelPlate3D();
	~ParallelPlate3D();

	TimeWindow signalTimeWindow;

	void SetOutputFolder(const std::string& folder) { otuputFolder = folder; }

	void Setup(const std::vector<Layer>& detectorLayers, double voltage_cm, double width, double height);

	void AddTrack(TrackSettings trackSettings) { tracks.push_back(trackSettings); }
	void AddTrack(string particleName, double momentum, Vector3D startPos, Vector3D direction, double startTime);
	void AddDebugTrack();

	void Simulate();

	// Fields definitions
	std::tuple<double, double, double> ElectricField(const double x, const double y, const double z);
	std::tuple<double, double, double> WeightingField(const double x, const double y, const double z);
	double WeightingPotentialField(const double x, const double y, const double z);

	// Visualisation helpers
	void PlotElectricFieldProfile();
	void PlotElectricField2D();
	void PlotDriftLines2D();
	void View3D();
	void PlotDriftVelocity();
	void PlotSignal();
	void PlotCharge();
	void PlotWeightingFieldProfile();

	// Debug
	void PrintDebugAtPoint(double x, double y, double z);
};
