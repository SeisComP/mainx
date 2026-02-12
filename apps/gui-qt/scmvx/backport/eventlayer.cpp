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


#include <seiscomp/datamodel/event.h>
#include <seiscomp/datamodel/magnitude.h>
#include <seiscomp/gui/core/application.h>
#include <seiscomp/gui/core/compat.h>
#include <seiscomp/gui/datamodel/eventlayer.h>

#include <QMenu>
#include <QMouseEvent>

#include "eventlayer.h"


namespace Seiscomp {
namespace Gui {
namespace Backport {
namespace {


void updateSymbol(Map::Canvas *canvas, OriginSymbol *symbol,
                  DataModel::Event *event, DataModel::Origin *org) {
	symbol->setLocation(org->latitude(), org->longitude());

	try {
		symbol->setDepth(org->depth());
	}
	catch ( ... ) {}

	DataModel::Magnitude *mag = DataModel::Magnitude::Find(event->preferredMagnitudeID());
	if ( mag != nullptr ) {
		try {
			symbol->setPreferredMagnitudeValue(mag->magnitude());
		}
		catch ( ... ) {
			symbol->setPreferredMagnitudeValue(0);
		}
	}
	else
		symbol->setPreferredMagnitudeValue(0);

	if ( canvas )
		symbol->calculateMapPosition(canvas);
}


}


#define HMARGIN (textHeight/2)
#define VMARGIN (textHeight/2)
#define SPACING (textHeight/2)
#define VSPACING (textHeight*3/4)
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventLayer::EventLayer(QObject* parent) : Map::Layer(parent) {
	setName("events");
	setVisible(false);
	_hoverChanged = false;

	EventLegend *legend = new EventLegend(this);
	legend->setTitle(tr("Event symbols"));
	addLegend(legend);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventLayer::~EventLayer() {
	// Delete all symbols
	for ( auto &symbol : _eventSymbols ) {
		symbol.free();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::draw(const Map::Canvas *canvas, QPainter &p) {
	for ( auto &symbol : _eventSymbols ) {
		if ( !symbol.origin->isClipped() && symbol.origin->isVisible() ) {
			symbol.origin->draw(canvas, p);
		}

		if ( symbol.tensor ) {
			if ( !symbol.tensor->isClipped() && symbol.tensor->isVisible() ) {
				symbol.tensor->draw(canvas, p);
			}
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::calculateMapPosition(const Map::Canvas *canvas) {
	// Update position of all symbols
	for ( auto &symbol : _eventSymbols ) {
		symbol.origin->calculateMapPosition(canvas);
		if ( symbol.tensor ) {
			symbol.tensor->calculateMapPosition(canvas);
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventLayer::isInside(const QMouseEvent *event, const QPointF &geoPos) {
	int x = event->pos().x();
	int y = event->pos().y();
	auto it = _eventSymbols.end();

	while ( it != _eventSymbols.begin() ) {
		--it;
		if ( it.value().origin->isClipped() || !it.value().origin->isVisible() ) {
			continue;
		}
		if ( it.value().origin->isInside(x, y) ) {
			_hoverChanged = _hoverId != it.key();
			if ( _hoverChanged ) {
				_hoverId = it.key();
			}
			return true;
		}
	}

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::handleEnterEvent() {
	//
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::handleLeaveEvent() {
	_hoverChanged = !_hoverId.empty();
	_hoverId = std::string();
	emit eventHovered(_hoverId);
	_hoverChanged = false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventLayer::filterMouseMoveEvent(QMouseEvent *event, const QPointF &geoPos) {
	if ( _hoverChanged ) {
		emit eventHovered(_hoverId);
		_hoverChanged = false;
	}
	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventLayer::filterMouseDoubleClickEvent(QMouseEvent *event, const QPointF &geoPos) {
	if ( _hoverId.empty() )
		return false;

	if ( event->button() == Qt::LeftButton ) {
		emit eventSelected(_hoverId);
		return true;
	}

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::clear() {
	// Delete all symbols
	for ( auto &symbol : _eventSymbols ) {
		symbol.free();
	}

	_eventSymbols.clear();
	_hoverChanged = false;
	_hoverId = std::string();
	emit updateRequested();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::addEvent(Seiscomp::DataModel::Event *e, bool) {
	auto it = _eventSymbols.find(e->publicID());

	DataModel::Origin *org = DataModel::Origin::Find(e->preferredOriginID());
	if ( org ) {
		OriginSymbol *symbol;

		if ( it == _eventSymbols.end() ) {
			symbol = new OriginSymbol();
		}
		else {
			symbol = it.value().origin;
		}

		updateSymbol(canvas(), symbol, e, org);
		_eventSymbols[e->publicID()].origin = symbol;

		// Create origin symbol and register it
		emit updateRequested();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::updateEvent(Seiscomp::DataModel::Event *e) {
	auto it = _eventSymbols.find(e->publicID());
	if ( it == _eventSymbols.end() ) {
		return;
	}

	DataModel::Origin *org = DataModel::Origin::Find(e->preferredOriginID());
	if ( org ) {
		updateSymbol(canvas(), it.value().origin, e, org);
		emit updateRequested();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::removeEvent(Seiscomp::DataModel::Event *e) {
	auto it = _eventSymbols.find(e->publicID());
	if ( it == _eventSymbols.end() ) {
		return;
	}
	it.value().free();
	_eventSymbols.erase(it);
	emit updateRequested();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
