#ifndef _TextGridTierNavigator_h_
#define _TextGridTierNavigator_h_
/* TextGridTierNavigator.h
 *
 * Copyright (C) 2020-2021 David Weenink
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

#include "NavigationContext.h"
#include "TextGrid.h"
#include "melder.h"

#include "TextGridTierNavigator_enums.h"
#include "TextGridTierNavigator_def.h"

/*
	The purpose of a TextGridTierNavigator is to find successive elements that match a criterion on one selected tier.
	The navigation context handles what to match/search on the tier.
	We search for a topic label which may be preceeded by a Before label and/or an After label.
*/

autoTextGridTierNavigator TextGrid_and_NavigationContext_to_TextGridTierNavigator (TextGrid textgrid, NavigationContext navigationContext, integer tierNumber, kMatchDomain matchDomain);

autoNavigationContext TextGridTierNavigator_extractNavigationContext (TextGridTierNavigator me);

void TextGridTierNavigator_replaceNavigationContext (TextGridTierNavigator me, NavigationContext thee);

void TextGridTierNavigator_replaceTier (TextGridTierNavigator me, Function thee);

void TextGridTierNavigator_modifyBeforeRange (TextGridTierNavigator me, integer from, integer to);
void TextGridTierNavigator_modifyAfterRange (TextGridTierNavigator me, integer from, integer to);

void TextGridTierNavigator_modifyTopicCriterion (TextGridTierNavigator me, kMelder_string newCriterion);
void TextGridTierNavigator_modifyBeforeCriterion (TextGridTierNavigator me, kMelder_string newCriterion);
void TextGridTierNavigator_modifyAfterCriterion (TextGridTierNavigator me, kMelder_string newCriterion);
void TextGridTierNavigator_modifyUseCriterion (TextGridTierNavigator me, kContext_use newUse, bool excludeTopicMatch);
void TextGridTierNavigator_modifyMatchDomain (TextGridTierNavigator me, kMatchDomain newDomain);

void TextGridTierNavigator_modifyMatchingRange (TextGridTierNavigator me, integer maximumLookAhead, integer maximumLookBack);

bool TextGridTierNavigator_isLabelMatch (TextGridTierNavigator me, integer indexInTopicTier);

integer TextGridTierNavigator_getNumberOfMatches (TextGridTierNavigator me);
integer TextGridTierNavigator_getNumberOfTopicMatches (TextGridTierNavigator me);
integer TextGridTierNavigator_getNumberOfBeforeMatches (TextGridTierNavigator me);
integer TextGridTierNavigator_getNumberOfAfterMatches (TextGridTierNavigator me);

integer TextGridTierNavigator_locateNext (TextGridTierNavigator me);

integer TextGridTierNavigator_locateNextAfterTime (TextGridTierNavigator me, double time);

integer TextGridTierNavigator_locatePrevious (TextGridTierNavigator me);

integer TextGridTierNavigator_locatePreviousBeforeTime (TextGridTierNavigator me, double time);

integer TextGridTierNavigator_getIndex (TextGridTierNavigator me, kContext_where where);
double TextGridTierNavigator_getStartTime (TextGridTierNavigator me, kContext_where where);
conststring32 TextGridTierNavigator_getLabel (TextGridTierNavigator me,  kContext_where where);
double TextGridTierNavigator_getEndTime (TextGridTierNavigator me, kContext_where where);

static inline integer TextGridTierNavigator_locateFirst (TextGridTierNavigator me) {
	return TextGridTierNavigator_locateNextAfterTime (me, my xmin - 0.1);
}

static inline integer TextGridTierNavigator_locateLast (TextGridTierNavigator me) {
	return TextGridTierNavigator_locatePreviousBeforeTime (me, my xmax + 0.1);
}

#endif /* _TextGridTierNavigator_h_ */
