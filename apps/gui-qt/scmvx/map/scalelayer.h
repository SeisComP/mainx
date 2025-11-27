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


#ifndef SEISCOMP_MAPVIEWX_LAYERS_SCALELAYER_H
#define SEISCOMP_MAPVIEWX_LAYERS_SCALELAYER_H


#include <seiscomp/gui/map/layer.h>


namespace Seiscomp::MapViewX {


class ScaleLayer : public Gui::Map::Layer {
	Q_OBJECT

	public:
		ScaleLayer(QObject *parent = NULL, Qt::Alignment alignment = Qt::AlignBottom | Qt::AlignHCenter);

	public:
		void draw(const Gui::Map::Canvas*, QPainter &p) override;


	private:
		Qt::Alignment _alignment;
};


}


#endif
