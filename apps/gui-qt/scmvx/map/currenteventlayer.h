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


#ifndef SEISCOMP_MAPVIEWX_LAYERS_CURRENTEVENTLAYER_H
#define SEISCOMP_MAPVIEWX_LAYERS_CURRENTEVENTLAYER_H


#include "seiscomp/datamodel/event.h"
#ifndef Q_MOC_RUN
#include <seiscomp/gui/map/layer.h>
#endif


namespace Seiscomp::MapViewX {


class CurrentEventLayer : public Gui::Map::Layer {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		CurrentEventLayer(QObject* parent);


	// ----------------------------------------------------------------------
	//  Layer interface
	// ----------------------------------------------------------------------
	public:
		/**
		 * @brief Sets the event.
		 * @param evt The pointer to the current event.
		 */
		void setEvent(DataModel::Event *evt);


	// ----------------------------------------------------------------------
	//  Layer interface
	// ----------------------------------------------------------------------
	public:
		void draw(const Gui::Map::Canvas *, QPainter &) override;
		void calculateMapPosition(const Gui::Map::Canvas *canvas) override;
		bool isInside(const QMouseEvent *event, const QPointF &geoPos) override;


	// ----------------------------------------------------------------------
	//  Private members
	// ----------------------------------------------------------------------
	private:
		bool            _isValid;
		QSize           _size;
		QSize           _magSize;
		int             _headerHeight;

		QFont           _magFont;
		QFont           _boldFont;
		QFont           _normalFont;

		OPT(double)     _magnitude;
		QString         _region;
		OPT(Core::Time) _originTime;
};


}


#endif
