// Author: Allan Jales
// Units factor conversion for Garfield++

#pragma once

// Length
constexpr double mm = 0.1;
constexpr double cm = 1.; // default
constexpr double m  = 100.;

// Voltage
constexpr double V  = 1.; // default
constexpr double kV = 1e3;

// Time
constexpr double ns = 1.; // default
constexpr double us = 1.e3;
constexpr double ms = 1.e6;
constexpr double s  = 1.e9;

// electron-volt
constexpr double eV  = 1.; // default
constexpr double keV = 1.e3;
constexpr double MeV = 1.e6;
constexpr double GeV = 1.e9;