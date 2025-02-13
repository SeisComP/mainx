/***************************************************************************
 * Copyright (C) gempa GmbH                                                *
 * All rights reserved.                                                    *
 * Contact: gempa GmbH (seiscomp-dev@gempa.de)                             *
 *                                                                         *
 * GNU Affero General Public License Usage                                 *
 * This file may be used under the terms of the GNU Affero                 *
 * Public License version 3.0 as published by the Free Software Foundation *
 * and appearing in the file LICENSE included in the packaging of this     *
 * file. Please review the following information to ensure the GNU Affero  *
 * Public License version 3.0 requirements will be met:                    *
 * https://www.gnu.org/licenses/agpl-3.0.html.                             *
 *                                                                         *
 * Other Usage                                                             *
 * Alternatively, this file may be used in accordance with the terms and   *
 * conditions contained in a signed written agreement between you and      *
 * gempa GmbH.                                                             *
 ***************************************************************************/


#include "settings.h"


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace Seiscomp {
namespace MapViewX {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Settings global;
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Settings::accept(System::Application::SettingsLinker &linker) {
	linker
	& cli(
		displayMode,
		"MapViewX", "displaymode",
		"Start mapview as walldisplay. Modes: groundmotion, qualitycontrol"
	)
	& cliSwitch(
		showLegend,
		"MapViewX", "with-legend",
		"Shows the map legend if started as walldisplay"
	)
	& cliSwitch(
		offline,
		"MapViewX", "offline",
		"Do not connect to a messaging server and do not subscribe channel data"
	)
	& cli(
		inputFile,
		"MapViewX", "input-file,i",
		"Load events in given XML file during startup and "
		"switch to offline mode."
	)
	& cfg(filter, "stations.groundMotionFilter")
	& cfg(maximumAmplitudeTimeSpan, "stations.amplitudeTimeSpan")
	& cfg(ringBuffer, "stations.groundMotionRecordLifeSpan")
	& cfg(triggerTimeout, "stations.triggerTimeout")
	& cfg(eventTimeSpan, "readEventsNotOlderThan")
	& cfg(centerOrigins, "centerOrigins")
	& cfg(showLatestEvent, "showLatestEvent")
	& cfg(displayMode, "displaymode")
	& cfg(initialRegion, "display")
	;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
