# Author: Allan Jales

#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import ROOT
import numpy as np
from src.EfficienciesPlot import EfficienciesPlot
from src.utils import *
from src.sigmoid import Sigmoid

def plot_efficiencies(preset_name : str):
	'''
	Overplots efficiency graphs from multiple scans utilizando Matplotlib.
	'''

	PRESET = {
		"X_MIN": 5600,
		"X_MAX": 6400,
		"Y_MAX": 1.05,
	}

	plotter = EfficienciesPlot(preset_name, PRESET["X_MIN"], PRESET["X_MAX"], PRESET["Y_MAX"])

	for path, (label, color, marker) in PRESET["SCANS"].items():
		root_path = os.path.join(READ_BASE_DIR, path, "output.root")

		if not os.path.exists(root_path):
			print(f"[ERROR] File not found: {root_path}")
			continue

		f = ROOT.TFile.Open(root_path)
		if not f or f.IsZombie():
			print(f"[ERROR] Error openning: {root_path}")
			continue

		# Load efficiency graph
		if not (eff_gr := load_TGraph_from_TFile(f, "efficiencyMuon_corrected_HVeff")):
			print("Skipping.")
			continue

		if eff_gr.GetN() <= 3:
			print(f"[WARNING] Low number of points in efficiency graph for {root_path} ({eff_gr.GetN()} points)")

		tf1 = get_attached_tf1(eff_gr)
		if not tf1:
			print(f"[WARNING] No TF1 attached to graph in {root_path}")
			f.Close()
			continue
		
		# Load noise graph
		if not (noise_gr := load_TGraph_from_TFile(f, "noiseGammaRate_HVeff")):
			print("Skipping.")
			continue
			
		# Load cluster size
		if not (gamma_cls_gr := load_TGraph_from_TFile(f, "gammaCLS_HVeff")):
			print("Skipping.")
			continue

		sigmoid = Sigmoid(tf1)
		wp = sigmoid.wp()
		eff_at_wp = sigmoid.eval(wp)
		gamma_at_wp = eval_graph_at_x(noise_gr, wp.n)
		gamma_cls_at_wp = eval_graph_at_x(gamma_cls_gr, wp.n)
		if gamma_at_wp is None or gamma_cls_at_wp is None:
			print(f"[ERROR] Could not evaluate noise or cluster size at WP for {root_path}")
			f.Close()
			continue
			
		bkg_rate_at_wp = gamma_at_wp/gamma_cls_at_wp

		# Extract points and errors from TGraph as array
		n_points = eff_gr.GetN()
		x_vals = np.array([eff_gr.GetPointX(i) for i in range(n_points)])
		y_vals = np.array([eff_gr.GetPointY(i) for i in range(n_points)])
		x_errs = np.array([eff_gr.GetErrorX(i) for i in range(n_points)]) if hasattr(eff_gr, "GetErrorX") else None
		y_errs = np.array([eff_gr.GetErrorY(i) for i in range(n_points)]) if hasattr(eff_gr, "GetErrorY") else None
		
		# Legend string
		bkg_rate_str = f"bkg rate(WP) = {(bkg_rate_at_wp).n/1000:.1f} kHz/cm$^{{2}}$"
		if label == "Source OFF":
			bkg_rate_str = "without background rate"
		legend_label = f"plateau = {sigmoid.ymax.n:.0f}%, WP = {wp.n/1000:.2f} kV, {bkg_rate_str}, Eff(WP) = {eff_at_wp.n:.0f}%"
		
		plotter.add_efficiency_points(x_vals, y_vals, x_errs, y_errs, color=color, marker=marker, label=legend_label)
		plotter.add_sigmoid(sigmoid, color=color)
		
		f.Close()

	caption = ""
	for line in PRESET["CAPTION_LIST"]:
		caption += line + "\n"
	plotter.draw([caption])
	plotter.save()
	
def main():
	plot_efficiencies("mix1_2025")
	plot_efficiencies("mix1_2025_selected")
	plot_efficiencies("mix2_2025")

if __name__ == "__main__":
	main()