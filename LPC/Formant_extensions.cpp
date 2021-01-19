/* Formant_extensions.cpp
 *
 * Copyright (C) 2012-2019 David Weenink
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

#include "DataModeler.h"
#include "Formant_extensions.h"
#include "NUM2.h"

#include "enums_getText.h"
#include "Formant_extensions_enums.h"
#include "enums_getValue.h"
#include "Formant_extensions_enums.h"

void Formant_formula (Formant me, double tmin, double tmax, integer formantmin, integer formantmax, Interpreter interpreter, conststring32 expression) {
	try {
		Function_unidirectionalAutowindow (me, & tmin, & tmax);
		const integer numberOfPossibleFormants = my maxnFormants;
		if (formantmax >= formantmin) {
			formantmin = 1;
			formantmax = numberOfPossibleFormants;
		}
		Melder_clipLeft (1_integer, & formantmin);
		Melder_clipRight (& formantmax, numberOfPossibleFormants);

		autoMatrix fb = Matrix_create (my xmin, my xmax, my nx, my dx, my x1, 1.0, 2 * numberOfPossibleFormants, 2 * numberOfPossibleFormants, 1.0, 1.0);
		for (integer iframe = 1; iframe <= my nx; iframe ++) {
			const Formant_Frame frame = & my frames [iframe];
			const integer numberOfFormants = std::min (frame -> numberOfFormants, numberOfPossibleFormants);
			for (integer iformant = 1; iformant <= numberOfFormants; iformant++)
				if (iformant <= frame -> numberOfFormants) {
					fb -> z [2 * iformant - 1] [iframe] = frame -> formant [iformant]. frequency;
					fb -> z [2 * iformant    ] [iframe] = frame -> formant [iformant]. bandwidth;
				}
		}
		/*
			Apply the formaula
		*/
		const double ymin = 2.0 * formantmin - 1.0, ymax = 2.0 * formantmax;
		Matrix_formula_part (fb.get(), tmin, tmax, ymin, ymax, expression, interpreter, nullptr);
		/*
			Put results back in Formant
		*/
		integer ixmin, ixmax, iymin, iymax;
		(void) Matrix_getWindowSamplesX (fb.get(), tmin, tmax, & ixmin, & ixmax);
		(void) Matrix_getWindowSamplesY (fb.get(), ymin, ymax, & iymin, & iymax);

		for (integer iframe = ixmin; iframe <= ixmax; iframe ++) {
			/*
				If some of the formant frequencies are set to zero => remove the formant
			*/
			const Formant_Frame frame = & my frames [iframe];
			const integer numberOfFormants = std::min (frame -> numberOfFormants, formantmax);
			integer iformantto = ( formantmin > 1 ? formantmin - 1 : 0 );
			for (integer iformant = formantmin; iformant <= numberOfFormants; iformant++) {
				const double frequency = fb -> z [2 * iformant - 1] [iframe];
				const double bandWidth = fb -> z [2 * iformant    ] [iframe];
				if (frequency > 0 && bandWidth > 0) {
					iformantto ++;
					frame -> formant [iformantto]. frequency = frequency;
					frame -> formant [iformantto]. bandwidth = bandWidth;
				} else
					frame -> formant [iformant]. frequency = frame -> formant [iformant]. bandwidth = 0.0;
			}
			/*
				Shift the (higher) formants down if necessary.
			*/
			for (integer iformant = formantmax + 1; iformant <= frame -> numberOfFormants; iformant ++) {
				const double frequency = fb -> z [2 * iformant - 1] [iframe];
				const double bandWidth = fb -> z [2 * iformant    ] [iframe];
				if (frequency > 0 && bandWidth > 0) {
					iformantto ++;
					frame -> formant [iformantto]. frequency = frequency;
					frame -> formant [iformantto]. bandwidth = bandWidth;
				} else
					frame -> formant [iformant]. frequency = frame -> formant [iformant]. bandwidth = 0.0;
			}
			frame -> numberOfFormants = iformantto;
		}
	} catch (MelderError) {
		Melder_throw (me, U": not filtered.");
	}
}

autoVEC Formant_listFormantSlope (Formant me, integer iformant, double tmin, double tmax, kSlopeCurve curveType) {
	integer itmin, itmax;
	autoVEC lineFit = raw_VEC (7);
	lineFit.get()  <<=  undefined;
	const integer numberOfFrames = Sampled_getWindowSamples (me, tmin, tmax, & itmin, & itmax);
	if (numberOfFrames < 2)
		return lineFit;
	autoVEC x = raw_VEC (numberOfFrames);
	autoVEC y = raw_VEC (numberOfFrames);
	autoVEC s = raw_VEC (numberOfFrames);
	integer numberOfDataPoints = 0;
	for (integer iframe = itmin; iframe <= itmax; iframe ++) {
		const Formant_Frame frame = & my frames [iframe];
		const integer numberOfFormants = frame -> numberOfFormants;
		const double frequency = frame -> formant [iformant]. frequency;
		if (iformant > numberOfFormants || ! isdefined (frequency))
			continue;
		numberOfDataPoints ++;
		x [numberOfDataPoints] = Sampled_indexToX (me, iframe);
		y [numberOfDataPoints] = frequency;
		s [numberOfDataPoints] = frame -> formant [iformant]. bandwidth;
	}
	if (numberOfDataPoints != numberOfFrames) {
		if (numberOfDataPoints < 3)
			return lineFit;
		x.resize (numberOfDataPoints);
		y.resize (numberOfDataPoints);
		s.resize (numberOfDataPoints);
	}
	double constant, b, c, flocus, ftarget, residualVariance;
	if (curveType == kSlopeCurve::EXPONENTIAL) {// exponential
		/*
			Model formant (t) = constant + b * exp (c * t)
		*/
		NUMfitExponentialPlusConstant (x.get(), y.get(), & constant, & b, & c, & residualVariance);
		flocus 	= constant + b * exp (c * tmin);
		ftarget = constant + b * exp (c * tmax);
		lineFit [5] = constant;
		lineFit [6] = b;
		lineFit [7] = c;
	} else if (curveType == kSlopeCurve::PARABOLIC) {
		/*
			Model: formant (t) = constant + b * x + c * x^2
		*/
		autoDataModeler dm = DataModeler_create (tmin, tmax, numberOfDataPoints, 3, kDataModelerFunction::LEGENDRE);
		for (integer k = 1; k <= numberOfDataPoints; k ++) {
			dm -> data [k] .x = x [k];
			dm -> data [k] .y = y [k];
			dm -> data [k] .sigmaY = s [k];
			dm -> data [k] .status = kDataModelerData::VALID;
		}
		DataModeler_fit (dm.get());
		flocus =  DataModeler_getModelValueAtX (dm.get(), tmin);
		ftarget =  DataModeler_getModelValueAtX (dm.get(), tmax);
		residualVariance = DataModeler_getResidualSumOfSquares (dm.get(), nullptr) / (numberOfDataPoints - 1);
		lineFit [5] = dm -> parameters [1].value;
		lineFit [6] = dm -> parameters [2].value;
		lineFit [7] = dm -> parameters [3].value;
	} else if (curveType == kSlopeCurve::SIGMOID) {
		/*
			Model: formant (t) = constant + b / (1 + exp (- (x - m) / s))
		*/
		double mu, sigma;
		const double yscaleFactor = 100.0; // to improve balance between x and y value scales
		y.get()  /=  yscaleFactor;
		NUMfitLogisticPlusConstant (x.get(), y.get(), & constant, & b, & mu, & sigma, & residualVariance); // y(x) = a 
		flocus  = yscaleFactor * (constant + b / (1.0 + exp (- (tmin - mu) / sigma)));
		ftarget = yscaleFactor * (constant + b / (1.0 + exp (- (tmax - mu) / sigma)));
		lineFit.resize (8);
		lineFit [5] = yscaleFactor * constant;
		lineFit [6] = yscaleFactor * b;
		lineFit [7] = mu;
		lineFit [8] = sigma;
		residualVariance *= yscaleFactor * yscaleFactor;
	} else
		return lineFit;
	const double averageSlope =  (ftarget - flocus) / (tmax - tmin);
	lineFit [1] = averageSlope;
	lineFit [2] = 1.0 - residualVariance / NUMvariance (y.get());
	lineFit [3] = flocus;
	lineFit [4] = ftarget;
	return lineFit;
}

autoIntensityTier Formant_Spectrogram_to_IntensityTier (Formant me, Spectrogram thee, integer iformant) {
	try {
		Melder_require (my xmin == thy xmin && my xmax == thy xmax,
			U"The start and end times of the Formant and the Spectrogram should be equal.");
		Melder_require (iformant > 0 && iformant <= my maxnFormants,
			U"Formant number should be in the range [1, ", my maxnFormants, U"].");
		autoIntensityTier him = IntensityTier_create (my xmin, my xmax);
		double previousValue = -80000.0; // can never occur
		double previousTime = my xmin;
		for (integer iframe = 1; iframe <= my nx; iframe ++) {
			const Formant_Frame frame = & my frames [iframe];
			const integer numberOfFormants = frame -> numberOfFormants;
			const double time = Sampled_indexToX (me, iframe);
			double value = 0.0;
			if (iformant <= numberOfFormants) {
				const double f = frame -> formant [iformant]. frequency;
				value = Matrix_getValueAtXY (thee, time, f);
				value = isdefined (value) ? value : 0.0;
			}
			value = 10.0 * log10 ((value + 1e-30) / 4.0e-10); /* dB / Hz */
			if (value != previousValue) {
				if (iframe > 1 && previousTime < time - 1.5 * my dx)   // mark the end of the same interval
					RealTier_addPoint (him.get(), time - my dx, previousValue);
				RealTier_addPoint (him.get(), time, value);
				previousTime = time;
			}
			previousValue = value;
		}
		return him;
	} catch (MelderError) {
		Melder_throw (U"IntensityTier not created from ", me, U" and ", thee, U".");
	}
}

/* End of file Formant_extensions.cpp */
