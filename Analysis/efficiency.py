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

# Adjustable threshold in fC for efficiency calculation
THRESHOLD_FC = 60.0

# RPC gap size in mm to convert electric field [kV/mm] to High Voltage [V]
GAP_SIZE_MM = 1.4

def extract_simulation_data(input_dir: str, threshold_fc: float, output_dir: str, foldername: str) -> pd.DataFrame:
	'''
	Reads all consolidated tic_*kVmm.csv files, calculates mean induced charge,
	standard deviations, standard errors, and binomial efficiency, saving inside input_dir.
	'''

	results = []
	csv_files = glob.glob(os.path.join(input_dir, "tic_*kVmm.csv"))

	print(f"Found {len(csv_files)} files in '{input_dir}'. Extracting data...")

	for file_path in csv_files:

		file_name = os.path.basename(file_path)
		
		# Extract the numeric value of the electric field from string
		field_str = file_name.replace("tic_", "").replace("kVmm.csv", "")
		field_value = float(field_str)

		try:
			# Skip line 0. Line 1 becomes the actual header
			df_event = pd.read_csv(file_path, skiprows=1)

			df_event = df_event.dropna(subset=["Total Charge [fC]"])
			charges = df_event["Total Charge [fC]"].abs()
			n_events = len(charges)
			
			if n_events == 0:
				print(f"[WARNING] No valid events found in {file_name}. Skipping this file.")
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
			print(f"[WARNING] Could not read {file_name}: {e}")

	df = pd.DataFrame(results)

	if not df.empty:
		df = df.sort_values(by="Electric Field [kV/mm]")
		
		# Save the csv
		os.makedirs(output_dir, exist_ok=True)
		output_csv = os.path.join(output_dir, foldername)
		df.to_csv(output_csv, index=False)
		print(f"Table successfully saved to '{output_csv}'!")
	else:
		print("[ERROR] No valid data found to process.")

	return df

def main():
	gases_list = [
		{
			"foldername": "cms_rpc_95.2_4.5_0.3_25-40kV",
			"label": "Standard gas mixture",
			"color": "magenta",
			"marker": "v"
		},
		{
			"foldername": "CO2_30_SF6_1",
			"label": "CO$_{2}$ 30% + SF$_{6}$ 1.0%",
			"color": "green",
			"marker": "^"
		},
		{
			"foldername": "CO2_30_SF6_05",
			"label": "CO$_{2}$ 30% + SF$_{6}$ 0.5%",
			"color": "red",
			"marker": "o"
		},
		{
			"foldername": "CO2_40_SF6_1",
			"label": "CO$_{2}$ 40% + SF$_{6}$ 1.0%",
			"color": "blue",
			"marker": "s"
		},
	]
	preset_name = "efficiency_curves"

	output_dir = f"output/"
	plotter = EfficienciesPlot(preset_name, x_min=6100, x_max=7625, y_max=128)

	# Plot each gas
	for gas in gases_list:
		input_dir = f"../Garfield/results/{gas['foldername']}/"
		df = extract_simulation_data(input_dir, THRESHOLD_FC, output_dir, foldername=f"{gas['foldername']}_{preset_name}.csv")

		if df.empty:
			print(f"[WARNING] No valid data found for gas '{gas['label']}' ({gas['foldername']}). Skipping plot generation.")
			continue

		# Convert Electric Field [kV/mm] to High Voltage [V]
		x_vals = df["Electric Field [kV/mm]"].values * GAP_SIZE_MM * 1000.0
		x_errs = np.zeros_like(x_vals)
		
		# As percentage [%]
		y_vals = df["Efficiency"].values * 100.0
		y_errs = df["Uncertainty Efficiency"].values * 100.0

		plot_save_path = os.path.join(output_dir, preset_name)

		legend_label = "Simulated Efficiency"

		# Fit Sigmoid and get labels
		sigmoid = Sigmoid()
		sigmoid.fit(x_vals, y_vals, y_errs)
		plotter.add_sigmoid(sigmoid, color=gas['color'])
		
		wp = sigmoid.wp()
		eff_wp = sigmoid.eval(wp)
		
		# legend_label = rf"{gas['label']} plateau = ({sigmoid.ymax.n:.1f} $\pm$ {sigmoid.ymax.s:.1f}) %, " \
		# 			   rf"WP = ({wp.n:.0f} $\pm$ {wp.s:.0f}) V, " \
		# 			   rf"Eff(WP) = ({eff_wp.n:.1f} $\pm$ {eff_wp.s:.1f}) %"

		legend_label = rf"plateau = {sigmoid.ymax.n:.0f}%, WP = {wp.n:.0f} V, {gas['label']}, Eff(WP) = {eff_wp.n:.0f}%"
		
		plotter.add_efficiency_points(x_vals, y_vals, x_errs, y_errs, color=gas['color'], marker=gas['marker'], label=legend_label)

	caption_list = [
		"Without background rate",
		f"Threshold = {THRESHOLD_FC} fC",
		f"{GAP_SIZE_MM} mm gap RPC",
	]

	plotter.draw(caption_list)
	plotter.save()
	print(f"Plot successfully saved to '{plot_save_path}'!")

if __name__ == "__main__":
	main()