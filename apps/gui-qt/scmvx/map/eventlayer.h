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


#ifndef SEISCOMP_MAPVIEWX_LAYERS_EVENTLAYER_H
#define SEISCOMP_MAPVIEWX_LAYERS_EVENTLAYER_H


#ifndef Q_MOC_RUN
#include <seiscomp/datamodel/publicobjectcache.h>
#include <seiscomp/gui/datamodel/originsymbol.h>
#include <seiscomp/gui/datamodel/eventlayer.h>
#endif

#include <QMap>


namespace Seiscomp::MapViewX {


class EventLayer : public Gui::EventLayer {
	Q_OBJECT


	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		EventLayer(QObject* parent, DataModel::PublicObjectCache *cache);


	// ----------------------------------------------------------------------
	//  Layer interface
	// ----------------------------------------------------------------------
	public:
		/**
		 * @brief Sets the current event if available as symbol. This event
		 *        will be drawn on-top and toggle its colors when calling
		 *        tick (blinking).
		 * @param evt The pointer to the current event. Actually only the
		 *            publicID will be read and looked up in the local store.
		 */
		void setCurrentEvent(DataModel::Event *evt);
		int eventCount() const;


	// ----------------------------------------------------------------------
	//  Layer interface
	// ----------------------------------------------------------------------
	public:
		void draw(const Gui::Map::Canvas *, QPainter &) override;
		bool isInside(const QMouseEvent *event, const QPointF &geoPos) override;

		void handleLeaveEvent() override;
		bool filterMouseDoubleClickEvent(QMouseEvent *event, const QPointF &geoPos) override;
		bool filterMousePressEvent(QMouseEvent *event, const QPointF &geoPos) override;
		bool filterMouseReleaseEvent(QMouseEvent *event, const QPointF &geoPos) override;


	// ----------------------------------------------------------------------
	//  Slots
	// ----------------------------------------------------------------------
	public slots:
		void clear();
		void addEvent(Seiscomp::DataModel::Event*,bool);
		void updateEvent(Seiscomp::DataModel::Event*);
		void removeEvent(Seiscomp::DataModel::Event*);
		void tick();


	// ----------------------------------------------------------------------
	//  Signals
	// ----------------------------------------------------------------------
	signals:
		void eventHovered(const std::string &eventID);
		void eventSelected(const std::string &eventID);


	// ----------------------------------------------------------------------
	//  Private members
	// ----------------------------------------------------------------------
	private:
		void setHoverId(const std::string &id);


	// ----------------------------------------------------------------------
	//  Protected members
	// ----------------------------------------------------------------------
	protected:
		Gui::OriginSymbol            *_currentEvent;
		DataModel::PublicObjectCache *_cache;
};


}


#endif
