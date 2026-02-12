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
#include <seiscomp/datamodel/focalmechanism.h>
#include <seiscomp/datamodel/momenttensor.h>
#include <seiscomp/datamodel/magnitude.h>
#include <seiscomp/math/conversions.h>
#include <seiscomp/math/tensor.h>
#include <seiscomp/gui/core/application.h>
#include <seiscomp/gui/datamodel/eventlayer.h>
#include <seiscomp/gui/datamodel/ttdecorator.h>

#include <QMenu>
#include <QMouseEvent>

#include "eventlayer.h"


using namespace Seiscomp::DataModel;
using namespace Seiscomp::Gui;


namespace Seiscomp::MapViewX {

namespace {


void updateSymbol(DataModel::PublicObjectCache *cache,
                  Map::Canvas *canvas, Gui::EventLayer::EventSymbol &symbol,
                  Event *event, Origin *org, FocalMechanism *fm) {
	double latitude = org->latitude();
	double longitude = org->longitude();
	double depth = 10;
	double M = 0;

	MagnitudePtr mag = cache ? cache->get<Magnitude>(event->preferredMagnitudeID()) : Magnitude::Find(event->preferredMagnitudeID());
	if ( mag ) {
		try {
			M = mag->magnitude();
		}
		catch ( ... ) {}
	}

	const DataModel::NodalPlane *np{};

	if ( fm ) {
		int preferredNodalPlane = 0;

		try {
			preferredNodalPlane = fm->nodalPlanes().preferredPlane();
		}
		catch ( Core::ValueException& ) {}

		try {
			if ( preferredNodalPlane == 0 ) {
				np = &(fm->nodalPlanes().nodalPlane1());
			}
			else {
				np = &(fm->nodalPlanes().nodalPlane2());
			}
		}
		catch ( Core::ValueException& ) {}
	}

	symbol.free();

	symbol.origin = new OriginSymbol;

	try {
		depth = org->depth();
	}
	catch ( ... ) {}

	symbol.origin->setLocation(latitude, longitude);
	symbol.origin->setDepth(depth);
	symbol.origin->setPreferredMagnitudeValue(M);

	Core::Time originTime;
	originTime = org->time().value();

	if ( Core::Time::UTC() - originTime < Core::TimeSpan(30 * 60, 0) ) {
		// Consider the symbol for TT decorator
		if ( !symbol.origin->decorator() ) {
			symbol.origin->setDecorator(new TTDecorator());
		}

		TTDecorator *d = static_cast<TTDecorator*>(symbol.origin->decorator());
		d->setLatitude(latitude);
		d->setLongitude(longitude);
		d->setDepth(depth);
		d->setPreferredMagnitudeValue(M);
		d->setOriginTime(originTime);
	}
	else if ( symbol.origin->decorator() ) {
		if ( !symbol.origin->decorator()->isVisible() ) {
			symbol.origin->setDecorator(nullptr);
		}
	}

	if ( canvas ) {
		symbol.origin->calculateMapPosition(canvas);
	}

	if ( np ) {
		np->strike().value();
		np->dip().value();
		np->rake().value();

		Math::Tensor2Sd tensor;
		Math::np2tensor({
			np->strike().value(),
			np->dip().value(),
			np->rake().value()
		}, tensor);

		symbol.tensor = new Gui::TensorSymbol(tensor);

		OriginPtr origin;

		if ( fm->momentTensorCount() > 0 ) {
			auto mt = fm->momentTensor(0);
			origin = cache ? cache->get<Origin>(mt->derivedOriginID()) : Origin::Find(mt->derivedOriginID());
			if ( origin ) {
				org = origin.get();
			}

			mag = cache ? cache->get<Magnitude>(mt->momentMagnitudeID()) : Magnitude::Find(mt->momentMagnitudeID());
			if ( mag ) {
				try {
					M = mag->magnitude();
				}
				catch ( ... ) {}
			}
		}

		int size;
		if ( M > 0 ) {
			size = std::max(SCScheme.map.originSymbolMinSize, int(4.9 * (M - 1.2)));
		}
		else {
			size = SCScheme.map.originSymbolMinSize;
		}

		symbol.tensor->setSize(QSize(size * 3 / 2, size * 3 / 2));
		symbol.tensor->setShadingEnabled(true);
		symbol.tensor->setPosition(QPointF(org->longitude(), org->latitude()));
		symbol.tensor->setTColor(SCScheme.colors.originSymbol.depth.gradient.colorAt(depth, SCScheme.colors.originSymbol.depth.discrete));
		symbol.tensor->setOffset(QPoint(-16, -32));
		symbol.tensor->setDrawConnectorEnabled(true);
		symbol.tensor->setPriority(Gui::Map::Symbol::HIGH);

		if ( canvas ) {
			symbol.tensor->calculateMapPosition(canvas);
		}
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
			_currentEvent = it.value().origin;
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
	EventSymbol *currentSymbol = nullptr;

	// Render all symbols
	for ( auto &symbol : _eventSymbols ) {
		if ( symbol.origin == _currentEvent ) {
			currentSymbol = &symbol;
			continue;
		}

		if ( !symbol.origin->isClipped() && symbol.origin->isVisible() ) {
			symbol.origin->draw(canvas, p);
		}

		if ( symbol.tensor ) {
			if ( !symbol.tensor->isClipped() && symbol.tensor->isVisible() ) {
				symbol.tensor->draw(canvas, p);
			}
		}
	}

	if( currentSymbol ) {
		auto &symbol = *currentSymbol;

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
	FocalMechanismPtr fm;
	if ( !e->preferredFocalMechanismID().empty() ) {
		fm = _cache ? _cache->get<FocalMechanism>(e->preferredFocalMechanismID()) : FocalMechanism::Find(e->preferredFocalMechanismID());
	}

	if ( org ) {
		EventSymbol symbol;

		if ( it != _eventSymbols.end() ) {
			symbol = it.value();
		}

		updateSymbol(_cache, canvas(), symbol, e, org.get(), fm.get());
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
	FocalMechanismPtr fm;
	if ( !e->preferredFocalMechanismID().empty() ) {
		fm = _cache ? _cache->get<FocalMechanism>(e->preferredFocalMechanismID()) : FocalMechanism::Find(e->preferredFocalMechanismID());
	}

	if ( org ) {
		updateSymbol(_cache, canvas(), it.value(), e, org.get(), fm.get());
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
	if ( it.value().origin == _currentEvent ) {
		_currentEvent = nullptr;
	}
	it.value().free();
	_eventSymbols.erase(it);
	emit updateRequested();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventLayer::tick() {
	bool hasActiveDecorators = false;
	auto it = _eventSymbols.begin();
	for ( ; it != _eventSymbols.end(); ++it ) {
		if ( it.value().origin->decorator() ) {
			if ( !it.value().origin->decorator()->isVisible() ) {
				it.value().origin->setDecorator(nullptr);
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
			it.value().origin->setDepth(it.value().origin->depth());
			updateRequired = true;
		}
	}

	_hoverId = id;

	if ( !_hoverId.empty() ) {
		auto it = _eventSymbols.find(_hoverId);
		if ( it != _eventSymbols.end() ) {
			it.value().origin->setFillColor(it.value().origin->color());
			it.value().origin->setColor(QColor(33,53,81));
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
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
