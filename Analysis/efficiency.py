#!/usr/bin/env python3

# -*- coding: utf-8 -*-
# Author: Allan Jales

import os
import glob
import numpy as np
import pandas as pd
from tqdm import tqdm
from src.EfficienciesPlot import EfficienciesPlot
from src.sigmoid import Sigmoid

# Define the gas name here (this will direct all reads and writes)
# GAS_NAME = "cms_rpc_95.2_4.5_0.3_25-40kV"
# GAS_NAME = "CO2_30_SF6_1"
# GAS_NAME = "CO2_40_SF6_1"
GAS_NAME = "CO2_30_SF6_05"

# Adjustable threshold in fC for efficiency calculation
THRESHOLD_FC = 60.0

# RPC gap size in mm to convert electric field [kV/mm] to High Voltage [V]
GAP_SIZE_MM = 1.4

def extract_simulation_data(input_dir: str, threshold_fc: float, output_dir: str, filename: str) -> pd.DataFrame:
	'''
	Reads all consolidated tic_*kVmm.csv files, calculates mean induced charge,
	standard deviations, standard errors, and binomial efficiency, saving inside input_dir.
	'''

	results = []
	csv_files = glob.glob(os.path.join(input_dir, "tic_*kVmm.csv"))

	print(f"Found {len(csv_files)} files in '{input_dir}'. Extracting data...\n")

	for file_path in tqdm(sorted(csv_files), desc="Processing Electric Fields", unit="field"):

		file_name = os.path.basename(file_path)
		
		# Extract the numeric value of the electric field from string
		try:
			field_str = file_name.replace("tic_", "").replace("kVmm.csv", "")
			field_value = float(field_str)
		except ValueError:
			continue

		try:
			# Skip line 0. Line 1 becomes the actual header
			df_event = pd.read_csv(file_path, skiprows=1)

			df_event = df_event.dropna(subset=["Total Charge [fC]"])
			charges = df_event["Total Charge [fC]"].abs()
			n_events = len(charges)
			
			if n_events == 0:
				continue
				
			# Charge statistics
			mean_charge = charges.mean()
			std_charge = charges.std()
			sem_charge = charges.sem() # Standard Error of the Mean (std / sqrt(N))
			
			# Efficiency calculation
			n_passed = (charges >= threshold_fc).sum()
			efficiency = n_passed / n_events
			std_efficiency = np.sqrt(efficiency * (1.0 - efficiency))
			unc_efficiency = std_efficiency / np.sqrt(n_events)

			results.append({
				"Electric Field [kV/mm]": field_value,
				"Mean Total Charge [fC]": mean_charge,
				"StdDev Total Charge [fC]": std_charge,
				"SEM Total Charge [fC]": sem_charge,
				"Efficiency": efficiency,
				"StdDev Efficiency": std_efficiency,
				"Uncertainty Efficiency": unc_efficiency,
				"N Events": n_events
			})
		except Exception as e:
			print(f"\n[WARNING] Could not read {file_name}: {e}")

	df = pd.DataFrame(results)

	if not df.empty:
		df = df.sort_values(by="Electric Field [kV/mm]")
		print("\n")
		print(df.to_string(index=False))
		
		# Save the csv
		os.makedirs(output_dir, exist_ok=True)
		output_csv = os.path.join(output_dir, filename)
		df.to_csv(output_csv, index=False)
		print(f"\nTable successfully saved to '{output_csv}'!")
	else:
		print("\n[ERROR] No valid data found to process.")

	return df

def plot_simulated_efficiency(df: pd.DataFrame, output_dir: str, preset_name: str, gap_size_mm: float = 1.4):
	'''
	Converts the electric field to HV based on gap size and plots the 
	efficiency curve with its sigmoid fit, saving inside output_dir.
	'''
	if df.empty:
		print("[ERROR] DataFrame is empty. Cannot generate plot.")
		return

	# Convert Electric Field [kV/mm] to High Voltage [V]
	x_vals = df["Electric Field [kV/mm]"].values * gap_size_mm * 1000.0
	
	# As percentage [%]
	y_vals = df["Efficiency"].values * 100.0
	y_errs = df["Uncertainty Efficiency"].values * 100.0
	x_errs = np.zeros_like(x_vals)

	plot_save_path = os.path.join(output_dir, preset_name)

	# plotter = EfficienciesPlot(plot_save_path, x_min=min(x_vals) - 100, x_max=max(x_vals) + 100, y_max=110)	
	plotter = EfficienciesPlot(preset_name, x_min=6100, x_max=7625, y_max=110)
	
	legend_label = "Simulated Efficiency"
	
	# Fit Sigmoid and get labels
	try:
		sigmoid_curve = Sigmoid()
		sigmoid_curve.fit(x_vals, y_vals, y_errs)
		plotter.add_sigmoid(sigmoid_curve, color="red")
		
		wp = sigmoid_curve.wp()
		eff_wp = sigmoid_curve.eval(wp)
		
		legend_label = rf"plateau = ({sigmoid_curve.ymax.n:.1f} $\pm$ {sigmoid_curve.ymax.s:.1f}) %, " \
					   rf"WP = ({wp.n:.0f} $\pm$ {wp.s:.0f}) V, " \
					   rf"Eff(WP) = ({eff_wp.n:.1f} $\pm$ {eff_wp.s:.1f}) %"
	except Exception as e:
		print(f"[WARNING] Could not fit the sigmoid: {e}")
	
	plotter.add_efficiency_points(x_vals, y_vals, x_errs, y_errs, color="red", marker="o", label=legend_label)

	caption_list = [
		f"Gas: {GAS_NAME}",
		"Without background rate",
		f"Threshold = {THRESHOLD_FC} fC",
		f"{gap_size_mm} mm gap RPC",
	]

	plotter.draw(caption_list)
	plotter.save()
	print(f"Plot successfully saved to '{plot_save_path}'!")

def main():
	# input_dir = f"results/{GAS_NAME}/"
	input_dir = f"../Garfield/results/{GAS_NAME}/"
	output_dir = f"output/"
	df_results = extract_simulation_data(input_dir, THRESHOLD_FC, output_dir, filename=f"{GAS_NAME}_Efficiency_Results.csv")
	
	if not df_results.empty:
		print("\nGenerating efficiency curve plot...")
		plot_simulated_efficiency(df_results, output_dir, f"{GAS_NAME}_Efficiency_Curve", gap_size_mm=GAP_SIZE_MM)

if __name__ == "__main__":
	main()