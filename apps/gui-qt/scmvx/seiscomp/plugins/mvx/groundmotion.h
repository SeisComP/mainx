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


#ifndef SEISCOMP_PLUGINS_MAPVIEWX_GROUNDMOTION_H
#define SEISCOMP_PLUGINS_MAPVIEWX_GROUNDMOTION_H


#include <seiscomp/core/baseobject.h>
#include <seiscomp/core/interfacefactory.h>
#include <seiscomp/gui/core/gradient.h>
#include <seiscomp/plugins/mvx/api.h>


namespace Seiscomp {
namespace MapViewX {


DEFINE_SMARTPOINTER(GroundMotionScale);
class GroundMotionScale : public Core::BaseObject {
	public:
		GroundMotionScale() = default;

	public:
		/**
		 * @brief Converts an input pgv value to an output value.
		 * The pga value is optional and might be presented.
		 * @param pgv Input pgv in m/s.
		 * @param pga Optional input pga in m/s**2
		 * @return The output value
		 */
		virtual double convert(double pgv, double *pga) = 0;

		/**
		 * @brief Returns the color gradient of the scale.
		 * @return The color gradient
		 */
		virtual Gui::Gradient gradient() = 0;

		/**
		 * @brief Returns the color used for unset values.
		 * @return The color for unset values.
		 */
		virtual QColor unset() = 0;
		/**
		 * @brief Returns the title of the scale.
		 * @return The scale title
		 */
		virtual std::string title() = 0;
};


DEFINE_INTERFACE_FACTORY(GroundMotionScale);


}
}


#define REGISTER_MAPVIEWX_GROUNDMOTION_SCALE(Class, Service) \
Seiscomp::Core::Generic::InterfaceFactory<Seiscomp::MapViewX::GroundMotionScale, Class> __##Class##InterfaceFactory__(Service)


#endif
