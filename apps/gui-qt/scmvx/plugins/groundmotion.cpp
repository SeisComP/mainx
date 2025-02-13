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


#include <seiscomp/core/plugin.h>

#include "groundmotion.h"


namespace {

ADD_SC_PLUGIN("Ground motion scale example for scmvx",
              "Jan Becker, gempa GmbH <jabe@gempa.de>", 1, 0, 0)

REGISTER_MAPVIEWX_GROUNDMOTION_SCALE(GroundMotionScale, "example");


double GroundMotionScale::convert(double pgv, double *) {
	return pgv * 1E9; // Convert to nm/s
}

Seiscomp::Gui::Gradient GroundMotionScale::gradient() {
	Seiscomp::Gui::Gradient gradient;
	gradient.setColorAt(0, QColor(0,0,0), "Sensor off?");
	gradient.setColorAt(1500, QColor(0,255,0), "Light");
	gradient.setColorAt(30000, QColor(255,128,0), "Moderate");
	gradient.setColorAt(150000, QColor(255,0,0), "Severe");
	return gradient;
}

QColor GroundMotionScale::unset() {
	return QColor(0, 0, 0, 128);
}

std::string GroundMotionScale::title() {
	return "States";
}


}
