#ifndef _TextGridEditor_h_
#define _TextGridEditor_h_
/* TextGridEditor.h
 *
 * Copyright (C) 1992-2005,2007-2022 Paul Boersma
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

#include "TimeSoundAnalysisEditor.h"
#include "TextGrid.h"
#include "SpellingChecker.h"
#include "Preferences.h"

#include "TextGridEditor_enums.h"

Thing_define (TextGridEditor, TimeSoundAnalysisEditor) {
	TextGrid textGrid() { return static_cast <TextGrid> (our data); }

	SpellingChecker spellingChecker;
	integer selectedTier;
	bool suppressRedraw;
	autostring32 findString;
	GuiMenuItem extractSelectedTextGridPreserveTimesButton, extractSelectedTextGridTimeFromZeroButton;

	double draggingTime;
	autoBOOLVEC draggingTiers;
	bool hasBeenDraggedBeyondVicinityRadiusAtLeastOnce = false;

	void v_info ()
		override;
	void v_createChildren ()
		override;
	void v_createMenus ()
		override;
	void v_createHelpMenuItems (EditorMenu menu)
		override;
	void v_dataChanged ()
		override;
	void v_createMenuItems_file_extract (EditorMenu menu)
		override;
	void v_createMenuItems_file_write (EditorMenu menu)
		override;
	void v_createMenuItems_file_draw (EditorMenu menu)
		override;
	void v_prepareDraw ()
		override;
	void v_distributeAreas ()
		override;
	void v_draw ()
		override;
	bool v_hasSelectionViewer ()
		override { return true; }
	void v_drawSelectionViewer ()
		override;
	bool v_hasText ()
		override { return true; }
	bool v_mouseInWideDataView (GuiDrawingArea_MouseEvent event, double xWC, double yWC)
		override;
	void v_clickSelectionViewer (double xWC, double yWC)
		override;
	void v_play (double tmin, double tmax)
		override;
	void v_updateText ()
		override;
	void v_prefs_addFields (EditorCommand cmd)
		override;
	void v_prefs_setValues (EditorCommand cmd)
		override;
	void v_prefs_getValues (EditorCommand cmd)
		override;
	conststring32 v_selectionViewerName ()
		override { return U"IPA chart"; }
	void v_createMenuItems_view_timeDomain (EditorMenu menu)
		override;
	void v_highlightSelection (double left, double right, double bottom, double top)
		override;
	double v_getBottomOfSoundArea ()
		override;
	double v_getBottomOfSoundAndAnalysisArea ()
		override;
	void v_updateMenuItems_file ()
		override;
	void v_createMenuItems_pitch_picture (EditorMenu menu)
		override;

	#include "TextGridEditor_prefs.h"

private:
	/* only in v_mouseInWideDataView: */
	bool anchorIsInWideSoundOrAnalysisPart = false;
	bool anchorIsInWideTextGridPart = false;
	double anchorTime = undefined;
	integer clickedLeftBoundary = 0;
	double leftDraggingBoundary = undefined, rightDraggingBoundary = undefined;   // initial dragging range
};

void TextGridEditor_init (TextGridEditor me, conststring32 title, TextGrid textGrid,
	SpellingChecker spellingChecker, conststring32 callbackSocket
);

autoTextGridEditor TextGridEditor_create (conststring32 title, TextGrid grid,
	SampledXY sound,   // either a Sound or a LongSound, or null
	SpellingChecker spellingChecker,
	conststring32 callbackSocket
);

/* End of file TextGridEditor.h */
#endif
