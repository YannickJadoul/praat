#ifndef _LPCToFormantWorkspace_h_
#define _LPCToFormantWorkspace_h_
/* LPCToFormantWorkspace.h
 *
 * Copyright (C) 2024 David Weenink
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

#include "Formant.h"
#include "LPC.h"
#include "Polynomial.h"
#include "Roots.h"
#include "LPCToSampledWorkspace.h"
#include "LPCToFormantWorkspace_def.h"

autoLPCToFormantWorkspace LPCToFormantWorkspace_create (constLPC input, mutableFormant output, double margin);

/*
	If the input LPC was missing
 */
void LPCToFormantWorkspace_initLPCDependency (LPCToFormantWorkspace me, integer maxnCoefficients, double samplingPeriod);

void LPCToFormantWorkspace_initFormantDependency (LPCToFormantWorkspace me, integer maxnFormants);

#endif /*_LPCToFormantWorkspace_h_ */
