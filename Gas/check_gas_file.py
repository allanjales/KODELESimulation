#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Author: Allan jales

import argparse
import ROOT
import ctypes
import matplotlib.pyplot as plt
import mplhep as hep

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
		print("OK. Not sorted, but no discrepacy greater than 1.")
	else:
		print(f"NOT OK. There are values with difference greater than {threshold}.")

def main():
	parser = argparse.ArgumentParser(description="Check .gas file and plot coefficients using Garfield++")
	parser.add_argument("filename", type=str, help="The path to the .gas file")
	parser.add_argument("-g", "--graph", action="store_true", help="Plot the coefficients graphically")
	parser.add_argument("-v", "--verbose", action="store_true", help="Verbose each point in the grid")
	parser.add_argument("-t", "--threshold", help="Threshold for consistency check", type=float, default=1.0)
	args = parser.parse_args()

	gas = ROOT.Garfield.MediumMagboltz()

	if not gas.LoadGasFile(args.filename):
		print(f"Error: Could not load {args.filename}")
		exit(1)

	eFields = ROOT.std.vector('double')()
	bFields = ROOT.std.vector('double')()
	angles  = ROOT.std.vector('double')()
	gas.GetFieldGrid(eFields, bFields, angles)

	E_vals = []
	alpha_vals = []
	eta_vals = []
	eff_vals = []

	# In PyROOT, we use ctypes to simulate passing a variable by reference from C++
	alpha = ctypes.c_double(0.0)
	eta = ctypes.c_double(0.0)

	# Iterates over each electric field point index to extract the coefficients
	for i, eField in enumerate(eFields):
		# PyROOT matches the overload that expects grid indices (size_t ie, size_t ib, size_t ia)
		# Since B field and angle are 0, their indices are 0
		gas.GetElectronTownsend(i, 0, 0, alpha)
		gas.GetElectronAttachment(i, 0, 0, eta)

		a = alpha.value
		e = eta.value

		E_vals.append(eField)
		alpha_vals.append(a)
		eta_vals.append(e)
		eff_vals.append(a - e) # Effective Townsend

		if args.verbose:
			print(f"Point {i:03d}: E = {eField:.1f}, alpha = {a:.2f}, eta = {e:.2f}, alpha - eta = {(a - e):.2f}")


	print(f"Gas Mixture: {args.filename}")
	print(f"Start : {E_vals[0]}")
	print(f"End   : {E_vals[-1]}")
	print(f"Points: {len(E_vals)}")

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

		# Adds a reference line at Y=0
		# Where the green curve crosses zero is the avalanche threshold
		plt.axhline(0, color='black', linewidth=1)

		plt.title(f'Gas Coefficients - Mixture: {args.filename}')
		plt.xlabel('Electric Field E [V/cm]')
		plt.ylabel('Coefficient [1/cm]')
		plt.legend()
		plt.grid(True, which="both", ls="-", alpha=0.3)

		# plt.yscale('symlog', linthresh=1e-2)

		plt.tight_layout()
		plt.show()

if __name__ == "__main__":
	main()