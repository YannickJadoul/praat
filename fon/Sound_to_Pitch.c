/* Sound_to_Pitch.c
 *
 * Copyright (C) 1992-2006 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * pb 2002/07/16 GPL
 * pb 2002/10/11 removed some assertions
 * pb 2003/05/20 default time step is four times oversampling
 * pb 2003/07/02 checks on NUMrealft
 * pb 2004/05/10 better error messages
 * pb 2004/10/18 auto maxnCandidates
 * pb 2004/10/18 use of constant FFT tables speeds up AC method by a factor of 1.9
 * pb 2006/12/31 compatible with stereo sounds
 */

#include "Sound_to_Pitch.h"
#include "NUM2.h"

#define AC_HANNING  0
#define AC_GAUSS  1
#define FCC_NORMAL  2
#define FCC_ACCURATE  3
#define SCC_NORMAL  4
#define SCC_ACCURATE  5

Pitch Sound_to_Pitch_any (Sound me,
	double dt, double minimumPitch, double periodsPerWindow, int maxnCandidates,
	int method,
	double silenceThreshold, double voicingThreshold,
	double octaveCost, double octaveJumpCost, double voicedUnvoicedCost, double ceiling)
{
	double duration, t1;
	Pitch thee = NULL;
	long i, j;
	double dt_window;   /* Window length in seconds. */
	long nsamp_window, halfnsamp_window;   /* Number of samples per window. */
	long nFrames, minimumLag, maximumLag;
	long iframe, nsampFFT, *imax = NULL;
	double *frame = NULL, *r = NULL, *window = NULL, *windowR = NULL, globalPeak;
	double interpolation_depth;
	long nsamp_period, halfnsamp_period;   /* Number of samples in longest period. */
	long brent_ixmax, brent_depth;
	double brent_accuracy;   /* Obsolete. */
	struct NUMfft_Table_d fftTable = { 0 };

	Melder_assert (maxnCandidates >= 2);
	Melder_assert (method >= AC_HANNING && method <= FCC_ACCURATE);

	if (maxnCandidates < ceiling / minimumPitch) maxnCandidates = ceiling / minimumPitch;

	if (dt <= 0.0) dt = periodsPerWindow / minimumPitch / 4.0;   /* e.g. 3 periods, 75 Hz: 10 milliseconds. */

	Melder_progress (0.0, "Sound to Pitch...");
	switch (method) {
		case AC_HANNING:
			brent_depth = NUM_PEAK_INTERPOLATE_SINC70;
			brent_accuracy = 1e-7;
			interpolation_depth = 0.5;
			break;
		case AC_GAUSS:
			periodsPerWindow *= 2;   /* Because Gaussian window is twice as long. */
			brent_depth = NUM_PEAK_INTERPOLATE_SINC700;
			brent_accuracy = 1e-11;
			interpolation_depth = 0.25;   /* Because Gaussian window is twice as long. */
			break;
		case FCC_NORMAL:
			brent_depth = NUM_PEAK_INTERPOLATE_SINC70;
			brent_accuracy = 1e-7;
			interpolation_depth = 1.0;
			break;
		case FCC_ACCURATE:
			brent_depth = NUM_PEAK_INTERPOLATE_SINC700;
			brent_accuracy = 1e-11;
			interpolation_depth = 1.0;
			break;
	}
	duration = my dx * my nx;
	if (minimumPitch < periodsPerWindow / duration) {
		Melder_error ("For this Sound, the parameter 'minimum pitch'\n"
			"may not be less than %.8g Hz.", periodsPerWindow / duration);
		goto end;
	}

	/*
	 * Determine the number of samples in the longest period.
	 * We need this to compute the local mean of the sound (looking one period in both directions),
	 * and to compute the local peak of the sound (looking half a period in both directions).
	 */
	nsamp_period = floor (1 / my dx / minimumPitch);
	halfnsamp_period = nsamp_period / 2 + 1;

	if (ceiling > 0.5 / my dx) ceiling = 0.5 / my dx;

	/*
	 * Determine window length in seconds and in samples.
	 */
	dt_window = periodsPerWindow / minimumPitch;
	nsamp_window = floor (dt_window / my dx);
	halfnsamp_window = nsamp_window / 2 - 1;
	if (halfnsamp_window < 2) {
		Melder_errorp ("Analysis window too short.");
		goto end;
	}
	nsamp_window = halfnsamp_window * 2;

	/*
	 * Determine the minimum and maximum lags.
	 */
	minimumLag = floor (1 / my dx / ceiling);
	if (minimumLag < 2) minimumLag = 2;
	maximumLag = floor (nsamp_window / periodsPerWindow) + 2;
	if (maximumLag > nsamp_window) maximumLag = nsamp_window;

	/*
	 * Determine the number of frames.
	 * Fit as many frames as possible symmetrically in the total duration.
	 * We do this even for the forward cross-correlation method,
	 * because that allows us to compare the two methods.
	 */
	if (! Sampled_shortTermAnalysis (me, method >= FCC_NORMAL ? 1 / minimumPitch + dt_window : dt_window, dt, & nFrames, & t1))
		goto end;

	/*
	 * Create the resulting pitch contour.
	 */
	thee = Pitch_create (my xmin, my xmax, nFrames, dt, t1, ceiling, maxnCandidates); cherror

	/* Step 1: compute global absolute peak for determination of silence threshold. */
	{
		double mean = 0.0;
		globalPeak = 0;
		for (i = 1; i <= my nx; i ++) mean += Sampled_getValueAtSample (me, i, Sound_LEVEL_MONO, 0);
		mean /= my nx;
		for (i = 1; i <= my nx; i ++) {
			double damp = Sampled_getValueAtSample (me, i, Sound_LEVEL_MONO, 0) - mean;
			if (fabs (damp) > globalPeak) globalPeak = fabs (damp);
		}
		if (globalPeak == 0.0) { Melder_progress (1.0, NULL); return thee; }
	}

	if (method >= FCC_NORMAL) {   /* For cross-correlation analysis. */

		/*
		* Create buffer for cross-correlation analysis.
		*/
		frame = NUMdvector (1, nsamp_window); cherror

		brent_ixmax = nsamp_window * interpolation_depth;

	} else {   /* For autocorrelation analysis. */

		/*
		* Compute the number of samples needed for doing FFT.
		* To avoid edge effects, we have to append zeroes to the window.
		* The maximum lag considered for maxima is maximumLag.
		* The maximum lag used in interpolation is nsamp_window * interpolation_depth.
		*/
		nsampFFT = 1; while (nsampFFT < nsamp_window * (1 + interpolation_depth)) nsampFFT *= 2;

		/*
		* Create buffers for autocorrelation analysis.
		*/
		frame = NUMdvector (1, nsampFFT); cherror
		windowR = NUMdvector (1, nsampFFT); cherror
		window = NUMdvector (1, nsamp_window); cherror
		NUMfft_Table_init_d (& fftTable, nsampFFT); cherror

		/*
		* A Gaussian or Hanning window is applied against phase effects.
		* The Hanning window is 2 to 5 dB better for 3 periods/window.
		* The Gaussian window is 25 to 29 dB better for 6 periods/window.
		*/
		if (method == AC_GAUSS) {   /* Gaussian window. */
			double imid = 0.5 * (nsamp_window + 1), edge = exp (-12.0);
			for (i = 1; i <= nsamp_window; i ++)
				window [i] = (exp (-48.0 * (i - imid) * (i - imid) /
					(nsamp_window + 1) / (nsamp_window + 1)) - edge) / (1 - edge);
		} else {   /* Hanning window. */
			for (i = 1; i <= nsamp_window; i ++)
				window [i] = 0.5 - 0.5 * cos (i * 2 * NUMpi / (nsamp_window + 1));
		}

		/*
		* Compute the normalized autocorrelation of the window.
		*/
		for (i = 1; i <= nsamp_window; i ++) windowR [i] = window [i];
		NUMfft_forward_d (& fftTable, windowR);
		windowR [1] *= windowR [1];   /* DC component. */
		for (i = 2; i < nsampFFT; i += 2) {
			windowR [i] = windowR [i] * windowR [i] + windowR [i+1] * windowR [i+1];
			windowR [i + 1] = 0.0;   /* Power spectrum: square and zero. */
		}
		windowR [nsampFFT] *= windowR [nsampFFT];   /* Nyquist frequency. */
		NUMfft_backward_d (& fftTable, windowR);   /* Autocorrelation. */
		for (i = 2; i <= nsamp_window; i ++) windowR [i] /= windowR [1];   /* Normalize. */
		windowR [1] = 1.0;   /* Normalize. */

		brent_ixmax = nsamp_window * interpolation_depth;
	}

	if (! (r = NUMdvector (- nsamp_window, nsamp_window)) ||
	    ! (imax = NUMlvector (1, maxnCandidates))) { forget (thee); goto end; }

	for (iframe = 1; iframe <= nFrames; iframe ++) {
		Pitch_Frame pitchFrame = & thy frame [iframe];
		double t = Sampled_indexToX (thee, iframe), localMean, localPeak;
		long leftSample = Sampled_xToLowIndex (me, t), rightSample = leftSample + 1;
		long startSample, endSample;
		if (! Melder_progress (0.1 + (0.8 * iframe) / (nFrames + 1),
			"Sound to Pitch: analysis of frame %ld out of %ld", iframe, nFrames)) { forget (thee); goto end; }

		/*
		 * Compute the local mean; look one longest period to both sides.
		 */
		localMean = 0.0;
		startSample = rightSample - nsamp_period;
		endSample = leftSample + nsamp_period;
		Melder_assert (startSample >= 1);
		Melder_assert (endSample <= my nx);
		for (i = startSample; i <= endSample; i ++) localMean += Sampled_getValueAtSample (me, i, Sound_LEVEL_MONO, 0);
		localMean /= 2 * nsamp_period;

		/*
		 * Copy a window to a frame and subtract the local mean.
		 * We are going to kill the DC component before windowing.
		 */
		startSample = rightSample - halfnsamp_window;
		endSample = leftSample + halfnsamp_window;
		Melder_assert (startSample >= 1);
		Melder_assert (endSample <= my nx);
		if (method >= FCC_NORMAL) {
			for (j = 1, i = startSample; j <= nsamp_window; j ++)
				frame [j] = (Sampled_getValueAtSample (me, i ++, Sound_LEVEL_MONO, 0) - localMean);
		} else {
			for (j = 1, i = startSample; j <= nsamp_window; j ++)
				frame [j] = (Sampled_getValueAtSample (me, i ++, Sound_LEVEL_MONO, 0) - localMean) * window [j];
			for (j = nsamp_window + 1; j <= nsampFFT; j ++)
				frame [j] = 0.0;
		}

		/*
		 * Compute the local peak; look half a longest period to both sides.
		 */
		localPeak = 0;
		if ((startSample = halfnsamp_window + 1 - halfnsamp_period) < 1) startSample = 1;
		if ((endSample = halfnsamp_window + halfnsamp_period) > nsamp_window) endSample = nsamp_window;
		for (j = startSample; j <= endSample; j ++)
			if (fabs (frame [j]) > localPeak) localPeak = fabs (frame [j]);
		pitchFrame->intensity = localPeak > globalPeak ? 1 : localPeak / globalPeak;

		/*
		 * Compute the correlation into the array 'r'.
		 */
		if (method >= FCC_NORMAL) {
			double startTime = t - (1 / minimumPitch + dt_window) / 2;
			double sumx2 = 0, sumy2 = 0;   /* Sum of squares. */
			long localSpan = maximumLag + nsamp_window, localMaximumLag, offset;
			if ((startSample = Sampled_xToLowIndex (me, startTime)) < 1) startSample = 1;
			if (localSpan > my nx + 1 - startSample) localSpan = my nx + 1 - startSample;
			localMaximumLag = localSpan - nsamp_window;
			offset = startSample - 1;
			for (i = 1; i <= nsamp_window; i ++) {
				double x = Sampled_getValueAtSample (me, offset + i, Sound_LEVEL_MONO, 0);
				sumx2 += x * x;
			}
			sumy2 = sumx2;   /* At zero lag, these are still equal. */
			r [0] = 1.0;
			for (i = 1; i <= localMaximumLag; i ++) {
				double product = 0.0;
				double y0 = Sampled_getValueAtSample (me, offset + i, Sound_LEVEL_MONO, 0);
				double yZ = Sampled_getValueAtSample (me, offset + i + nsamp_window, Sound_LEVEL_MONO, 0);
				sumy2 += yZ * yZ - y0 * y0;
				for (j = 1; j <= nsamp_window; j ++) {
					double x = Sampled_getValueAtSample (me, offset + j, Sound_LEVEL_MONO, 0);
					double y = Sampled_getValueAtSample (me, offset + i + j, Sound_LEVEL_MONO, 0);
					product += x * y;
				}
				r [- i] = r [i] = product / sqrt (sumx2 * sumy2);
			}
		} else {

			/*
			 * The FFT of the autocorrelation is the power spectrum.
			 */
			NUMfft_forward_d (& fftTable, frame);   /* Complex spectrum. */
			frame [1] *= frame [1];   /* DC component? */
			for (i = 2; i < nsampFFT; i += 2) {
				frame [i] = frame [i] * frame [i] + frame [i+1] * frame [i+1];
				frame [i + 1] = 0.0;   /* Power spectrum: square and zero. */
			}
			frame [nsampFFT] *= frame [nsampFFT];   /* Nyquist frequency. */
			NUMfft_backward_d (& fftTable, frame);   /* Autocorrelation. */

			/*
			 * Normalize the autocorrelation to the value with zero lag,
			 * and divide it by the normalized autocorrelation of the window.
			 */
			r [0] = 1.0;
			for (i = 1; i <= brent_ixmax; i ++)
				r [- i] = r [i] = frame [i + 1] / (frame [1] * windowR [i + 1]);
		}

		/*
		 * Create (too much) space for candidates.
		 */
		Pitch_Frame_init (pitchFrame, maxnCandidates); cherror

		/*
		 * Register the first candidate, which is always present: voicelessness.
		 */
		pitchFrame->nCandidates = 1;
		pitchFrame->candidate[1].frequency = 0.0;   /* Voiceless: always present. */
		pitchFrame->candidate[1].strength = 0.0;

		/*
		 * Shortcut: absolute silence is always voiceless.
		 * Go to next frame.
		 */
		if (localPeak == 0) continue;

		/*
		 * Find the strongest maxima of the correlation of this frame, 
		 * and register them as candidates.
		 */
		imax [1] = 0;
		for (i = 2; i < maximumLag && i < brent_ixmax; i ++)
		    if (r [i] > 0.5 * voicingThreshold && /* Not too unvoiced? */
	 	        r [i] > r [i-1] && r [i] >= r [i+1])   /* Maximum? */
		{
			int place = 0;

			/*
			 * Use parabolic interpolation for first estimate of frequency,
			 * and sin(x)/x interpolation to compute the strength of this frequency.
			 */
			double dr = 0.5 * (r [i+1] - r [i-1]), d2r = 2 * r [i] - r [i-1] - r [i+1];
			double frequencyOfMaximum = 1 / my dx / (i + dr / d2r);
			long offset = - brent_ixmax - 1;
			double strengthOfMaximum = /* method & 1 ? */
				NUM_interpolate_sinc_d (r + offset, brent_ixmax - offset, 1 / my dx / frequencyOfMaximum - offset, 30)
				/* : r [i] + 0.5 * dr * dr / d2r */;
			/* High values due to short windows are to be reflected around 1. */
			if (strengthOfMaximum > 1.0) strengthOfMaximum = 1.0 / strengthOfMaximum;

			/*
			 * Find a place for this maximum.
			 */
			if (pitchFrame->nCandidates < thy maxnCandidates) { /* Is there still a free place? */
				place = ++ pitchFrame->nCandidates;
			} else {
				/* Try the place of the weakest candidate so far. */
				double weakest = 2;
				int iweak;
				for (iweak = 2; iweak <= thy maxnCandidates; iweak ++) {
					/* High frequencies are to be favoured */
					/* if we want to analyze a perfectly periodic signal correctly. */
					double localStrength = pitchFrame->candidate[iweak].strength - octaveCost *
						NUMlog2 (minimumPitch / pitchFrame->candidate[iweak].frequency);
					if (localStrength < weakest) { weakest = localStrength; place = iweak; }
				}
				/* If this maximum is weaker than the weakest candidate so far, give it no place. */
				if (strengthOfMaximum - octaveCost * NUMlog2 (minimumPitch / frequencyOfMaximum) <= weakest)
					place = 0;
			}
			if (place) {   /* Have we found a place for this candidate? */
				pitchFrame->candidate[place].frequency = frequencyOfMaximum;
				pitchFrame->candidate[place].strength = strengthOfMaximum;
				imax [place] = i;
			}
		}

		/*
		 * Second pass: for extra precision, maximize sin(x)/x interpolation ('sinc').
		 */
		for (i = 2; i <= pitchFrame->nCandidates; i ++) {
			if (method != AC_HANNING || pitchFrame->candidate[i].frequency > 0.0 / my dx) {
				double xmid, ymid;
				long offset = - brent_ixmax - 1;
				ymid = NUMimproveMaximum_d (r + offset, brent_ixmax - offset, imax [i] - offset,
					pitchFrame->candidate[i].frequency > 0.3 / my dx ? NUM_PEAK_INTERPOLATE_SINC700 : brent_depth, & xmid);
				xmid += offset;
				pitchFrame->candidate[i].frequency = 1.0 / my dx / xmid;
				if (ymid > 1.0) ymid = 1.0 / ymid;
				pitchFrame->candidate[i].strength = ymid;
			}
		}
	}   /* Next frame. */

	Melder_progress (0.95, "Sound to Pitch: path finder");
	Pitch_pathFinder (thee, silenceThreshold, voicingThreshold,
		octaveCost, octaveJumpCost, voicedUnvoicedCost, ceiling, FALSE);

end:
	Melder_progress (1.0, NULL);   /* Done. */
	NUMdvector_free (frame, 1);
	NUMdvector_free (window, 1);
	NUMdvector_free (windowR, 1);
	NUMdvector_free (r, - nsamp_window);
	NUMlvector_free (imax, 1);
	NUMfft_Table_free_d (& fftTable);
	iferror forget (thee);
	return thee;
}

Pitch Sound_to_Pitch (Sound me, double timeStep, double minimumPitch, double maximumPitch) {
	return Sound_to_Pitch_ac (me, timeStep, minimumPitch,
		3.0, 15, FALSE, 0.03, 0.45, 0.01, 0.35, 0.14, maximumPitch);
}

Pitch Sound_to_Pitch_ac (Sound me,
	double dt, double minimumPitch, double periodsPerWindow, int maxnCandidates, int accurate,
	double silenceThreshold, double voicingThreshold,
	double octaveCost, double octaveJumpCost, double voicedUnvoicedCost, double ceiling)
{
	return Sound_to_Pitch_any (me, dt, minimumPitch, periodsPerWindow, maxnCandidates, accurate,
		silenceThreshold, voicingThreshold, octaveCost, octaveJumpCost, voicedUnvoicedCost, ceiling);
}

Pitch Sound_to_Pitch_cc (Sound me,
	double dt, double minimumPitch, double periodsPerWindow, int maxnCandidates, int accurate,
	double silenceThreshold, double voicingThreshold,
	double octaveCost, double octaveJumpCost, double voicedUnvoicedCost, double ceiling)
{
	return Sound_to_Pitch_any (me, dt, minimumPitch, periodsPerWindow, maxnCandidates, 2 + accurate,
		silenceThreshold, voicingThreshold, octaveCost, octaveJumpCost, voicedUnvoicedCost, ceiling);
}

/* End of file Sound_to_Pitch.c */
