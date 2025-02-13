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


#ifndef SEISCOMP_MAPVIEWX_LAYERS_EVENTHEATLAYER_H
#define SEISCOMP_MAPVIEWX_LAYERS_EVENTHEATLAYER_H


#ifndef Q_MOC_RUN
#include <seiscomp/datamodel/event.h>
#include <seiscomp/gui/core/lut.h>
#include <seiscomp/gui/map/layer.h>
#endif
#include <QTimer>


namespace Seiscomp {
namespace MapViewX {


class EventHeatLayer : public Gui::Map::Layer {
	Q_OBJECT

	public:
		EventHeatLayer(QObject* parent);


	// ----------------------------------------------------------------------
	//  Slots
	// ----------------------------------------------------------------------
	public slots:
		void clear();
		void addEvent(Seiscomp::DataModel::Event*, bool);
		void updateEvent(Seiscomp::DataModel::Event*);
		void removeEvent(Seiscomp::DataModel::Event*);

		void setCompositionMode(bool);

	private slots:
		void updateCanvas();


	public:
		void baseBufferUpdated(Gui::Map::Canvas *canvas,
		                       QPainter &painter) override;
		void setVisible(bool) override;


	private:
		void generateLUT();


	public:
		DEFINE_SMARTPOINTER(Event);
		struct Event : public Core::BaseObject {
			QPointF location;
			float   magnitude;
		};


	private:
		using EventMap = QMap<std::string, EventPtr>;
		EventMap                   _events;

		QVector<float>             _intensityBuffer;
		Gui::Gradient              _gradient;
		Gui::StaticColorLUT<1024>  _gradientLUT;
		bool                       _composeMultiply;
		QTimer                     _updateTimer;
		Gui::Map::Legend          *_legend;
};


}
}


#endif
