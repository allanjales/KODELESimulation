# Author: Allan Jales

#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations
import ROOT
from uncertainties import ufloat, umath
import numpy as np

class Sigmoid:
	def __init__(self, tf1: ROOT.TF1 = None, scale : float = 1.):
		# Allows empty instantiation if no TF1 is provided
		if tf1 is not None:
			self.set_parameters(tf1, scale)
	
	def set_parameters(self, tf1: ROOT.TF1, scale : float = 1.):
		'''
		Assuming TF1 is a sigmoid, returns:
		ymax, lambda, HV50 with uncertainties
		'''
		self.ymax    = ufloat(tf1.GetParameter(0), tf1.GetParError(0)) * scale
		self.lambda_ = ufloat(tf1.GetParameter(1), tf1.GetParError(1))
		self.hv_50   = ufloat(tf1.GetParameter(2), tf1.GetParError(2))

	def eval(self, x: ufloat | float) -> ufloat:
		'''
		Evaluates the sigmoid function at a given x with uncertainties
		'''
		if isinstance(x, float):
			x = ufloat(x, 0.)

		return self.ymax / (1. + umath.exp(-self.lambda_ * (x - self.hv_50)))

	def function(self, x: np.ndarray) -> np.ndarray:
		'''
		Evaluates the sigmoid function at a given x with uncertainties
		'''
		return self.ymax.n / (1. + np.exp(-self.lambda_.n * (x - self.hv_50.n)))

	def hv_knee(self) -> ufloat:
		'''
		Calculates the HV_knee: ln(19)/lambda + HV50
		'''
		return self.hv_50 + umath.log(19) / self.lambda_

	def wp(self, v0: float = 150.) -> ufloat:
		'''
		Calculates the working point: HV_knee + v0
		'''
		return self.hv_knee() + v0

	def fit(self, x_vals: np.ndarray, y_vals: np.ndarray, y_errs: np.ndarray = None, scale: float = 1.):
		'''
		Fits a sigmoid to the given data arrays using ROOT and ONLY updates the internal variables.
		'''
		n_points = len(x_vals)

		# [FIX 1] Prevent exact 0.0 errors from trapping the MINUIT algorithm.
		# A tiny error like 1e-5 creates an infinitely deep chi-square well, forcing a step function.
		# We use 0.5% as a reasonable minimum error floor so the fit can "breathe".
		if y_errs is not None:
			y_errs_safe = np.where(y_errs < 0.5, 0.5, y_errs)
		else:
			y_errs_safe = np.full(n_points, 1.0)
			
		x_errs = np.zeros(n_points)
		
		# Arrays must be float64 to interface correctly with ROOT C++
		x_arr = np.array(x_vals, dtype=np.float64)
		y_arr = np.array(y_vals, dtype=np.float64)
		ex_arr = np.array(x_errs, dtype=np.float64)
		ey_arr = np.array(y_errs_safe, dtype=np.float64)
		
		gr = ROOT.TGraphErrors(n_points, x_arr, y_arr, ex_arr, ey_arr)
		
		# Define the Sigmoid equation: p0 / (1 + exp(-p1 * (x - p2)))
		tf1 = ROOT.TF1("sigmoid_fit", "[0] / (1. + TMath::Exp(-[1] * (x - [2])))", x_arr.min(), x_arr.max())
		
		# [FIX 2] Better initial guess for HV50 based on the actual data
		ymax_guess = np.max(y_arr)
		idx_50 = np.argmin(np.abs(y_arr - (ymax_guess / 2.0)))
		hv50_guess = x_arr[idx_50]
		
		tf1.SetParameter(0, ymax_guess)      # ymax
		tf1.SetParameter(1, 0.005)           # lambda
		tf1.SetParameter(2, hv50_guess)      # HV50
		
		# [FIX 3] Set physical limits so parameters don't explode to infinity
		tf1.SetParLimits(0, ymax_guess * 0.8, 105.0)  # Efficiency max limit
		tf1.SetParLimits(1, 0.0001, 0.1)              # Lambda slope limits (prevents step functions)
		tf1.SetParLimits(2, x_arr.min(), x_arr.max()) # HV50 must be inside the scanned range
		
		# Perform the fit quietly (Q) and do not draw automatically (0)
		gr.Fit(tf1, "Q0")
		
		# EXPLICITLY set only the internal variables (No return statement)
		self.ymax    = ufloat(tf1.GetParameter(0), tf1.GetParError(0)) * scale
		self.lambda_ = ufloat(tf1.GetParameter(1), tf1.GetParError(1))
		self.hv_50   = ufloat(tf1.GetParameter(2), tf1.GetParError(2))

		# Nicely formatted info print
		print(f"[INFO] Sigmoid fit results: ymax = {self.ymax.n:.2f}, lambda = {self.lambda_.n:.5f}, HV50 = {self.hv_50.n:.2f}")