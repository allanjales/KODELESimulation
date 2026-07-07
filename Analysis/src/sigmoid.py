# Author: Allan Jales

#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import annotations
import ROOT
from uncertainties import ufloat, umath
import numpy as np

class Sigmoid:
	def __init__(self, tf1: ROOT.TF1, scale : float = 1.):
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