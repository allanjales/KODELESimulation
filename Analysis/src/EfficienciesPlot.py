# Author: Allan Jales

#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import mplhep as hep
from src.sigmoid import Sigmoid

# Garante renderização não interativa (bom para servidores/batch)
matplotlib.use("Agg")

class EfficienciesPlot:
	def __init__(self, output_filename : str, x_min : float, x_max : float, y_max : float = 1.45):
		self.save_base_dir = "output/"
		self.output_filename = output_filename
		
		# Apply CMS style from mplhep
		hep.style.use("CMS")
		
		self.fig, self.ax = plt.subplots(figsize=(10, 10))
		self.fig.subplots_adjust(left=0.10, right=0.96, bottom=0.08, top=0.94)
		
		self.ax.set_xlabel("HV$_{eff}$ [V]", fontsize=22, fontweight="bold")
		self.ax.set_ylabel("Efficiency [%]", fontsize=22, fontweight="bold")
		self.ax.tick_params(axis="both", labelsize=22)
		plt.setp(self.ax.get_xticklabels(), fontweight='bold')
		plt.setp(self.ax.get_yticklabels(), fontweight='bold')
		self.ax.set_xlim(x_min, x_max)
		self.ax.set_ylim(0, y_max)
		
		# Ticks on left and right sides
		self.ax.tick_params(top=False, right=True, which="both", direction="in")
		
		# CMS Preliminary
		hep.cms.label("Simulation", data=True, ax=self.ax, loc=0, fontsize=("large", "medium", "x-large", "medium"),
			rlabel="Garfield++", fontweight=("bold", "normal", "bold", "normal"))
		
		# Horizontal line at y=100
		self.ax.axhline(100., color="black", linestyle="--", linewidth=3., alpha=0.8)


	def add_efficiency_points(self, x_values, y_values, x_errors, y_errors, color, marker, label):
		'''
		Add efficiency points with their respective error bars.
		'''
		self.ax.errorbar(x_values, y_values, xerr=x_errors, yerr=y_errors,
			fmt=marker, color=color, markersize=7.5, linewidth=2, capsize=0, elinewidth=1.5, label=label)


	def add_sigmoid(self, sigmoid:Sigmoid, color: str):
		'''
		Adds a sigmoid curve to the plot based on the given parameters (ymax, lambda, HV50).
		'''
		(x_min, x_max) = self.ax.get_xlim()

		x_fit = np.linspace(x_min, x_max, 500)
		y_fit = sigmoid.function(x_fit)
		
		self.ax.plot(x_fit, y_fit, color=color, linestyle="-", linewidth=1.5)


	def draw(self, caption_list: list[str]):
		# Draw legend
		self.ax.legend(loc="upper left", fontsize=18)
		
		# Remove y-axis tick label above 100
		yticks = self.ax.get_yticks()
		self.ax.yaxis.set_major_locator(ticker.FixedLocator(yticks))
		labels = [f"{t:g}" for t in yticks]
		to_remove = [i for i, t in enumerate(yticks) if t > 100.]
		for i in to_remove:
			labels[i] = ""
		self.ax.set_yticklabels(labels)
		
		# Draw captions
		x_caption = 0.15
		y_caption = 0.65
		for i, line in enumerate(caption_list):
			self.fig.text(x_caption, y_caption - (i * 0.04), line, fontsize=20, ha="left", va="top", transform=self.fig.transFigure)
		
		self.fig.canvas.draw()


	def save(self):
		OUT_PNG = os.path.join(self.save_base_dir, f"{self.output_filename}.png")
		OUT_PDF = os.path.join(self.save_base_dir, f"{self.output_filename}.pdf")

		for path in [OUT_PNG, OUT_PDF]:
			os.makedirs(os.path.dirname(path), exist_ok=True)

		self.fig.savefig(OUT_PNG, dpi=120)
		print(f"Graph saved at {OUT_PNG}")
		self.fig.savefig(OUT_PDF)
		print(f"Graph saved at {OUT_PDF}")

		plt.close(self.fig)