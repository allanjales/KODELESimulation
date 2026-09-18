#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Author: Allan jales
# Description: This script checks a .gas file and plots the coefficients using Garfield++.

import argparse
import ROOT
import ctypes
import matplotlib.pyplot as plt
import mplhep as hep
from matplotlib.offsetbox import AnchoredText

# Garfield library load
ROOT.gSystem.Load("libGarfield.so")

# Disable interactive graphs
plt.ioff()

def are_values_consistent(vals : list[float], threshold : float = 1.0) -> bool:
	"""
	Check if a list of values has consistency behaviour.
	It detects if there is values with difference greater than the threshold given.

	Returns:
	bool: True if the values are consistent, False otherwise.
	"""
	for i in range(1, len(vals)):
		if abs(vals[i] - vals[i-1]) > threshold:
			return False
	return True

def print_consistency(name : str, vals : list[float], threshold : float = 1.0, reverse : bool = False) -> None:
	print(f"{name}", end="")
	if vals == sorted(vals, reverse=reverse):
		print("Too Perfect. Sorted.")
	elif are_values_consistent(vals, threshold=threshold):
		print(f"OK. Not sorted, but no discrepacy greater than {threshold}.")
	else:
		print(f"NOT OK. There are values with difference greater than {threshold}.")

def main():
	parser = argparse.ArgumentParser(description="Check .gas file and plot coefficients using Garfield++ for different electric fields")
	parser.add_argument("filename", type=str, help="The path to the .gas file")
	parser.add_argument("-g", "--graph", action="store_true", help="Plot the coefficients graphically")
	parser.add_argument("-l", "--log", action="store_true", help="Set graph Y-axis to logarithmic scale")
	parser.add_argument("-v", "--verbose", action="store_true", help="Verbose each point in the grid")
	parser.add_argument("-t", "--threshold", help="Threshold for consistency check", type=float, default=1.0)
	args = parser.parse_args()

	gas = ROOT.Garfield.MediumMagboltz()

	if not gas.LoadGasFile(args.filename):
		print(f"Error: Could not load {args.filename}")
		exit(1)

	pressure = gas.GetPressure()
	temperature = gas.GetTemperature()

	components = []
	label = ROOT.std.string()
	frac = ctypes.c_double(0.0)
	for i in range(gas.GetNumberOfComponents()):
		gas.GetComponent(i, label, frac)
		components.append(f"{str(label)} {frac.value:g}")
	components = " ".join(components)

	eFields = ROOT.std.vector('double')()
	bFields = ROOT.std.vector('double')()
	angles  = ROOT.std.vector('double')()
	gas.GetFieldGrid(eFields, bFields, angles)

	E_vals = []
	alpha_vals = []
	eta_vals = []
	eff_vals = []

	# c_types simulate passing a variable by reference form C++
	alpha = ctypes.c_double(0.0)
	eta = ctypes.c_double(0.0)

	for i, eField in enumerate(eFields):
		# Electric field, Magnetic field and angle.
		gas.GetElectronTownsend(i, 0, 0, alpha)
		gas.GetElectronAttachment(i, 0, 0, eta)

		a = alpha.value
		e = eta.value

		E_vals.append(eField)
		alpha_vals.append(a)
		eta_vals.append(e)
		eff_vals.append(a - e)

		if args.verbose:
			print(f"Point {i:03d}: E = {eField:.1f}, alpha = {a:.2f}, eta = {e:.2f}, alpha - eta = {(a - e):.2f}")

	print("-------- Summary --------")
	print(f"File path  : {args.filename}")
	print(f"Gas Mixture: {components}")
	print(f"Temperature: {temperature:.2f} K ({(temperature-273.15):.2f} °C)")
	print(f"Pressure   : {pressure:.2f} Torr ({pressure * 1.33322368:.2f} mbar)")
	print(f"Start : {E_vals[0]} V/cm")
	print(f"End   : {E_vals[-1]} V/cm")
	print(f"Points: {len(E_vals)}")
	print("-------------------------")

	if len(E_vals) > 1:
		print_consistency("Effective Townsend Coefficient (alpha - eta)... ", eff_vals, threshold=args.threshold)
		print_consistency("Townsend Coefficient (alpha)...                 ", alpha_vals, threshold=args.threshold)
		print_consistency("Attachment Coefficient (eta)...                 ", eta_vals, threshold=args.threshold, reverse=True)

	# ==========================================
	# Matplotlib plot
	# ==========================================
	if args.graph:
		plt.figure(figsize=(10, 6))

		plt.plot(E_vals, alpha_vals, label=r'Townsend ($\alpha$)', color='tab:blue', marker='o', markersize=4)
		plt.plot(E_vals, eta_vals, label=r'Attachment ($\eta$)', color='tab:red', marker='s', markersize=4)
		plt.plot(E_vals, eff_vals, label=r'Effective ($\alpha - \eta$)', color='tab:green', linestyle='--')

		plt.title(f'Gas Coefficients - Mixture: {args.filename} @ {temperature:.2f} K, {pressure:.2f} Torr')
		plt.xlabel('Electric Field E [V/cm]')
		plt.ylabel('Coefficient [1/cm]')
		plt.legend()

		plt.grid(True, which="both", ls="-", alpha=0.3)

		if args.log:
			plt.yscale('symlog', linthresh=1e-2)

		plt.tight_layout()
		plt.show()

if __name__ == "__main__":
	main()