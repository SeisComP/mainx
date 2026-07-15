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


#ifndef SEISCOMP_MAPVIEWX_MAPWIDGET_H
#define SEISCOMP_MAPVIEWX_MAPWIDGET_H


#ifndef Q_MOC_RUN
#include <seiscomp/gui/map/canvas.h>
#include <seiscomp/gui/map/mapwidget.h>
#endif

#include <QAction>
#include <QContextMenuEvent>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QScreen>
#include <QScrollArea>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>

#include <functional>

#include "map/eventlayer.h"
#include "map/networklayer.h"


namespace Seiscomp::MapViewX {


/**
 * @brief A QMenu that stays open when a checkable/leaf action is clicked, so
 *        several entries can be toggled without reopening it.
 */
class StayOpenMenu : public QMenu {
	public:
		using QMenu::QMenu;

	protected:
		void mouseReleaseEvent(QMouseEvent *event) override {
			QAction *action = activeAction();
			if ( action && action->isEnabled() && !action->menu()
			  && !action->isSeparator() ) {
				action->trigger();
				return;
			}
			QMenu::mouseReleaseEvent(event);
		}
};


/**
 * @brief MapWidget adds the overlapping-symbol chooser and a pluggable
 *        context-menu extender (populated by MainWindow with the station-issue
 *        and network-selection submenus).
 */
class MapWidget : public Gui::MapWidget {
	public:
		explicit MapWidget(const Gui::MapsDesc &desc, QWidget *parent = nullptr)
		: Gui::MapWidget(desc, parent) {}

		void setNetworkLayer(NetworkLayer *layer) { _networkLayer = layer; }
		void setEventLayer(EventLayer *layer) { _eventLayer = layer; }

		//! Sets a callback used to extend the map context menu (MainWindow
		//! injects the station-issue and network-selection submenus).
		void setContextMenuExtender(std::function<void(QMenu*)> fn) {
			_extendContextMenu = fn;
		}

	protected:
		void contextMenuEvent(QContextMenuEvent *event) override {
			if ( canvas().filterContextMenuEvent(event, this) ) {
				return;
			}

			QMenu menu(this);
			updateContextMenu(&menu);

			if ( _extendContextMenu ) {
				_extendContextMenu(&menu);
			}

			executeContextMenuAction(menu.exec(event->globalPos()));
		}

		void mouseReleaseEvent(QMouseEvent *event) override {
			QVector<NetworkLayerSymbol*> stations;
			std::vector<std::string> events;

			// Only resolve station/event symbols when one of those layers is
			// actually under the cursor; otherwise another layer (e.g. the
			// latest-event box) should handle the click.
			auto *hover = canvas().hoverLayer();
			if ( event->button() == Qt::LeftButton && _networkLayer && _eventLayer
			  && (hover == _networkLayer || hover == _eventLayer) ) {
				stations = _networkLayer->symbolsUnder(event->pos().x(), event->pos().y());
				events = _eventLayer->eventsUnder(event->pos().x(), event->pos().y());
			}

			bool chooser = (int(stations.size()) + int(events.size())) > 1;
			if ( chooser ) {
				// Let the base finish its drag/zoom bookkeeping but keep the
				// individual layers from acting on the click; the chooser below
				// decides what to open.
				_networkLayer->setClickSuppressed(true);
				_eventLayer->setClickSuppressed(true);
			}

			Gui::MapWidget::mouseReleaseEvent(event);

			if ( chooser ) {
				_networkLayer->setClickSuppressed(false);
				_eventLayer->setClickSuppressed(false);
				showSymbolChooser(stations, events, QCursor::pos());
			}
		}

	private:
		//! Shows a chooser listing the stations and/or events under the cursor
		//! (stations left, events right). Each present kind gets a header
		//! column; the map-highlighted entry is shown bold, and the list
		//! scrolls when it exceeds ~20 rows or the screen height.
		void showSymbolChooser(const QVector<NetworkLayerSymbol*> &stations,
		                       const std::vector<std::string> &events,
		                       const QPoint &globalPos) {
			QMenu menu(this);

			QWidget *content = new QWidget(&menu);
			QVBoxLayout *contentLayout = new QVBoxLayout(content);
			contentLayout->setContentsMargins(6, 4, 6, 4);
			contentLayout->setSpacing(2);

			int stationCol = -1;
			int eventCol = -1;
			int columns = 0;
			if ( !stations.isEmpty() ) {
				stationCol = columns++;
			}
			if ( !events.empty() ) {
				eventCol = columns++;
			}

			// Pinned header, kept outside the scroll area.
			QWidget *header = new QWidget(content);
			QGridLayout *headerLayout = new QGridLayout(header);
			headerLayout->setContentsMargins(0, 0, 0, 0);
			headerLayout->setHorizontalSpacing(16);
			if ( stationCol >= 0 ) {
				headerLayout->setColumnStretch(stationCol, 1);
				QLabel *h = new QLabel(tr("stations"), header);
				h->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
				headerLayout->addWidget(h, 0, stationCol);
			}
			if ( eventCol >= 0 ) {
				headerLayout->setColumnStretch(eventCol, 1);
				QLabel *h = new QLabel(tr("events"), header);
				h->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
				headerLayout->addWidget(h, 0, eventCol);
			}
			contentLayout->addWidget(header);

			QFrame *separator = new QFrame(content);
			separator->setFrameShape(QFrame::HLine);
			separator->setFrameShadow(QFrame::Sunken);
			contentLayout->addWidget(separator);

			// Scrollable entries. Equal column stretch keeps them aligned with
			// the pinned header above.
			QWidget *grid = new QWidget;
			QGridLayout *layout = new QGridLayout(grid);
			layout->setContentsMargins(0, 0, 0, 0);
			layout->setHorizontalSpacing(16);
			layout->setVerticalSpacing(2);
			if ( stationCol >= 0 ) {
				layout->setColumnStretch(stationCol, 1);
			}
			if ( eventCol >= 0 ) {
				layout->setColumnStretch(eventCol, 1);
			}

			QFont boldFont = font();
			boldFont.setBold(true);

			NetworkLayerSymbol *chosenStation = nullptr;
			std::string chosenEvent;

			int row = 0;
			for ( NetworkLayerSymbol *s : stations ) {
				DataModel::Station *sta = s->model();
				QToolButton *button = new QToolButton(grid);
				button->setAutoRaise(true);
				button->setText(QString("%1.%2").arg(sta->network()->code().c_str(),
				                                      sta->code().c_str()));
				if ( s == _networkLayer->currentSymbol()
				  || s == _networkLayer->selectedSymbol() ) {
					button->setFont(boldFont);
				}
				connect(button, &QToolButton::clicked, &menu, [&menu, &chosenStation, s]() {
					chosenStation = s;
					menu.close();
				});
				layout->addWidget(button, row++, stationCol);
			}

			row = 0;
			for ( const std::string &id : events ) {
				QToolButton *button = new QToolButton(grid);
				button->setAutoRaise(true);
				button->setText(id.c_str());
				if ( id == _eventLayer->hoveredId() ) {
					button->setFont(boldFont);
				}
				connect(button, &QToolButton::clicked, &menu, [&menu, &chosenEvent, id]() {
					chosenEvent = id;
					menu.close();
				});
				layout->addWidget(button, row++, eventCol);
			}

			QScrollArea *scroll = new QScrollArea(content);
			scroll->setWidgetResizable(true);
			scroll->setFrameShape(QFrame::NoFrame);
			scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			scroll->setWidget(grid);

			// Reserve room for the vertical scrollbar so the station codes and
			// event IDs are not truncated once the list scrolls.
			scroll->setMinimumWidth(grid->sizeHint().width() +
			                        style()->pixelMetric(QStyle::PM_ScrollBarExtent));

			// Cap the entries at ~20 rows or the available screen height,
			// scrolling beyond that.
			int entryRows = qMax(int(stations.size()), int(events.size()));
			int fullHeight = grid->sizeHint().height();
			int maxHeight = fullHeight;
			if ( entryRows > 20 ) {
				maxHeight = fullHeight * 20 / entryRows;
			}
			int screenHeight = QGuiApplication::primaryScreen()->availableGeometry().height();
			maxHeight = qMin(maxHeight, screenHeight - 160);
			scroll->setMaximumHeight(maxHeight);
			contentLayout->addWidget(scroll);

			QWidgetAction *gridAction = new QWidgetAction(&menu);
			gridAction->setDefaultWidget(content);
			menu.addAction(gridAction);

			menu.exec(globalPos);

			if ( chosenStation ) {
				_networkLayer->selectStation(chosenStation);
			}
			else if ( !chosenEvent.empty() ) {
				_eventLayer->selectEvent(chosenEvent);
			}
		}

		NetworkLayer *_networkLayer{nullptr};
		EventLayer   *_eventLayer{nullptr};
		std::function<void(QMenu*)> _extendContextMenu;
};


}


#endif
