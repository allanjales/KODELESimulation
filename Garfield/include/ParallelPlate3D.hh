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
// #include "Garfield/AvalancheGrid.hh"
#include "AvalancheGrid.hh"

#include "Garfield/ViewMedium.hh"
#include "Garfield/MediumConductor.hh"
#include "Garfield/ViewField.hh"
#include "Garfield/ViewDrift.hh"
#include "Garfield/ViewSignal.hh"

#include "utils/Layer.hh"
// #include "utils/Vectors4D.hh"
#include "utils/Vectors3D.hh"
#include "utils/TimeWindow.hh"
// #include "utils/Helpers.hh"
#include "utils/Units.hh"
#include "utils/StopWatch.hh"

#include "TCanvas.h"

using namespace Garfield;

class ParallelPlate3D
{
private:
	void setupGeometry();
	void setupFieldsAndSensors();
	void finishPlotsAndPrintTotalCharge();

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

	// Avalanche handlers
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

	virtual void OnSetupBegin()  {}
	virtual void OnSetupEnd()    {}
	virtual void OnSimulateBegin() {}
	virtual void OnSimulateEnd()   {}

public:
	ParallelPlate3D();
	~ParallelPlate3D();

	TimeWindow signalTimeWindow;

	void Setup(const std::vector<Layer>& detectorLayers, double voltage_cm, double width, double height);
	void DepositCharge(string particleName, double momentum, Vector3D startPos, Vector3D direction, double startTime, bool debug);
	void DepositDebugCharge();
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
	void PlotWeightingFieldProfile();

	// Debug
	void PrintDebugAtPoint(double x, double y, double z);
};
