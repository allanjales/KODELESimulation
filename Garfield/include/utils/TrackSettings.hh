// Author: Allan Jales
// Layer class to store infos of a track

#pragma once

#include <string>
#include "utils/Vectors3D.hh"

struct TrackSettings
{
	string particleName;
	double momentum;
	Vector3D startPos = Vector3D(0., 0., 0.);
	Vector3D direction = Vector3D(0., 0., 0.);
	double startTime;
};