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


#include "eventheatlayer.h"

#include <seiscomp/datamodel/origin.h>
#include <seiscomp/datamodel/magnitude.h>
#include <seiscomp/gui/core/compat.h>
#include <seiscomp/gui/map/canvas.h>

#include <iostream>

#include <QApplication>


namespace Seiscomp {
namespace MapViewX {

namespace {


void fillEvent(EventHeatLayer::Event *evt,
               DataModel::Event *event,
               DataModel::Origin *org) {
	evt->location = QPointF(org->longitude(), org->latitude());

	DataModel::Magnitude *mag = DataModel::Magnitude::Find(event->preferredMagnitudeID());
	if ( mag ) {
		try {
			evt->magnitude = mag->magnitude().value();
		}
		catch ( ... ) {
			evt->magnitude = 0;
		}
	}
	else
		evt->magnitude = 0;
}


class HeatMapLegend : public Gui::Map::Legend {
	public:
		HeatMapLegend(QObject *parent = nullptr)
		: Gui::Map::Legend(parent), _gradient(nullptr) {
			setArea(Qt::AlignLeft | Qt::AlignBottom);
			setTitle(tr("Heatmap"));
			_lowerBound = _upperBound = -1;
		}


	public:
		void setGradient(const Gui::Gradient *gradient,
		                 double lowerBound, double upperBound) {
			if ( _gradient == gradient && lowerBound == _lowerBound && upperBound == _upperBound )
				return;

			_gradient = gradient;
			_lowerBound = lowerBound;
			_upperBound = upperBound;

			_items.clear();

			if ( !_gradient || _gradient->isEmpty() ) {
				setEnabled(false);
				return;
			}
			else
				setEnabled(true);

			int fontHeight = qApp->fontMetrics().height();
			int w = 0;

			double newRange = _upperBound - _lowerBound;

			double minValue = _gradient->begin().key();
			double maxValue = (--_gradient->end()).key();
			double range = maxValue - minValue;

			Gui::Gradient::const_iterator it;
			for ( it = _gradient->begin(); it != _gradient->end(); ++it ) {
				double newKey = ((it.key()-minValue) / range) * newRange + _lowerBound;
				_items.append(StringAtPos(QString::number(newKey), it.key(), -1));
			}

			int maxItemWidth = 0;
			for ( int i = 0; i < _items.count(); ++i ) {
				_items[i].width = QT_FM_WIDTH(qApp->fontMetrics(), _items[i].label);
				if ( maxItemWidth < _items[i].width ) {
					maxItemWidth = _items[i].width;
				}
			}

			if ( _items.front().width > maxItemWidth/2 ) {
				maxItemWidth = _items.front().width*2;
			}
			if ( _items.back().width > maxItemWidth/2 ) {
				maxItemWidth = _items.back().width*2;
			}

			w = maxItemWidth * _items.count() + (_items.count()-1)*fontHeight/2;

			int x = fontHeight/2;
			int spacing = w / (_items.count()-1);
			for ( int i = 0; i < _items.count(); ++i ) {
				_items[i].x = x;
				x += spacing;
			}

			_gradientImage = QImage(_items.back().x-_items.front().x+1, fontHeight, QImage::Format_ARGB32);
			QPainter gp(&_gradientImage);

			x = _items[0].x;
			double key = _items[0].key;

			for ( int i = 1, ix = 0; i < _items.count(); ++i ) {
				int toX = _items[i].x;
				double toKey = _items[i].key;

				int segmentWidth = toX-x;
				double keyWidth = toKey - key;

				for ( int gx = 0; gx < segmentWidth; ++gx ) {
					double k =key + gx*keyWidth/segmentWidth;

					QColor c = _gradient->colorAt(k);
					c.setAlpha(255);
					gp.setPen(c);
					gp.drawLine(ix+gx, 0, ix+gx, _gradientImage.height());
				}

				x = toX;
				key = toKey;

				ix += segmentWidth;
			}

			QColor c = _gradient->colorAt(maxValue);
			c.setAlpha(255);
			gp.setPen(c);
			gp.drawLine(_gradientImage.width()-1, 0, _gradientImage.width()-1, _gradientImage.height());

			_size = QSize(w+fontHeight, fontHeight*9/2);
		}


	// ----------------------------------------------------------------------
	//  Legend interface
	// ----------------------------------------------------------------------
	public:
		virtual void draw(const QRect &r, QPainter &p) {
			int fontHeight = qApp->fontMetrics().height();
			int halfFontHeight = fontHeight/2;
			int contentY = r.top() + halfFontHeight+fontHeight;

			QFont f = p.font();
			QFont bold(f);
			bold.setBold(true);
			p.setFont(bold);

			p.drawText(r.left() + halfFontHeight, r.top() + halfFontHeight,
			           r.width()-fontHeight, fontHeight,
			           Qt::AlignCenter, qApp->tr("Event locations"));

			p.setFont(f);
			p.drawImage(r.left()+_items.front().x,
			            contentY+halfFontHeight+fontHeight, _gradientImage);

			for ( int i = 0; i < _items.count(); ++i ) {
				int lhw = _items[i].width/2+1;

				if ( i == 0 )
					p.drawText(r.left() + _items[i].x, contentY,
					           _items[i].width, halfFontHeight+fontHeight,
					           Qt::AlignLeft  | Qt::AlignBottom,
					           _items[i].label);
				else if ( i == _items.count()-1 )
					p.drawText(r.left() + _items[i].x-_items[i].width, contentY,
					           _items[i].width, halfFontHeight+fontHeight,
					           Qt::AlignRight | Qt::AlignBottom,
					           _items[i].label);
				else
					p.drawText(r.left() + _items[i].x-lhw, contentY,
					           _items[i].width, halfFontHeight+fontHeight,
					           Qt::AlignHCenter | Qt::AlignBottom,
					           _items[i].label);

				p.drawLine(r.left() + _items[i].x, contentY+halfFontHeight+fontHeight,
				           r.left() + _items[i].x, contentY+2*fontHeight);
			}
		}


	private:
		const Gui::Gradient *_gradient;
		double _lowerBound, _upperBound;

		struct StringAtPos {
			StringAtPos() {}
			StringAtPos(const QString &l, double k, int p)
			: label(l), key(k), x(p), width(-1) {}

			QString label;
			double  key;
			int     x;
			int     width;
		};

		QVector<StringAtPos> _items;
		QImage _gradientImage;
};


}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventHeatLayer::EventHeatLayer(QObject* parent)
: Gui::Map::Layer(parent) {
	setName("heatmap");
	_gradient.setColorAt(0, QColor(0,0,0,0));
	_gradient.setColorAt(0.2, QColor(0,0,255,51));
	_gradient.setColorAt(0.4, QColor(0,255,255,102));
	_gradient.setColorAt(0.6, QColor(0,255,0,153));
	_gradient.setColorAt(0.8, QColor(255,255,0,204));
	_gradient.setColorAt(1, QColor(255,0,0,255));

	_composeMultiply = false;
	generateLUT();

	_updateTimer.setSingleShot(true);
	connect(&_updateTimer, SIGNAL(timeout()), this, SLOT(updateCanvas()));

	_legend = new HeatMapLegend(this);
	addLegend(_legend);

	static_cast<HeatMapLegend*>(_legend)->setGradient(&_gradient, _gradientLUT.lowerBound(), _gradientLUT.upperBound());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventHeatLayer::setCompositionMode(bool multiply) {
	if ( _composeMultiply == multiply ) return;
	_composeMultiply = multiply;
	generateLUT();
	updateCanvas();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventHeatLayer::clear() {
	if ( _events.isEmpty() ) return;
	_events.clear();

	if ( !_updateTimer.isActive() )
		_updateTimer.start(1000);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventHeatLayer::addEvent(DataModel::Event *e, bool) {
	EventMap::iterator it = _events.find(e->publicID());

	DataModel::Origin *org = DataModel::Origin::Find(e->preferredOriginID());
	if ( org ) {
		EventPtr evt;

		if ( it == _events.end() ) {
			evt = new Event();
			_events[e->publicID()] = evt;
		}
		else
			evt = it.value();

		fillEvent(evt.get(), e, org);

		if ( !_updateTimer.isActive() )
			_updateTimer.start(1000);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventHeatLayer::updateEvent(DataModel::Event *e) {
	EventMap::iterator it = _events.find(e->publicID());
	if ( it == _events.end() ) return;

	DataModel::Origin *org = DataModel::Origin::Find(e->preferredOriginID());
	if ( org ) {
		fillEvent(it.value().get(), e, org);

		if ( !_updateTimer.isActive() )
			_updateTimer.start(1000);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventHeatLayer::removeEvent(DataModel::Event *e) {
	EventMap::iterator it = _events.find(e->publicID());
	if ( it == _events.end() ) return;
	_events.erase(it);

	if ( !_updateTimer.isActive() )
		_updateTimer.start(1000);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventHeatLayer::baseBufferUpdated(Gui::Map::Canvas *canvas, QPainter &painter) {
	_updateTimer.stop();

	if ( _events.isEmpty() ) {
		_legend->setEnabled(false);
		return;
	}

	_legend->setEnabled(true);

	QImage &base = canvas->buffer();
	QRgb *data = (QRgb*)base.bits();

	int width = base.width();
	int height = base.height();
	int numberOfPixels = width*height;

	if ( _intensityBuffer.count() != numberOfPixels )
		_intensityBuffer.resize(numberOfPixels);

	_intensityBuffer.fill(0);

	EventMap::iterator it;

	float diameterScale = 4*(1+log(qMax(1.0f, canvas->pixelPerDegree())));

	float *intensityData = _intensityBuffer.data();
	for ( it = _events.begin(); it != _events.end(); ++it ) {
		Event *evt = it.value().get();
		QPoint coords;
		if ( !canvas->projection()->project(coords, evt->location) )
			continue;

		int cx = coords.x();
		int cy = coords.y();
		int diameter = qMax(2, (int)(evt->magnitude*diameterScale));
		//float intensityScale = qMax(0.02f, evt->magnitude*0.02f);
		//float intensityScale = log10(qMax(1.0f, evt->magnitude))*0.1;
		float intensityScale = 1.0f;

		int x0 = cx-diameter;
		int y0 = cy-diameter;
		int x1 = cx+diameter+1;
		int y1 = cy+diameter+1;

		if ( x0 < 0 ) x0 = 0; else if ( x0 >= width ) continue;
		if ( x1 <= 0 ) continue; else if ( x1 > width ) x1 = width;
		if ( y0 < 0 ) y0 = 0; else if ( y0 >= height ) continue;
		if ( y1 <= 0 ) continue; else if ( y1 > height ) y1 = height;

		int ofs = y0*width+x0;
		int pw = x1-x0;
		int ph = y1-y0;

		int diameterSquared = diameter*diameter;

		for ( int y = 0, ry = y0-cy; y < ph; ++y, ++ry, ofs += width ) {
			for ( int x = 0, rx = x0-cx; x < pw; ++x, ++rx ) {
				int distanceSquared = rx*rx + ry*ry;
				if ( distanceSquared > diameterSquared ) continue;
				intensityData[ofs+x] += intensityScale*(1.0f - (float)distanceSquared/(float)diameterSquared);
			}
		}
	}

#if 1
	float maxIntensity = 0;
	for ( int i = 0; i < numberOfPixels; ++i ) {
		if ( intensityData[i] > maxIntensity )
			maxIntensity = intensityData[i];
	}

	maxIntensity = ceil(maxIntensity);
	float intensityScale = _gradientLUT.upperBound() / maxIntensity;
#else
	float intensityScale = 1.0f;
#endif

	if ( _composeMultiply ) {
		for ( int i = 0; i < numberOfPixels; ++i, ++data, ++intensityData ) {
			if ( *intensityData > 0 ) {
				QRgb c = _gradientLUT.valueAt(*intensityData*intensityScale);
				*data = qRgba((qRed(*data)*qRed(c)) / 255,
				              (qGreen(*data)*qGreen(c)) / 255,
				              (qBlue(*data)*qBlue(c)) / 255,
				              qAlpha(*data));
			}
		}
	}
	else {
		for ( int i = 0; i < numberOfPixels; ++i, ++data, ++intensityData ) {
			if ( *intensityData > 0 ) {
				QRgb c = _gradientLUT.valueAt(*intensityData*intensityScale);
				int alpha = qAlpha(c);
				int invertedAlpha = 255-alpha;
				*data = qRgba((qRed(*data)*invertedAlpha + qRed(c)*alpha) / 255,
				              (qGreen(*data)*invertedAlpha + qGreen(c)*alpha) / 255,
				              (qBlue(*data)*invertedAlpha + qBlue(c)*alpha) / 255,
				              qAlpha(*data));
			}
		}
	}

	static_cast<HeatMapLegend*>(_legend)->setGradient(&_gradient, 0, maxIntensity);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventHeatLayer::setVisible(bool f) {
	if ( isVisible() == f ) return;
	Gui::Map::Layer::setVisible(f);
	updateCanvas();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventHeatLayer::generateLUT() {
	_gradientLUT.generateFrom(_gradient);
	if ( _composeMultiply ) {
		for ( int i = 0; i < _gradientLUT.size(); ++i ) {
			QRgb &c = _gradientLUT[i];
			int alpha = qAlpha(c);
			int invertedAlpha = 255-alpha;
			c = qRgb((255*invertedAlpha + qRed(c)*alpha) / 255,
			         (255*invertedAlpha + qGreen(c)*alpha) / 255,
			         (255*invertedAlpha + qBlue(c)*alpha) / 255);
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventHeatLayer::updateCanvas() {
	if ( canvas() ) {
		canvas()->updateBuffer();
		emit updateRequested();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
