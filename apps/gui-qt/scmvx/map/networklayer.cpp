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


#include <seiscomp/gui/core/application.h>
#include <seiscomp/gui/core/compat.h>
#include <seiscomp/gui/core/icon.h>
#include <seiscomp/gui/map/canvas.h>

#include <algorithm>

#include "networklayer.h"
#include "settings.h"


using namespace std;


namespace Seiscomp {
namespace MapViewX {

namespace {


static QColor defaultColor(45, 105, 192);
static QColor selectedColor(60, 139, 255);
static QColor defaultFrameColor(64,64,64,128);
//static QColor selectedFrameColor(60,139,255);
static QColor selectedFrameColor(255,255,255);
static QColor hoverFrameColor(255,255,255,192);
static qreal defaultFrameWidth = 1.5;


typedef QList<QRect> Row;
typedef QVector<Row> Grid;


#define goldenRationConjugate 0.618033988749895


bool topToBottom(const NetworkLayerSymbol *s1, const NetworkLayerSymbol *s2) {
	return s1->latitude() > s2->latitude();
}


void drawWarningSymbol(QPainter &painter, const QPoint &center, const QPixmap &pixmap) {
	static QColor errorBorder(192, 0, 0);
	int size = painter.fontMetrics().height();
	painter.setPen(QPen(errorBorder, qMax(2, size / 5)));
	painter.setBrush(Qt::white);
	painter.drawEllipse(center, size, size);
	painter.drawPixmap(center.x() - size / 2, center.y() - size / 2, pixmap);
}


NetworkLayerGradient *currentGradient = nullptr;


}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
NetworkLayerSymbol::NetworkLayerSymbol(NetworkLayer *layer,
                                       DataModel::Station *station,
                                       Gui::Map::AnnotationItem *annotation)
: _annotation(annotation)
, _model(station)
, _selected(false)
, _value(-1)
, _layer(layer)
, _state(Settings::OK) {
	setDefaultVisibility();
	setColor(defaultColor);
	setPen(defaultFrameColor);

	auto it = global.stationConfig.find(_model);
	if ( it != global.stationConfig.end() ) {
		_data = it->second.get();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
NetworkLayerSymbol::~NetworkLayerSymbol() {
	delete _annotation;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayerSymbol::setDefaultVisibility() {
	try {
		setLocation(_model->latitude(), _model->longitude());
		setVisible(true);
	}
	catch ( ... ) {
		setVisible(false);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool NetworkLayerSymbol::setSelected(bool f) {
	if ( _selected == f ) return false;
	_selected = f;
	setPen(_selected ? selectedFrameColor : defaultFrameColor);
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayerSymbol::setColor(QColor c) {
	_color = c;
	setFill(_color);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayerSymbol::setColorFromValue(double value) {
	setValue(value);
	updateColor();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayerSymbol::updateColor() {
	if ( _value < 0 ) {
		setColor(currentGradient ? currentGradient->unsetColor : QColor(0,0,0,128));
	}
	else if ( currentGradient ) {
		setColor(currentGradient->colorAt(_value, true));
	}
	else {
		setColor(Qt::white);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayerSymbol::calculateMapPosition(const Seiscomp::Gui::Map::Canvas *canvas) {
	StationSymbol::calculateMapPosition(canvas);
	if ( _annotation ) {
		_annotation->visible = !isClipped() && isVisible();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayerSymbol::customDraw(const Gui::Map::Canvas *canvas, QPainter &painter) {
	StationSymbol::customDraw(canvas, painter);

	if ( _annotation ) {
		_annotation->updateLabelRect(painter, _position - QPoint(0, width() + painter.fontMetrics().height() / 2));
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
NetworkLayerLegend::NetworkLayerLegend(QObject *parent)
: Gui::Map::Legend(parent) {
	_maxColumns = 1;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayerLegend::contextResizeEvent(const QSize &size) {
	updateLayout();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayerLegend::draw(const QRect &rect, QPainter &painter) {
	int fontHeight = QFontMetricsF(qApp->font()).height();
	int w = rect.width();
	int x = rect.left() + fontHeight/2;
	int idx = 0;

	for ( int c = 0; c < _columns; ++c ) {
		int y = rect.top() + fontHeight/2;
		int textOffset = fontHeight*3/2;
		int textWidth = w - fontHeight - textOffset;

		int itemsPerColumn = (_items.count() + _columns - 1) / _columns;
		int cnt = _items.count() - itemsPerColumn * c;
		if ( cnt > itemsPerColumn ) cnt = itemsPerColumn;

		for ( int i = 0; i < cnt; ++i, ++idx ) {
			painter.fillRect(x, y, fontHeight, fontHeight, _items[idx].second);
			painter.drawText(x + textOffset, y, textWidth, fontHeight,
			                 Qt::AlignLeft | Qt::AlignVCenter, _items[idx].first);
			y += fontHeight*3/2;
		}

		x += _columnWidth + fontHeight*4/2;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayerLegend::updateFrom(NetworkLayer *layer) {
	switch ( layer->colorMode() ) {
		case NetworkLayer::Network:
		{
			setTitle(tr("Networks"));

			const NetworkLayer::NetworkColors &colors = layer->networkColors();

			_items.clear();
			NetworkLayer::NetworkColors::const_iterator it;
			for ( it = colors.begin(); it != colors.end(); ++it )
				_items.append(QPair<QString, QColor>(it->first.c_str(), it->second));

			_maxColumns = 6;
			updateLayout();

			setEnabled(true);
			break;
		}

		case NetworkLayer::GroundMotion:
		{
			_items.clear();

			auto gradient = layer->gmGradient();
			auto it = gradient->begin();

			setTitle(gradient->title);

			QColor currentColor = it.value().first;
			++it;

			_items.append(QPair<QString, QColor>(QString("<%1").arg(it.key()), currentColor));

			for ( ; it != gradient->end(); ++it ) {
				_items.append(QPair<QString, QColor>(QString("%1").arg(it.key()), it.value().first));
			}

			if ( gradient->unsetColor.isValid() ) {
				_items.append(QPair<QString, QColor>(tr("Unset"), gradient->unsetColor));
			}

			_maxColumns = 1;
			updateLayout();

			setEnabled(true);
			break;
		}

		case NetworkLayer::QC:
		{
			auto gradient = layer->qcGradient();
			if ( gradient ) {
				_items.clear();
				auto it = gradient->begin();

				setTitle(gradient->title);

				QColor currentColor = it.value().first;
				QString currentTag = it.value().second;
				qreal lastValue = it.key();
				++it;

				for ( ; it != gradient->end(); ++it ) {
					lastValue = it.key();
					if ( currentTag.isEmpty() )
						_items.append(QPair<QString, QColor>(QString("< %1").arg(lastValue), currentColor));
					else
						_items.append(QPair<QString, QColor>(currentTag, currentColor));
					currentColor = it.value().first;
					currentTag = it.value().second;
				}

				if ( currentTag.isEmpty() )
					_items.append(QPair<QString, QColor>(QString(">= %1").arg(lastValue), currentColor));
				else
					_items.append(QPair<QString, QColor>(currentTag, currentColor));

				if ( gradient->unsetColor.isValid() ) {
					_items.append(QPair<QString, QColor>(tr("Unset"), gradient->unsetColor));
				}

				_maxColumns = 1;
				updateLayout();

				setEnabled(true);
				break;
			}
			// Fall through to default case
		}

		default:
			setTitle(QString());
			setEnabled(false);
			_items.clear();
			break;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayerLegend::updateLayout() {
	QSize size = layer()->size();

	if ( !size.isValid() ) return;

	int ch = size.height();
	int fontHeight = QFontMetricsF(qApp->font()).height();

	_columns = 1;

	_columnWidth = 0;
	for ( int i = 0; i < _items.count(); ++i ) {
		int itemWidth = QT_FM_WIDTH(QFontMetricsF(qApp->font()), _items[i].first);
		if ( itemWidth > _columnWidth )
			_columnWidth = itemWidth;
	}

	_size.setWidth((_columnWidth + fontHeight*3/2)*_columns + fontHeight + fontHeight/2*(_columns-1));
	_size.setHeight(qMax(((_items.count()+_columns-1)/_columns)*fontHeight*3/2+fontHeight/2, 0));

	if ( ch <= 0 ) return;

	// TODO: What is a good offset value?
	while ( (_size.height() > ch - 30) && (_columns < _maxColumns) ) {
		++_columns;
		_size.setWidth((_columnWidth + fontHeight*3/2)*_columns + fontHeight + fontHeight/2*(_columns-1));
		_size.setHeight(qMax(((_items.count()+_columns-1)/_columns)*fontHeight*3/2+fontHeight/2, 0));
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
NetworkLayer::NetworkLayer(QObject *parent)
: Gui::Map::Layer(parent)
, _showIssues(true)
, _colorMode(Network)
, _currentSymbol(nullptr)
, _isInsideSymbol(nullptr) {
	setName("stations");
	_activeQCParameter = "delay";

	_gmGradient.title = "PGV in nm/s";
	_gmGradient.unsetColor = SCScheme.colors.gm.gmNotSet;
	_gmGradient.setColorAt(0, SCScheme.colors.gm.gm0);
	_gmGradient.setColorAt(200, SCScheme.colors.gm.gm1);
	_gmGradient.setColorAt(400, SCScheme.colors.gm.gm2);
	_gmGradient.setColorAt(800, SCScheme.colors.gm.gm3);
	_gmGradient.setColorAt(1500, SCScheme.colors.gm.gm4);
	_gmGradient.setColorAt(4000, SCScheme.colors.gm.gm5);
	_gmGradient.setColorAt(12000, SCScheme.colors.gm.gm6);
	_gmGradient.setColorAt(30000, SCScheme.colors.gm.gm7);
	_gmGradient.setColorAt(60000, SCScheme.colors.gm.gm8);
	_gmGradient.setColorAt(150000, SCScheme.colors.gm.gm9);

	auto g = &_qcGradients["delay"];
	g->title = "Delay";
	g->unsetColor = SCScheme.colors.qc.qcNotSet;
	g->setColorAt(0, SCScheme.colors.qc.delay0, "< 20s");
	g->setColorAt(20, SCScheme.colors.qc.delay1, "[20s, 1min)");
	g->setColorAt(60, SCScheme.colors.qc.delay2, "[1min, 3min)");
	g->setColorAt(180, SCScheme.colors.qc.delay3, "[3min, 10min)");
	g->setColorAt(600, SCScheme.colors.qc.delay4, "[10min, 15min)");
	g->setColorAt(1800, SCScheme.colors.qc.delay5, "[15min, 12h)");
	g->setColorAt(43200, SCScheme.colors.qc.delay6, "[12h, 1d)");
	g->setColorAt(86400, SCScheme.colors.qc.delay7, ">= 1d");

	g = &_qcGradients["latency"];
	g->title = "Latency";
	g->unsetColor = SCScheme.colors.qc.qcNotSet;
	g->setColorAt(0, SCScheme.colors.qc.qcOk, "OK < 60s");
	g->setColorAt(60, SCScheme.colors.qc.qcWarning, "Warning >= 60s");

	g = &_qcGradients["timing quality"];
	g->title = "Timing Quality";
	g->unsetColor = SCScheme.colors.qc.qcNotSet;
	g->setColorAt(0, SCScheme.colors.qc.qcWarning, "Warning < 50");
	g->setColorAt(50, SCScheme.colors.qc.qcOk, "OK >= 50");

	g = &_qcGradients["gaps interval"];
	g->title = "Gaps Interval";
	g->setColorAt(0, SCScheme.colors.qc.qcOk, "OK");
	g->setColorAt(1, SCScheme.colors.qc.qcError, "Error");

	g = &_qcGradients["gaps length"];
	g->title = "Gaps Length in s";
	g->setColorAt(0, SCScheme.colors.qc.qcOk, "OK");
	g->setColorAt(1, SCScheme.colors.qc.qcError, "Error");

	g = &_qcGradients["overlaps interval"];
	g->title = "Overlaps interval";
	g->setColorAt(0, SCScheme.colors.qc.qcOk, "OK");
	g->setColorAt(1, SCScheme.colors.qc.qcError, "Error");

	g = &_qcGradients["availability"];
	g->title = "Availability";
	g->setColorAt(0, SCScheme.colors.qc.qcError, "Error");
	g->setColorAt(75, SCScheme.colors.qc.qcWarning, "Warning");
	g->setColorAt(100, SCScheme.colors.qc.qcOk, "OK");

	g = &_qcGradients["offset"];
	g->title = "Offset";
	g->setColorAt(0, SCScheme.colors.qc.qcOk, "OK");
	g->setColorAt(100000, SCScheme.colors.qc.qcWarning, "Warning");

	g = &_qcGradients["rms"];
	g->title = "RMS";
	g->setColorAt(0, SCScheme.colors.qc.qcError, "Error");
	g->setColorAt(10, SCScheme.colors.qc.qcOk, "OK");

	_legend = new NetworkLayerLegend(this);
	_legend->setEnabled(true);
	_legend->setArea(Qt::AlignRight | Qt::AlignTop);
	_legend->setLayer(this);
	addLegend(_legend);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
NetworkLayer::~NetworkLayer() {
	disposeSymbols();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::updateAnnotations() {
	foreach ( NetworkLayerSymbol *s, _stationSymbols ) {
		DataModel::Station *sta = s->model();
		if ( _showChannelCodes && s->data()->channel ) {
			s->setAnnotation(
				(
					sta->network()->code() + "." + sta->code() + "." +
					s->data()->channel->sensorLocation()->code() + "." +
					s->data()->channel->code()
				).c_str()
			);
		}
		else {
			s->setAnnotation(
				(
					sta->network()->code() + "." + sta->code()
				).c_str()
			);
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::disposeSymbols() {
	foreach ( NetworkLayerSymbol *s, _stationSymbols )
		delete s;

	_stationSymbols.clear();
	_stationSymbolLookup.clear();
	_currentSymbol = nullptr;
	_currentClickSymbol = nullptr;

	// Remove link to global array
	for ( auto & [model, data] : global.stationConfig ) {
		data->viewData = nullptr;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::setColorMode(ColorMode mode, bool force) {
	if ( (_colorMode == mode) && !force ) return;

	_colorMode = mode;

	foreach ( NetworkLayerSymbol *s, _stationSymbols ) {
		updateColor(s);
	}

	_legend->updateFrom(this);

	emit updateRequested();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::setInventory(DataModel::Inventory *inv,
                                Gui::Map::Annotations *annotations,
                                const Core::Time *time) {
	disposeSymbols();

	if ( !inv ) return;

	size_t n, s;

	Core::Time refTime;
	if ( time )
		refTime = *time;
	else
		refTime = Core::Time::UTC();

	for ( n = 0; n < inv->networkCount(); ++n ) {
		DataModel::Network *net = inv->network(n);

		for ( s = 0; s < net->stationCount(); ++s ) {
			DataModel::Station *sta = net->station(s);

			if ( refTime < sta->start() )
				continue;

			try {
				if ( sta->end() <= refTime )
					continue;
			}
			catch ( ... ) {}

			double lat, lon;
			try {
				lat = sta->latitude();
				lon = sta->longitude();
			}
			catch ( ... ) {
				continue;
			}

			if ( _stationSymbolLookup.find(sta) != _stationSymbolLookup.end() ) {
				// Symbol with ID already registered
				continue;
			}

			// Got a valid station epoch
			NetworkLayerSymbol *symbol = new NetworkLayerSymbol(this, sta, annotations->add(QString()));
			symbol->setPenWidth(defaultFrameWidth);
			symbol->setLocation(lat, lon);
			updateColor(symbol);

			_stationSymbolLookup[sta] = symbol;
			_stationSymbols.append(symbol);

			// Register symbol with config
			auto it = global.stationConfig.find(symbol->model());
			if ( it != global.stationConfig.end() ) {
				auto data = it->second.get();
				data->viewData = symbol;
			}
		}
	}

	updateAnnotations();

	std::sort(_stationSymbols.begin(), _stationSymbols.end(), topToBottom);
	_legend->updateFrom(this);

	emit updateRequested();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::clear() {
	disposeSymbols();
	emit updateRequested();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::setGMGradient(const NetworkLayerGradient &gradient) {
	_gmGradient = gradient;

	if ( _colorMode == GroundMotion ) {
		setColorMode(GroundMotion, true);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const NetworkLayerGradient *NetworkLayer::qcGradient() const {
	auto it = _qcGradients.find(_activeQCParameter);
	if ( it == _qcGradients.end() )
		return nullptr;
	return &it.value();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::setActiveQCParameter(const std::string &param) {
	_activeQCParameter = param;
	setColorMode(QC, true);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::setStationsVisible(QSet<const DataModel::Station*> *set) {
	if ( set ) {
		foreach ( NetworkLayerSymbol *s, _stationSymbols ) {
			s->setDefaultVisibility();
			if ( !set->contains(s->model()) )
				s->setVisible(false);
		}
	}
	else {
		foreach ( NetworkLayerSymbol *s, _stationSymbols ) {
			s->setDefaultVisibility();
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::setShowChannelCodes(bool enable) {
	if ( _showChannelCodes == enable ) return;
	_showChannelCodes = enable;

	updateAnnotations();

	emit updateRequested();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::setShowIssues(bool enable) {
	if ( _showIssues == enable ) return;
	_showIssues = enable;
	emit updateRequested();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::tick() {
	Core::Time now = Core::Time::UTC();
	bool changed = false;

	foreach ( NetworkLayerSymbol *s, _stationSymbols ) {
		if ( !s->_data ) {
			continue;
		}

		if ( !s->_data->triggerTime ) {
			continue;
		}

		Core::TimeSpan diff = now - *s->_data->triggerTime;

		if ( diff > global.triggerTimeout ) {
			// Reset trigger time if outdated
			s->_data->triggerTime = Core::None;
			s->setFrameSize(0);
			changed = true;
		}
		else if ( diff < Core::TimeSpan(0,0) ) {
			if ( s->frameSize() > 0 ) {
				s->setFrameSize(0);
				changed = true;
			}
		}
		else {
			if ( global.tickToggleState ) {
				// Trigger on
				int alpha = qMax(
					0,
					qMin(
						255,
						static_cast<int>(
							255 * static_cast<double>(global.triggerTimeout - diff) / static_cast<double>(global.triggerTimeout)
						)
					)
				);
				s->setFrameColor(QColor(255,0,0,alpha));
				s->setFrameSize(2);
			}
			else {
				// Trigger off
				s->setFrameSize(0);
			}

			changed = true;
		}
	}

	if ( changed )
		emit updateRequested();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool NetworkLayer::isInside(const QMouseEvent *event, const QPointF &geoPos) {
	int x = event->pos().x();
	int y = event->pos().y();
	auto it = _stationSymbols.end();
	_isInsideSymbol = nullptr;

	while ( it != _stationSymbols.begin() ) {
		--it;
		if ( (*it)->isInside(x, y) ) {
			_isInsideSymbol = *it;
			return true;
		}
	}

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::calculateMapPosition(const Gui::Map::Canvas *canvas) {
	foreach ( NetworkLayerSymbol *s, _stationSymbols )
		s->calculateMapPosition(canvas);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::draw(const Gui::Map::Canvas *canvas, QPainter &p) {
	p.save();

	foreach ( NetworkLayerSymbol *s, _stationSymbols ) {
		if ( s->isClipped() || !s->isVisible() ) {
			continue;
		}

		s->drawShadow(p);
	}

	for ( int i = Gui::Map::Symbol::NONE; i <= Gui::Map::Symbol::HIGH; ++i ) {
		foreach ( NetworkLayerSymbol *s, _stationSymbols ) {
			if ( s->isClipped() || !s->isVisible() || (s->priority() != i) ) {
				continue;
			}

			s->draw(canvas, p);

			if ( _showIssues && (s->state() != Settings::OK) ) {
				int em = qMax(p.fontMetrics().height() - 4, 0) * 0.707106781186;
				QPoint center = s->pos() + QPoint(s->width() + em, -em - s->width() * 3 / 2);

				static OPT(QPixmap) pmQuestion, pmWrench, pmUnlink, pmDatabase;

				switch ( s->state() ) {
					case Settings::Unknown:
						if ( !pmQuestion ) {
							pmQuestion = Gui::icon("question", Qt::black).pixmap(p.fontMetrics().height());
						}
						drawWarningSymbol(p, center, *pmQuestion);
						break;

					case Settings::Unconfigured:
						if ( !pmWrench ) {
							pmWrench = Gui::icon("wrench", Qt::black).pixmap(p.fontMetrics().height());
						}
						drawWarningSymbol(p, center, *pmWrench);
						break;

					case Settings::NoPrimaryStream:
						if ( !pmUnlink ) {
							pmUnlink = Gui::icon("unlink", Qt::black).pixmap(p.fontMetrics().height());
						}
						drawWarningSymbol(p, center, *pmUnlink);
						break;

					case Settings::NoChannelGroupMetaData:
					case Settings::NoVerticalChannelMetaData:
						if ( !pmDatabase ) {
							pmDatabase = Gui::icon("database", Qt::black).pixmap(p.fontMetrics().height());
						}
						drawWarningSymbol(p, center, *pmDatabase);
						break;

					default:
						break;
				}
			}
		}
	}

	p.restore();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::handleLeaveEvent() {
	if ( _currentSymbol ) {
		_currentSymbol->setPen(_currentSymbol->isSelected() ? selectedFrameColor : defaultFrameColor);
		_currentSymbol = nullptr;
		emit stationLeft();
		emit updateRequested();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool NetworkLayer::filterMouseMoveEvent(QMouseEvent *e, const QPointF &) {
	// Hover changed
	if ( _isInsideSymbol != _currentSymbol ) {
		// Just in case we missed the release event
		if ( e->button() != Qt::LeftButton )
			_currentClickSymbol = nullptr;

		if ( _currentSymbol ) _currentSymbol->setPen(_currentSymbol->isSelected() ? selectedFrameColor : defaultFrameColor);
		if ( _isInsideSymbol ) {
			if ( !_currentClickSymbol || (_isInsideSymbol == _currentClickSymbol) ) {
				_isInsideSymbol->setPen(hoverFrameColor);
				emit stationEntered(_isInsideSymbol->model());
			}
		}

		_currentSymbol = _isInsideSymbol;

		if ( _currentSymbol ) {
			DataModel::Station *sta = _currentSymbol->model();
			QString toolTip = QString("%1.%2").arg(sta->network()->code().c_str()).arg(sta->code().c_str());
			if ( !sta->description().empty() ) {
				toolTip += "\n";
				toolTip += sta->description().c_str();
			}
			if ( _currentSymbol->value() >= 0 )
				toolTip += QString("\nValue: %1").arg(_currentSymbol->value());
		}

		emit updateRequested();
	}

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool NetworkLayer::filterMousePressEvent(QMouseEvent *, const QPointF &) {
	_currentClickSymbol = _currentSymbol;
	return _currentClickSymbol != nullptr;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool NetworkLayer::filterMouseReleaseEvent(QMouseEvent *, const QPointF &) {
	if ( _currentClickSymbol ) {
		if ( _currentClickSymbol == _currentSymbol ) {
			_currentClickSymbol = nullptr;
			// This is a click on a symbol
			emit stationClicked(_currentSymbol->model());
		}
		else
			_currentClickSymbol = nullptr;

		return true;
	}

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool NetworkLayer::filterMouseDoubleClickEvent(QMouseEvent *, const QPointF &) {
	/*
	if ( _currentSymbol ) {
		return true;
	}
	*/

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void NetworkLayer::updateColor(NetworkLayerSymbol *symbol) {
	currentGradient = nullptr;

	symbol->setState(Settings::OK);

	switch ( _colorMode ) {
		case Default:
			symbol->setColor(defaultColor);
			break;

		case Network:
		{
			QColor color = _networkColors[symbol->model()->network()->code()];
			QColor baseColor = defaultColor.toHsv();

			if ( !color.isValid() ) {
				// distribute colors in color spectrum evenly with golden ratio
				float h = (_networkColors.size()-1) * goldenRationConjugate;
				if ( h > 1 ) h -= int(h);
				color = QColor::fromHsv((int(360*h)+baseColor.hue())%360, 192, 192);
				_networkColors[symbol->model()->network()->code()] = color;
			}

			symbol->setColor(color);

			Settings::StationConfigs::const_iterator it  = global.stationConfig.find(symbol->model());
			if ( it == global.stationConfig.end() ) {
				symbol->setState(Settings::Unknown);
			}
			else {
				const Settings::StationData *data = it->second.get();
				symbol->setState(data->state);
			}

			break;
		}

		case GroundMotion:
		{
			currentGradient = &_gmGradient;

			auto data = symbol->data();
			symbol->setColorFromValue(data->maximumAmplitude);

			break;
		}

		case QC:
		{
			auto git = _qcGradients.find(_activeQCParameter);
			if ( git != _qcGradients.end() ) {
				currentGradient = &git.value();
			}

			auto data = symbol->data();
			auto dit = data->qc.find(_activeQCParameter);
			if ( dit != data->qc.end() ) {
				symbol->setColorFromValue(dit->second->value());
			}
			else {
				symbol->setColorFromValue(-1);
			}

			break;
		}

		default:
			symbol->setColorFromValue(-1);
			break;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
