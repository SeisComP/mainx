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


#ifndef SEISCOMP_MAPVIEWX_LAYERS_NETWORKLAYER_H
#define SEISCOMP_MAPVIEWX_LAYERS_NETWORKLAYER_H


#ifndef Q_MOC_RUN
#include <seiscomp/datamodel/inventory.h>
#include <seiscomp/gui/core/gradient.h>
#include <seiscomp/gui/datamodel/stationsymbol.h>
#include <seiscomp/gui/map/layer.h>

#include <map>

#include "../settings.h"
#include "stationsymbol.h"
#endif


namespace Seiscomp {
namespace MapViewX {


class NetworkLayer;


class NetworkLayerGradient : public Gui::Gradient {
	public:
		NetworkLayerGradient() = default;

	public:
		QString title;
		QColor  unsetColor;
};


class NetworkLayerSymbol : public StationSymbol {
	public:
		explicit NetworkLayerSymbol(NetworkLayer *layer,
		                            DataModel::Station *station);


	public:
		DataModel::Station *model() const { return _model; }
		Settings::StationData *data() const { return _data; }

		void setDefaultVisibility();

		bool setSelected(bool);
		bool isSelected() const { return _selected; }

		void setColor(QColor c);
		void setColorFromValue(double value);
		QColor color() const { return _color; }

		void setValue(double v) { _value = v; }
		double value() const { return _value; }

		void updateColor();

		void setAnnotation(const QString &a) { _annotation = a; }
		const QString &annotation() const { return _annotation; }

		void setState(Settings::State state) { _state = state; }
		Settings::State state() const { return _state; }


	private:
		DataModel::Station    *_model;
		Settings::StationData *_data;
		bool                   _selected;
		double                 _value;
		QColor                 _color;
		QString                _annotation;
		NetworkLayer          *_layer;
		Settings::State        _state;

	friend class NetworkLayer;
};


class NetworkLayerLegend : public Gui::Map::Legend {
	public:
		NetworkLayerLegend(QObject *parent = nullptr);

		virtual void draw(const QRect &rect, QPainter &painter);
		virtual void contextResizeEvent(const QSize &size);

		void updateFrom(NetworkLayer *layer);

	private:
		void updateLayout();

	private:
		QVector< QPair<QString, QColor> > _items;
		int                               _columns;
		int                               _columnWidth;
		int                               _maxColumns;
};


/**
 * @brief The NetworkLayer class
 */
class NetworkLayer : public Gui::Map::Layer {
	Q_OBJECT


	// ----------------------------------------------------------------------
	//  Public types
	// ----------------------------------------------------------------------
	public:
		//! Associative container to map network codes to colors
		typedef std::map<std::string, QColor> NetworkColors;

		enum ColorMode {
			Default,
			Network,
			GroundMotion,
			QC
		};



	// ----------------------------------------------------------------------
	//  X'truction
	// ----------------------------------------------------------------------
	public:
		NetworkLayer(QObject* parent = nullptr);
		~NetworkLayer();

	// ----------------------------------------------------------------------
	//  Public interface
	// ----------------------------------------------------------------------
	public:
		/**
		 * @brief Creates station symbols from all stations in the passed
		 *        inventory where the epoch is open or valid for a passed
		 *        reference time.
		 * @param inv The inventory pointer
		 * @param time The reference time for which the station must be
		 *             operational
		 */
		void setInventory(DataModel::Inventory *inv,
		                  const Core::Time *time = nullptr);

		/**
		 * @brief Removes all symbols
		 */
		void clear();

		/**
		 * @brief Returns the current network code color map
		 * @return The string:QColor map
		 */
		const NetworkColors &networkColors() const { return _networkColors; }

		void setGMGradient(const NetworkLayerGradient &);
		const NetworkLayerGradient *gmGradient() const { return &_gmGradient; }

		const NetworkLayerGradient *qcGradient() const;

		/**
		 * @brief Sets the station symbol color mode.
		 * @param mode The mode flag
		 * @param force If true then the values will be recomputed even if the
		 *        requested color mode is already active
		 */
		void setColorMode(ColorMode mode, bool force = false);
		ColorMode colorMode() const { return _colorMode; }

		void setActiveQCParameter(const std::string &);
		const std::string &activeQCParameter() const { return _activeQCParameter; }

		void setStationsVisible(QSet<const DataModel::Station *> *);


	// ----------------------------------------------------------------------
	//  Signals
	// ----------------------------------------------------------------------
	signals:
		void stationEntered(Seiscomp::DataModel::Station *station);
		void stationLeft();
		void stationClicked(Seiscomp::DataModel::Station *station);


	// ----------------------------------------------------------------------
	//  Public slots
	// ----------------------------------------------------------------------
	public slots:
		/**
		 * @brief Sets if station annotations should be shown or not. The
		 *        default is true.
		 * @param enable The visibility state
		 * @param withChannelCode
		 */
		void setShowAnnotations(bool enable);

		/**
		 * @brief Whether to include the preferred channel code. The default
		 *        is false.
		 * @param enable The visibility state
		 */
		void setShowChannelCodes(bool enable);

		/**
		 * @brief Sets if station issues should be indicated with an
		 *        additional symbol or not.
		 * @param enable The visibility state
		 */
		void setShowIssues(bool enable);

		/**
		 * @brief Updates the internal render state for each station symbol.
		 */
		void tick();


	// ----------------------------------------------------------------------
	//  Layer interface
	// ----------------------------------------------------------------------
	public:
		bool isInside(const QMouseEvent *event, const QPointF &geoPos) override;
		void calculateMapPosition(const Gui::Map::Canvas *canvas) override;
		void draw(const Gui::Map::Canvas *canvas, QPainter &p) override;


	// ----------------------------------------------------------------------
	//  Event interface
	// ----------------------------------------------------------------------
	protected:
		void handleLeaveEvent() override;
		bool filterMousePressEvent(QMouseEvent *event, const QPointF &geoPos) override;
		bool filterMouseReleaseEvent(QMouseEvent *event, const QPointF &geoPos) override;
		bool filterMouseMoveEvent(QMouseEvent *event, const QPointF &geoPos) override;
		bool filterMouseDoubleClickEvent(QMouseEvent *event, const QPointF &geoPos) override;


	// ----------------------------------------------------------------------
	//  Private methods
	// ----------------------------------------------------------------------
	private:
		void updateAnnotations();
		void disposeSymbols();
		void updateColor(NetworkLayerSymbol *symbol);


	// ----------------------------------------------------------------------
	//  Private members
	// ----------------------------------------------------------------------
	private:
		typedef QVector<NetworkLayerSymbol*> Symbols;
		typedef std::map<void*, NetworkLayerSymbol*> StationSymbolMap;

		bool                                     _showAnnotations;
		bool                                     _showChannelCodes;
		bool                                     _showIssues;
		ColorMode                                _colorMode;
		std::string                              _activeQCParameter;
		Symbols                                  _stationSymbols;
		NetworkColors                            _networkColors;
		StationSymbolMap                         _stationSymbolLookup;
		NetworkLayerSymbol                      *_currentSymbol;
		NetworkLayerSymbol                      *_currentClickSymbol;
		NetworkLayerLegend                      *_legend;
		NetworkLayerGradient                     _gmGradient;
		QMap<std::string, NetworkLayerGradient>  _qcGradients;

		mutable NetworkLayerSymbol              *_isInsideSymbol;
};


}
}


#endif
