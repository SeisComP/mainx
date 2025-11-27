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


#include "scalelayer.h"
#include <seiscomp/math/math.h>
#include <seiscomp/gui/map/canvas.h>
#include <cmath>


namespace Seiscomp::MapViewX {


namespace {


double getSpacing(double width, int steps, bool onlyIntegerSpacing) {
	if ( steps <= 0 ) return 0;

	double fSpacing = width / ((double)steps+1e-10);
	int pow10 = int(floor(log10(fSpacing)));
	fSpacing /= pow(10, pow10);
	int spacing = (int)round(fSpacing);

	if ( onlyIntegerSpacing && (pow10 < 0) ) {
		pow10 = 0;
		spacing = 1;
	}

	return (double)spacing * pow(10, pow10);
}


}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
ScaleLayer::ScaleLayer(QObject* parent, Qt::Alignment alignment)
: Gui::Map::Layer(parent), _alignment(alignment) {
	setName(tr("scale"));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void ScaleLayer::draw(const Gui::Map::Canvas *canvas, QPainter &p) {
	int w = canvas->width();
	int h = canvas->height();
	double ppkm;

	QPointF left, right;
	if ( canvas->projection()->unproject(left, QPoint(w/4, h/2)) &&
	     canvas->projection()->unproject(right, QPoint(w*3/4-1, h/2)) ) {
		double dist, az, baz;
		Math::Geo::delazi(left.y(), left.x(), right.y(), right.x(),
		                  &dist, &az, &baz);
		ppkm = Math::Geo::km2deg(w/2/dist);
	}
	else
		ppkm = Math::Geo::km2deg(canvas->pixelPerDegree());

	int steps = 4;
	int targetWidth = 150;
	double scaledTargetKm = targetWidth / ppkm;

	double spacing = getSpacing(scaledTargetKm, steps, false);
	double realTargetKm = spacing * steps;

	int realWidth = realTargetKm * ppkm;
	QString label;

	if ( realTargetKm < 1 )
		label = QString(tr("%1 m").arg(realTargetKm*1E3));
	else
		label = QString(tr("%1 km").arg(realTargetKm));

	QRect labelRect = p.fontMetrics().boundingRect(label);
	int eh = labelRect.height();

	if ( labelRect.width() < realWidth )
		labelRect.setWidth(realWidth);

	labelRect.setHeight(eh+4+eh/2);

	labelRect.adjust(-4,-4,4,4);

	if ( _alignment & Qt::AlignTop )
		labelRect.moveTop(10);
	else if ( _alignment & Qt::AlignBottom )
		labelRect.moveBottom(h-10);
	else if ( _alignment & Qt::AlignVCenter )
		labelRect.moveTop((h-labelRect.height())/2);

	if ( _alignment & Qt::AlignLeft )
		labelRect.moveLeft(10);
	else if ( _alignment & Qt::AlignRight )
		labelRect.moveRight(w-10);
	else if ( _alignment & Qt::AlignHCenter )
		labelRect.moveLeft((w-labelRect.width())/2);

	p.save();
	p.setRenderHint(QPainter::Antialiasing, false);
	p.setPen(Qt::darkGray);
	p.setBrush(QColor(255,255,255,192));
	p.drawRect(labelRect);
	p.setPen(Qt::black);
	p.drawText(QRect(labelRect.left(), labelRect.top()+4, labelRect.width(), eh), Qt::AlignCenter, label);

	int startY = labelRect.bottom()-4-eh/2;
	int startX = labelRect.left() + (labelRect.width()-realWidth+1) / 2;

	int ofsX = spacing*ppkm;

	for ( int i = 0; i < steps; ++i ) {
		p.fillRect(startX, startY, ofsX, eh/2, i % 2 ? Qt::lightGray: Qt::black);
		startX += ofsX;
	}

	p.restore();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
