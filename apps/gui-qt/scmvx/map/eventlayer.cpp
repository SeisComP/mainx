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


#define SEISCOMP_COMPONENT MapView
#include <seiscomp/logging/log.h>
#include <seiscomp/datamodel/event.h>
#include <seiscomp/datamodel/magnitude.h>
#include <seiscomp/gui/datamodel/eventlayer.h>
#include <seiscomp/gui/datamodel/ttdecorator.h>

#include <QMenu>
#include <QMouseEvent>

#include "eventlayer.h"


using namespace Seiscomp::DataModel;
using namespace Seiscomp::Gui;


namespace Seiscomp {
namespace MapViewX {

namespace {


void updateSymbol(DataModel::PublicObjectCache *cache,
                  Map::Canvas *canvas, OriginSymbol *symbol,
                  Event *event, Origin *org) {
	double latitude = org->latitude();
	double longitude = org->longitude();
	double depth = 10;
	double M = 0;

	try {
		depth = org->depth();
	}
	catch ( ... ) {}

	MagnitudePtr mag = cache ? cache->get<Magnitude>(event->preferredMagnitudeID()) : Magnitude::Find(event->preferredMagnitudeID());
	if ( mag ) {
		try {
			M = mag->magnitude();
		}
		catch ( ... ) {}
	}

	symbol->setLocation(latitude, longitude);
	symbol->setDepth(depth);
	symbol->setPreferredMagnitudeValue(M);

	Core::Time originTime;
	originTime = org->time().value();

	if ( Core::Time::UTC()-originTime < Core::TimeSpan(30*60,0) ) {
		// Consider the symbol for TT decorator
		if ( !symbol->decorator() ) {
			symbol->setDecorator(new TTDecorator());
		}

		TTDecorator *d = static_cast<TTDecorator*>(symbol->decorator());
		d->setLatitude(latitude);
		d->setLongitude(longitude);
		d->setDepth(depth);
		d->setPreferredMagnitudeValue(M);
		d->setOriginTime(originTime);
	}
	else if ( symbol->decorator() ) {
		if ( !symbol->decorator()->isVisible() ) {
			symbol->setDecorator(nullptr);
		}
	}

	if ( canvas ) {
		symbol->calculateMapPosition(canvas);
	}
}


}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventLayer::EventLayer(QObject* parent, DataModel::PublicObjectCache *cache)
: Gui::EventLayer(parent)
, _currentEvent(nullptr)
, _cache(cache) {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::setCurrentEvent(DataModel::Event *evt) {
	auto lastCurrent = _currentEvent;

	if ( _currentEvent ) {
		_currentEvent->setType(0);
		_currentEvent->setDepth(_currentEvent->depth());
	}

	if ( !evt ) {
		_currentEvent = nullptr;
	}
	else {
		auto it = _eventSymbols.find(evt->publicID());
		if ( it == _eventSymbols.end() ) {
			_currentEvent = nullptr;
		}
		else {
			_currentEvent = it.value();
		}
	}

	if ( lastCurrent != _currentEvent ) {
		emit updateRequested();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int EventLayer::eventCount() const {
	return _eventSymbols.size();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::draw(const Map::Canvas *canvas, QPainter &p) {
	// Render all symbols
	for ( auto &symbol : _eventSymbols ) {
		if ( symbol == _currentEvent ) {
			continue;
		}
		if ( symbol->isClipped() || !symbol->isVisible() ) {
			continue;
		}
		symbol->draw(canvas, p);
	}

	if ( _currentEvent && !_currentEvent->isClipped() ) {
		_currentEvent->draw(canvas, p);
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
		if ( it.value()->isClipped() || !it.value()->isVisible() ) continue;
		if ( it.value()->isInside(x, y) ) {
			_hoverChanged = _hoverId != it.key();
			if ( _hoverChanged ) {
				setHoverId(it.key());
			}
			return true;
		}
	}

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::handleLeaveEvent() {
	_hoverChanged = !_hoverId.empty();
	setHoverId(std::string());
	emit eventHovered(_hoverId);
	_hoverChanged = false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventLayer::filterMouseDoubleClickEvent(QMouseEvent *event, const QPointF &geoPos) {
	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventLayer::filterMousePressEvent(QMouseEvent *, const QPointF &) {
	return !_hoverId.empty();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool EventLayer::filterMouseReleaseEvent(QMouseEvent *event, const QPointF &) {
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
	_currentEvent = nullptr;
	setHoverId(std::string());
	Gui::EventLayer::clear();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::addEvent(Event *e, bool) {
	auto it = _eventSymbols.find(e->publicID());

	OriginPtr org = _cache ? _cache->get<Origin>(e->preferredOriginID()) : Origin::Find(e->preferredOriginID());
	if ( org ) {
		OriginSymbol *symbol;

		if ( it == _eventSymbols.end() ) {
			symbol = new OriginSymbol();
		}
		else {
			symbol = it.value();
		}

		updateSymbol(_cache, canvas(), symbol, e, org.get());
		_eventSymbols[e->publicID()] = symbol;

		// Create origin symbol and register it
		emit updateRequested();
	}
	else
		SEISCOMP_ERROR("Origin %s for event %s not found",
		               e->preferredOriginID().c_str(), e->publicID().c_str());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::updateEvent(Event *e) {
	auto it = _eventSymbols.find(e->publicID());
	if ( it == _eventSymbols.end() ) {
		return;
	}

	OriginPtr org = _cache ? _cache->get<Origin>(e->preferredOriginID()) : Origin::Find(e->preferredOriginID());
	if ( org ) {
		updateSymbol(_cache, canvas(), it.value(), e, org.get());
		emit updateRequested();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::removeEvent(Event *e) {
	auto it = _eventSymbols.find(e->publicID());
	if ( it == _eventSymbols.end() ) {
		return;
	}
	if ( it.value() == _currentEvent ) {
		_currentEvent = nullptr;
	}
	delete it.value();
	_eventSymbols.erase(it);
	emit updateRequested();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::tick() {
	bool hasActiveDecorators = false;
	auto it = _eventSymbols.begin();
	for ( ; it != _eventSymbols.end(); ++it ) {
		if ( it.value()->decorator() ) {
			if ( !it.value()->decorator()->isVisible() ) {
				it.value()->setDecorator(nullptr);
			}
			else
				hasActiveDecorators = true;
		}
	}

	if ( _currentEvent ) {
		if ( _currentEvent->type() == 0 ) {
			_currentEvent->setType(1);
			_currentEvent->setColor(QColor(33,53,81));
			_currentEvent->setFillColor(QColor(122,212,226,204));
		}
		else {
			_currentEvent->setType(0);
			_currentEvent->setDepth(_currentEvent->depth());
		}

		emit updateRequested();
	}
	else if ( hasActiveDecorators )
		emit updateRequested();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::setHoverId(const std::string &id) {
	bool updateRequired = false;

	if ( !_hoverId.empty() ) {
		auto it = _eventSymbols.find(_hoverId);
		if ( it != _eventSymbols.end() ) {
			it.value()->setDepth(it.value()->depth());
			updateRequired = true;
		}
	}

	_hoverId = id;

	if ( !_hoverId.empty() ) {
		auto it = _eventSymbols.find(_hoverId);
		if ( it != _eventSymbols.end() ) {
			it.value()->setFillColor(it.value()->color());
			it.value()->setColor(QColor(33,53,81));
			updateRequired = true;
		}
	}

	if ( updateRequired ) {
		emit updateRequested();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
