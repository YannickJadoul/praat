#ifndef _Spectrum_and_MultiSampledSpectrogram_h_
#define _Spectrum_and_MultiSampledSpectrogram_h_
/* Spectrum_and_MultiSampledSpectrogram.h
 * 
 * Copyright (C) 2021-2022 David Weenink
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

#include "AnalyticSound.h"
#include "Sound.h"
#include "Spectrum.h"
#include "MultiSampledSpectrogram.h"

void Spectrum_into_MultiSampledSpectrogram (Spectrum me, MultiSampledSpectrogram thee, double approximateTimeOverSampling,
	kSound_windowShape filterShape);

autoSpectrum MultiSampledSpectrogram_to_Spectrum (MultiSampledSpectrogram me);

autoSpectrum Sound_and_MultiSampledSpectrogram_to_Spectrum (Sound me, MultiSampledSpectrogram thee);

autoSound MultiSampledSpectrogram_to_Sound_frequencyBin (MultiSampledSpectrogram me, integer frequencyBinNumber);

autoAnalyticSound MultiSampledSpectrogram_to_AnalyticSound_frequencyBin (MultiSampledSpectrogram me, integer frequencyBinNumber);

#endif /* Spectrum_and_MultiSampledSpectrogram_h_ */
