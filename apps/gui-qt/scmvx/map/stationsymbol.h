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


#ifndef SEISCOMP_MAPVIEWX_STATIONSYMBOL_H
#define SEISCOMP_MAPVIEWX_STATIONSYMBOL_H


#include <QColor>

#include <seiscomp/gui/map/mapsymbol.h>


namespace Seiscomp::MapViewX {


class StationSymbol : public Gui::Map::Symbol {
	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		StationSymbol();
		StationSymbol(double latitude, double longitude);
		~StationSymbol();


	// ----------------------------------------------------------------------
	//  Public interface
	// ----------------------------------------------------------------------
	public:
		void setFill(const QColor &color);
		QColor fill() const;

		void setPen(const QColor &color);
		QColor pen() const;

		void setPenWidth(qreal);
		qreal penWidth() const;

		void setFrameSize(int frameSize);
		int frameSize() const;

		void setFrameColor(const QColor &color);

		void setWidth(int size);
		int width() const;

		static void generateShape(QPolygon &poly, int posX, int posY, int radius);


	// ----------------------------------------------------------------------
	//  Symbol interface
	// ----------------------------------------------------------------------
	public:
		bool isInside(int x, int y) const override;
		virtual void drawShadow(QPainter &painter);


	protected:
		void customDraw(const Gui::Map::Canvas *canvas, QPainter &painter) override;


	// ----------------------------------------------------------------------
	//  Private members
	// ----------------------------------------------------------------------
	private:
		int           _width{0};
		int           _frameSize{0};
		QColor        _frameColor;
		QPolygon     *_stationPolygon{nullptr};
		QPolygon     *_framePolygon{nullptr};

		QColor        _fillColor{Qt::black};
		QColor        _penColor{Qt::black};
		qreal         _penWidth{1.5};
};


inline int StationSymbol::frameSize() const {
	return _frameSize;
}


}


#endif
