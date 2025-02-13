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


#ifndef GROUNDMOTION_H
#define GROUNDMOTION_H


#include <seiscomp/plugins/mvx/groundmotion.h>


namespace {


class GroundMotionScale : public Seiscomp::MapViewX::GroundMotionScale {
	public:
		double convert(double pgv, double *pga) override;

		/**
		 * @brief Returns the color gradient of the scale.
		 * @return The color gradient
		 */
		Seiscomp::Gui::Gradient gradient() override;

		/**
		 * @brief Returns the color used for unset values.
		 * @return The color for unset values.
		 */
		QColor unset() override;
		/**
		 * @brief Returns the title of the scale.
		 * @return The scale title
		 */
		std::string title() override;
};


}


#endif // GROUNDMOTION_H
