// Author: Allan Jales
// Layer class to represent a layer in detector stack at my Garfield++ project

#pragma once

#include "Garfield/Medium.hh"

struct Layer
{
	Garfield::Medium* medium;
	double thickness;
};