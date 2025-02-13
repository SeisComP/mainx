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
#include <seiscomp/datamodel/origin.h>
#include <seiscomp/datamodel/magnitude.h>
#include <seiscomp/datamodel/utils.h>

#include <QApplication>

#include "currenteventlayer.h"


using namespace Seiscomp::DataModel;
using namespace Seiscomp::Gui;


namespace Seiscomp {
namespace MapViewX {

namespace {


QString timeAgoToString(const Core::TimeSpan &ts) {
	int seconds = abs(ts.seconds());
	int minutes = seconds / 60;
	int hours = minutes / 60;
	int days = hours / 24;
	int years = days / 365;

	QString strTimespan;

	if ( years > 1 )
		strTimespan = QObject::tr("%1 years").arg(years);
	else if ( years == 1 )
		strTimespan = "a year";
	else if ( days > 45 )
		strTimespan = QObject::tr("%1 months").arg(days/30);
	else if ( days > 25 )
		strTimespan = "a month";
	else if ( days > 1 )
		strTimespan = QObject::tr("%1 days").arg(days);
	else if ( days == 1 || hours >= 22 )
		strTimespan = "a day";
	else if ( hours > 1 )
		strTimespan = QObject::tr("%1 hours").arg(hours);
	else if ( hours == 1 || minutes >= 45 )
		strTimespan = "an hour";
	else if ( minutes > 1 )
		strTimespan = QObject::tr("%1 minutes").arg(minutes);
	else if ( minutes == 1 || seconds >= 45 )
		strTimespan = "a minute";
	else
		strTimespan = QString("%1 seconds").arg(seconds);

	if ( ts.length() > 0 )
		return QObject::tr("%1 ago").arg(strTimespan);
	else
		return QObject::tr("in %1").arg(strTimespan);
}


}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
CurrentEventLayer::CurrentEventLayer(QObject* parent)
: Map::Layer(parent) {
	//setName("event");
	setVisible(true);

	_magFont = qApp->font();
	_magFont.setPointSize(_magFont.pointSize()*4);

	int mag_em = QFontMetrics(_magFont).height()*3/4;

	_boldFont = qApp->font();
	_boldFont.setPointSize(_boldFont.pointSize()*3/2);
	_boldFont.setBold(true);

	int bold_em = QFontMetrics(_boldFont).height()*3/4;

	_normalFont = qApp->font();
	_normalFont.setPointSize(_normalFont.pointSize()*3/2);

	int normal_em = QFontMetrics(_normalFont).height()*3/4;

	_magSize = QSize(2*mag_em, 2*mag_em);
	_headerHeight = normal_em * 2;
	_size = QSize(qMax(_magSize.width() + 10*bold_em, 300), _magSize.height());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void CurrentEventLayer::setEvent(Event *evt) {
	_isValid = evt != nullptr;
	if ( !_isValid )
		return;

	Magnitude *mag = Magnitude::Find(evt->preferredMagnitudeID());
	if ( mag )
		_magnitude = mag->magnitude().value();
	else
		_magnitude = Core::None;

	if ( !mag && !evt->preferredMagnitudeID().empty() ) {
		SEISCOMP_WARNING("Preferred magnitude not found: %s",
		                 evt->preferredMagnitudeID().c_str());
	}

	Origin *org = Origin::Find(evt->preferredOriginID());
	if ( !org ) {
		_isValid = false;
		return;
	}

	_originTime = org->time().value();
	_region = eventRegion(evt).c_str();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void CurrentEventLayer::draw(const Map::Canvas *canvas, QPainter &p) {
	if ( !_isValid ) return;

	p.save();

	QFontMetrics fm = p.fontMetrics();
	int em = fm.height();
	int em2 = em/2;

	QRect headerRect(9,9,_size.width(),_headerHeight);
	QRect rect(9,9+_headerHeight,_size.width(),_size.height());
	QRect magRect(9,9+_headerHeight,_magSize.width(),_magSize.height());

	p.fillRect(headerRect, QColor(0xE5,0xE5,0xE5));
	p.fillRect(rect,Qt::white);
	p.fillRect(magRect,QColor(0x7A,0xD4,0xE2));

	p.setPen(QColor(0x21,0x35,0x51));
	p.setFont(_magFont);

	if ( _magnitude ) {
		p.drawText(magRect, Qt::AlignCenter, QString::number(*_magnitude, 'f', 1));
	}
	else {
		p.drawText(magRect, Qt::AlignCenter, "-.-");
	}

	int agoHeight = QFontMetrics(_normalFont).height();

	p.setPen(QColor(0x33,0x33,0x33));

	if ( !_region.isEmpty() ) {
		p.setFont(_boldFont);
		QRect regionRect(9 + _magSize.width(), 9 + _headerHeight, _size.width() - _magSize.width(), _size.height() - agoHeight - em2);
		regionRect.adjust(em2, em2, -em2, -em2);
		p.drawText(regionRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, _region);
	}

	QRect agoRect(9 + _magSize.width(), 9 + _headerHeight + _size.height() - agoHeight-em2,_size.width() - _magSize.width(),agoHeight);
	agoRect.adjust(em2,0,-em2,0);
	p.setFont(_normalFont);
	if ( _originTime ) {
		p.drawText(agoRect, Qt::AlignLeft | Qt::AlignTop, timeAgoToString(Core::Time::UTC() - *_originTime));
	}

	headerRect.adjust(em2,0,-em2,0);
	p.drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter, tr("Latest event"));

	p.restore();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void CurrentEventLayer::calculateMapPosition(const Map::Canvas *canvas) {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool CurrentEventLayer::isInside(const QMouseEvent *event, const QPointF &geoPos) {
	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
