/* TextGridArea.cpp
 *
 * Copyright (C) 1992-2022 Paul Boersma
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TextGridArea.h"
#include "AnyTextGridEditor.h"   // BUG: should not be included
#include "EditorM.h"

Thing_implement (TextGridArea, FunctionArea, 0);

#include "enums_getText.h"
#include "TextGridArea_enums.h"
#include "enums_getValue.h"
#include "TextGridArea_enums.h"

#include "Prefs_define.h"
#include "TextGridArea_prefs.h"
#include "Prefs_install.h"
#include "TextGridArea_prefs.h"
#include "Prefs_copyToInstance.h"
#include "TextGridArea_prefs.h"


#pragma mark - TextGridArea drawing

void structTextGridArea :: v_specializedHighlightBackground () const {
	Melder_assert (our textGrid());
	const integer numberOfTiers = our textGrid() -> tiers->size;
	for (integer itier = 1; itier <= numberOfTiers; itier ++) {
		const Function anyTier = our textGrid() -> tiers->at [itier];
		if (anyTier -> classInfo != classIntervalTier)
			continue;
		IntervalTier tier = (IntervalTier) anyTier;
		/*
			Highlight interval: yellow (selected) or green (matching label).
		*/
		const integer selectedInterval = ( itier == our selectedTier ? getSelectedInterval (this) : 0 );
		const integer numberOfIntervals = tier -> intervals.size;
		for (integer iinterval = 1; iinterval <= numberOfIntervals; iinterval ++) {
			const TextInterval interval = tier -> intervals.at [iinterval];
			/* mutable clip */ double startInterval = interval -> xmin, endInterval = interval -> xmax;
			if (endInterval > our startWindow() && startInterval < our endWindow()) {   // interval visible?
				const bool intervalIsSelected = ( iinterval == selectedInterval );
				const bool labelDoesMatch = Melder_stringMatchesCriterion (interval -> text.get(),
						our instancePref_greenMethod(), our instancePref_greenString(), true);
				Melder_clipLeft (our startWindow(), & startInterval);
				Melder_clipRight (& endInterval, our endWindow());
				const double bottom = 1.0 - double (itier) / numberOfTiers;
				const double top = 1.0 - double (itier - 1) / numberOfTiers;
				if (labelDoesMatch) {
					Graphics_setColour (our graphics(), Melder_LIME);
					Graphics_fillRectangle (our graphics(), startInterval, endInterval, bottom, top);
				}
				if (intervalIsSelected) {
					Graphics_setColour (our graphics(), Melder_YELLOW);
					Graphics_fillRectangle (our graphics(),
						labelDoesMatch ? 0.85 * startInterval + 0.15 * endInterval : startInterval,
						labelDoesMatch ? 0.15 * startInterval + 0.85 * endInterval : endInterval,
						labelDoesMatch ? 0.85 * bottom + 0.15 * top : bottom,
						labelDoesMatch ? 0.15 * bottom + 0.85 * top : top
					);
				}
			}
		}
	}
	Graphics_setColour (our graphics(), Melder_BLACK);
}


#pragma mark - TextGridArea File menu

static void menu_cb_SaveWholeTextGridAsTextFile (TextGridArea me, EDITOR_ARGS_FORM) {
	EDITOR_FORM_SAVE (U"Save whole TextGrid as text file", nullptr)
		Melder_sprint (defaultName,300, my textGrid() -> name.get(), U".TextGrid");
	EDITOR_DO_SAVE
		Data_writeToTextFile (my textGrid(), file);
	EDITOR_END
}

void structTextGridArea :: v_createMenuItems_file (EditorMenu menu) {
	FunctionAreaMenu_addCommand (menu, U"Save TextGrid to disk:", 0, nullptr, this);
	FunctionAreaMenu_addCommand (menu, U"Save whole TextGrid as text file...", 'S',
			menu_cb_SaveWholeTextGridAsTextFile, this);
	FunctionAreaMenu_addCommand (menu, U"-- after TextGrid save --", 0, nullptr, this);
}

/* End of file TextGridArea.cpp */
