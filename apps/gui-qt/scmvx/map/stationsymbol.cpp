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
#include <seiscomp/gui/map/canvas.h>
#include <seiscomp/gui/map/projection.h>
#include <seiscomp/gui/core/application.h>

#include "stationsymbol.h"

#include <QMutex>


namespace Seiscomp {
namespace MapViewX {


namespace {


void blurImage(QImage &img, int radius) {
	if ( radius < 1 ) return;

	QImage out = QImage(img.size(), QImage::Format_ARGB32);
	int div = (radius*2+1)*(radius*2+1);

	int w = out.width();
	int h = out.height();

	const QRgb *bits = (const QRgb*)img.bits();
	QRgb *out_bits = (QRgb*)out.bits();

	for ( int y = 0; y < h; ++y ) {
		for ( int x = 0; x < w; ++x, ++bits, ++out_bits ) {
			int r = 0; int g = 0; int b = 0; int a = 0;
			div = 0;
			for ( int ry = -radius; ry <= radius; ++ry ) {
				int ny = y+ry;
				if ( ny < 0 || ny >= h ) continue;
				for ( int rx = -radius; rx <= radius; ++rx ) {
					int nx = x+rx;
					if ( nx < 0 || nx >= w ) continue;
					QRgb c = *(bits + ry*w + rx);
					r += qRed(c);
					g += qGreen(c);
					b += qBlue(c);
					a += qAlpha(c);
					++div;
				}
			}

			r /= div;
			g /= div;
			b /= div;
			a /= div;

			*out_bits = qRgba(r,g,b,a);
		}
	}

	img = out;
}


class ShapeCache : public QMap<int, QPair<int, QPolygon*> > {
	public:
		typedef QPair<int, QPolygon*> RefCountedPolygon;
		typedef QMap<int, RefCountedPolygon> Base;

		~ShapeCache() {
			for ( auto it = begin(); it != end(); ++it )
				delete it.value().second;
		}

		QPolygon *shape(int size) {
			if ( auto it = find(size); it != end() ) {
				++it.value().first;
				return it.value().second;
			}

			QPolygon *poly = new QPolygon;
			StationSymbol::generateShape(*poly, 0, 0, size);
			insert(size, RefCountedPolygon(1, poly));

			SEISCOMP_DEBUG("Cache shape with size %d", size);

			return poly;
		}

		void dropShape(int size) {
			auto it = find(size);
			if ( it == end() ) {
				return;
			}

			--it.value().first;

			if ( it.value().first == 0 ) {
				SEISCOMP_DEBUG("Uncache shape with size %d", it.key());
				delete it.value().second;
				erase(it);
			}
		}
};


ShapeCache shapeCache;


}


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
StationSymbol::StationSymbol()
: Symbol(nullptr) {
	setWidth(SCScheme.map.stationSize);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
StationSymbol::StationSymbol(double latitude, double longitude)
: Symbol(latitude, longitude, nullptr) {
	setWidth(SCScheme.map.stationSize);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
StationSymbol::~StationSymbol() {
	if ( _stationPolygon ) {
		shapeCache.dropShape(_width);
	}

	if ( _framePolygon ) {
		shapeCache.dropShape(_width + _frameSize * 2);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool StationSymbol::isInside(int px, int py) const {
	if ( !_stationPolygon ) {
		return false;
	}
	return _stationPolygon->containsPoint(QPoint(px - x(), py - y()), Qt::OddEvenFill);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationSymbol::setPen(const QColor &color) {
	_penColor = color;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
QColor StationSymbol::pen() const {
	return _penColor;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationSymbol::setFill(const QColor &color) {
	_fillColor = color;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
QColor StationSymbol::fill() const {
	return _fillColor;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationSymbol::setPenWidth(qreal penWidth) {
	_penWidth = penWidth;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
qreal StationSymbol::penWidth() const {
	return _penWidth;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationSymbol::setFrameSize(int frameSize) {
	if ( _framePolygon ) {
		shapeCache.dropShape(_width + _frameSize*2);
		_framePolygon = nullptr;
	}

	_frameSize = frameSize;

	if ( _frameSize > 0 ) {
		_framePolygon = shapeCache.shape(_width + _frameSize*2);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationSymbol::setFrameColor(const QColor &color) {
	_frameColor = color;
}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationSymbol::setWidth(int w) {
	_width = w;
	_stationPolygon = shapeCache.shape(_width);
	setFrameSize(_frameSize);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int StationSymbol::width() const {
	return _width;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationSymbol::customDraw(const Gui::Map::Canvas *, QPainter &painter) {
	if ( !_stationPolygon ) {
		return;
	}

	QPen pen(Qt::MiterJoin);
	QBrush brush(Qt::SolidPattern);

	painter.translate(x(), y());

	if ( _frameSize > 0 ) {
		painter.setPen(_frameColor);
		brush.setColor(_frameColor);
		painter.setBrush(brush);

		painter.translate(0, _frameSize*2);
		painter.drawPolygon(*_framePolygon);
		painter.translate(0, -_frameSize*2);
	}

	pen.setWidthF(_penWidth);
	pen.setColor(_penColor);
	painter.setPen(pen);

	brush.setColor(_fillColor);
	painter.setBrush(brush);

	painter.drawPolygon(*_stationPolygon);
	painter.translate(-x(), -y());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationSymbol::drawShadow(QPainter &painter) {
	static int blurRadius = 1;
	static int shadowCacheSize = -1;
	static QImage shadowCache;

	if ( !_stationPolygon ) {
		return;
	}

	if ( shadowCacheSize != _width ) {
		int dim = _width * 2 + blurRadius * 2;
		shadowCache = QImage(dim, dim, QImage::Format_ARGB32);

		shadowCache.fill(0);

		{
			QPainter p(&shadowCache);
			p.setRenderHint(QPainter::Antialiasing);
			p.setPen(Qt::NoPen);
			p.setBrush(QColor(64, 64, 64, 160));

			p.translate(blurRadius, dim-blurRadius);
			p.scale(0.7, 0.5);
			p.shear(-1, 0.2);
			p.drawPolygon(*_stationPolygon);
			p.resetTransform();
		}

		blurImage(shadowCache, blurRadius);
		shadowCacheSize = _width;
	}

	painter.drawImage(x() - blurRadius, y() - shadowCache.height() + blurRadius, shadowCache);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationSymbol::generateShape(QPolygon &polygon, int posX, int posY, int radius) {
	switch ( SCScheme.map.stationSymbol ) {
		default:
		case Gui::Scheme::Map::StationSymbol::Triangle:
			polygon
				<< QPoint(posX, posY - radius)
				<< QPoint(posX + int(0.867 * radius), posY + int(0.5 * radius))
				<< QPoint(posX - int(0.867 * radius), posY + int(0.5 * radius));
			break;
		case Gui::Scheme::Map::StationSymbol::Diamond:
		{
			int radius1 = radius / 2;
			int radius2 = radius / 4;
			polygon
				<< QPoint(posX, posY - radius - radius1 - radius2)
				<< QPoint(posX + int(0.867 * radius), posY - radius - radius1)
				<< QPoint(posX, posY)
				<< QPoint(posX - int(0.867 * radius), posY - radius - radius1);
			break;
		}
		case Gui::Scheme::Map::StationSymbol::Box:
		{
			int radius1 = radius;
			int radius2 = radius / 3;
			int halfWidth = radius1;
			int height = radius * 2 * 3 / 4;
			polygon
				<< QPoint(posX, posY)
				<< QPoint(posX - radius2, posY - radius2)
				<< QPoint(posX - halfWidth, posY - radius2)
				<< QPoint(posX - halfWidth, posY - radius2 - height)
				<< QPoint(posX + halfWidth, posY - radius2 - height)
				<< QPoint(posX + halfWidth, posY - radius2)
			<< QPoint(posX + radius2, posY - radius2);
			break;
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
