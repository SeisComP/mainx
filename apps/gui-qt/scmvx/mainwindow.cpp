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
#include <seiscomp/core/datamessage.h>
#include <seiscomp/gui/core/application.h>
#include <seiscomp/gui/datamodel/eventlayer.h>
#include <seiscomp/io/archive/xmlarchive.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QTreeWidget>

#include "mainwindow.h"
#include "searchwidget.h"
#include "eventinfodialog.h"
#include "stationinfodialog.h"
#include "map/networklayer.h"
#include "map/eventlayer.h"
#include "map/eventheatlayer.h"
#include "map/currenteventlayer.h"
#include "map/scalelayer.h"


using namespace std;
using namespace Seiscomp;

namespace dm = Seiscomp::DataModel;


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace Seiscomp {
namespace MapViewX {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::Event::setEvent(DataModel::Event *evt,
                                 ObjectCache &cache) {
	event = evt;
	preferredOrigin = cache.get<dm::Origin>(evt->preferredOriginID());
	preferredMagnitude = cache.get<dm::Magnitude>(evt->preferredMagnitudeID());
	preferredFocalMechanism = cache.get<dm::FocalMechanism>(evt->preferredFocalMechanismID());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool MainWindow::Event::isMoreRecent(const Event &other) const {
	if ( !isValid() ) return false;
	if ( !other ) return true;
	return preferredOrigin->time().value() > other.preferredOrigin->time().value();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::Event::reset() {
	event.reset();
	preferredOrigin.reset();
	preferredMagnitude.reset();
	preferredFocalMechanism.reset();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
QPointF MainWindow::Event::location() const {
	if ( preferredOrigin )
		return QPointF(preferredOrigin->longitude(), preferredOrigin->latitude());
	else
		return QPointF();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags f)
: Gui::MainWindow(parent, f)
, _currentTraces(nullptr) {
	setWindowIcon(QIcon(QPixmap(":/gempa/gui/icons/gempalogo.png")));
	SCScheme.colors.legend.background.setAlpha(192);

	qRegisterMetaType<string>("std::string");

	_ui.setupUi(this);
	_ui.actionCenterMapOnEventUpdate->setChecked(global.centerOrigins);

	_cache.setDatabaseArchive(SCApp->query());
	_cache.setBufferSize(100);

	connect(SCApp, SIGNAL(messageAvailable(Seiscomp::Core::Message*,Seiscomp::Client::Packet*)),
	        this, SLOT(handleMessage(Seiscomp::Core::Message*)));
	connect(SCApp, SIGNAL(addObject(QString,Seiscomp::DataModel::Object*)),
	        this, SLOT(addObject(QString,Seiscomp::DataModel::Object*)));

	_actionToggleFullScreen->disconnect();
	connect(_actionToggleFullScreen, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));

	connect(_ui.actionOpenFile, SIGNAL(triggered()), this, SLOT(openFile()));
	connect(_ui.actionQuit, SIGNAL(triggered()), this, SLOT(close()));
	connect(_ui.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(switchTab(int)));

	_mapWidget = new Gui::MapWidget(SCApp->mapsDesc());
	_mapWidget->canvas().setLegendMargin(9);

	_eventListView = new Gui::EventListView(SCApp->query(), false);
	connect(SCApp, SIGNAL(messageAvailable(Seiscomp::Core::Message*,Seiscomp::Client::Packet*)),
	        _eventListView, SLOT(messageAvailable(Seiscomp::Core::Message*,Seiscomp::Client::Packet*)));

	connect(SCApp, SIGNAL(notifierAvailable(Seiscomp::DataModel::Notifier*)),
	        _eventListView, SLOT(notifierAvailable(Seiscomp::DataModel::Notifier*)));

	connect(_eventListView, SIGNAL(eventSelected(Seiscomp::DataModel::Event*)),
	        this, SLOT(selectEventFromList(Seiscomp::DataModel::Event*)));

	_ui.tabNetwork->setLayout(new QVBoxLayout);
	_ui.tabGM->setLayout(new QVBoxLayout);
	_ui.tabQC->setLayout(new QVBoxLayout);
	_ui.tabEvents->setLayout(new QVBoxLayout);

	_ui.tabEvents->layout()->addWidget(_eventListView);

	_ui.actionShowLatestEvent->setChecked(global.showLatestEvent);

	_currentEventLayer = new CurrentEventLayer(_mapWidget);
	_currentEventLayer->setVisible(_ui.actionShowLatestEvent->isChecked());

	connect(_ui.actionShowGrayscale, SIGNAL(toggled(bool)), _mapWidget, SLOT(setGrayScale(bool)));
	connect(_ui.actionShowLatestEvent, SIGNAL(toggled(bool)), _currentEventLayer, SLOT(setVisible(bool)));
	connect(_ui.actionShowMapLegend, SIGNAL(toggled(bool)), _mapWidget, SLOT(setDrawLegends(bool)));
	connect(&_mapWidget->canvas(), SIGNAL(legendVisibilityChanged(bool)), _ui.actionShowMapLegend, SLOT(setChecked(bool)));
	_mapWidget->addAction(_actionToggleFullScreen);

	_eventHeatLayer = new EventHeatLayer(_mapWidget);
	_eventHeatLayer->setVisible(false);

	connect(_eventListView, SIGNAL(reset()), _eventHeatLayer, SLOT(clear()));
	connect(_eventListView, SIGNAL(eventAddedToList(Seiscomp::DataModel::Event*,bool)),
	        _eventHeatLayer, SLOT(addEvent(Seiscomp::DataModel::Event*,bool)));
	connect(_eventListView, SIGNAL(eventUpdatedInList(Seiscomp::DataModel::Event*)),
	        _eventHeatLayer, SLOT(updateEvent(Seiscomp::DataModel::Event*)));
	connect(_eventListView, SIGNAL(eventRemovedFromList(Seiscomp::DataModel::Event*)),
	        _eventHeatLayer, SLOT(removeEvent(Seiscomp::DataModel::Event*)));

	_eventLayer = new EventLayer(_mapWidget, &_cache);
	_eventLayer->setVisible(true);

	connect(_eventListView, SIGNAL(reset()), _eventLayer, SLOT(clear()));
	connect(_eventListView, SIGNAL(eventAddedToList(Seiscomp::DataModel::Event*,bool)),
	        _eventLayer, SLOT(addEvent(Seiscomp::DataModel::Event*,bool)));
	connect(_eventListView, SIGNAL(eventUpdatedInList(Seiscomp::DataModel::Event*)),
	        _eventLayer, SLOT(updateEvent(Seiscomp::DataModel::Event*)));
	connect(_eventListView, SIGNAL(eventRemovedFromList(Seiscomp::DataModel::Event*)),
	        _eventLayer, SLOT(removeEvent(Seiscomp::DataModel::Event*)));
	connect(_eventLayer, SIGNAL(eventHovered(std::string)),
	        this, SLOT(hoverEvent(std::string)));
	connect(_eventLayer, SIGNAL(eventSelected(std::string)),
	        this, SLOT(selectEvent(std::string)),
	        Qt::QueuedConnection);

	connect(_eventListView, SIGNAL(visibleEventCountChanged()),
	        this, SLOT(updateEventTabText()));

	_annotationLayer = new Gui::Map::AnnotationLayer(_mapWidget, new Gui::Map::Annotations);
	_annotationLayer->setVisible(_ui.actionShowStationAnnotations->isChecked());

	_stationLayer = new NetworkLayer(_mapWidget);
	_stationLayer->setInventory(Client::Inventory::Instance()->inventory(), _annotationLayer->annotations());
	_stationLayer->setShowChannelCodes(_ui.actionShowChannelCodes->isChecked());
	_stationLayer->setShowIssues(_ui.actionShowStationIssues->isChecked());

	connect(_stationLayer, SIGNAL(stationEntered(Seiscomp::DataModel::Station*)),
	        this, SLOT(stationEntered(Seiscomp::DataModel::Station*)));
	connect(_stationLayer, SIGNAL(stationLeft()), this, SLOT(stationLeft()));
	connect(_stationLayer, SIGNAL(stationClicked(Seiscomp::DataModel::Station*)),
	        this, SLOT(stationClicked(Seiscomp::DataModel::Station*)));

	connect(_ui.actionShowStationAnnotations, SIGNAL(toggled(bool)), _annotationLayer, SLOT(setVisible(bool)));
	connect(_ui.actionShowChannelCodes, SIGNAL(toggled(bool)), _stationLayer, SLOT(setShowChannelCodes(bool)));
	connect(_ui.actionShowStationIssues, SIGNAL(toggled(bool)), _stationLayer, SLOT(setShowIssues(bool)));
	connect(_ui.actionSearchStation, SIGNAL(triggered()), this, SLOT(searchStation()));
	connect(_ui.actionCenterMapOnEventUpdate, SIGNAL(toggled(bool)), this, SLOT(toggleCentering(bool)));
	connect(_ui.actionResetView, SIGNAL(triggered()), this, SLOT(resetView()));

	{
		QActionGroup *qcActions = new QActionGroup(_ui.menuQC);
		QAction *action;

		action = _ui.menuQC->addAction(tr("Delay")); action->setData("delay");
		action->setCheckable(true);
		action->setActionGroup(qcActions);
		action->setChecked(action->data().toString() == _stationLayer->activeQCParameter().c_str());

		action = _ui.menuQC->addAction(tr("Latency"));
		action->setData("latency"); action->setCheckable(true);
		action->setActionGroup(qcActions);
		action->setChecked(action->data().toString() == _stationLayer->activeQCParameter().c_str());

		action = _ui.menuQC->addAction(tr("Timing Quality"));
		action->setData("timing quality");
		action->setCheckable(true);
		action->setActionGroup(qcActions);
		action->setChecked(action->data().toString() == _stationLayer->activeQCParameter().c_str());

		action = _ui.menuQC->addAction(tr("Gaps Interval"));
		action->setData("gaps interval");
		action->setCheckable(true);
		action->setActionGroup(qcActions);
		action->setChecked(action->data().toString() == _stationLayer->activeQCParameter().c_str());

		action = _ui.menuQC->addAction(tr("Gaps Length"));
		action->setData("gaps length");
		action->setCheckable(true);
		action->setActionGroup(qcActions);
		action->setChecked(action->data().toString() == _stationLayer->activeQCParameter().c_str());

		action = _ui.menuQC->addAction(tr("Overlaps Interval"));
		action->setData("overlaps interval");
		action->setCheckable(true);
		action->setActionGroup(qcActions);
		action->setChecked(action->data().toString() == _stationLayer->activeQCParameter().c_str());

		action = _ui.menuQC->addAction(tr("Availability"));
		action->setData("availability");
		action->setCheckable(true);
		action->setActionGroup(qcActions);
		action->setChecked(action->data().toString() == _stationLayer->activeQCParameter().c_str());

		action = _ui.menuQC->addAction(tr("Offset"));
		action->setData("offset");
		action->setCheckable(true);
		action->setActionGroup(qcActions);
		action->setChecked(action->data().toString() == _stationLayer->activeQCParameter().c_str());

		action = _ui.menuQC->addAction(tr("RMS"));
		action->setData("rms");
		action->setCheckable(true);
		action->setActionGroup(qcActions);
		action->setChecked(action->data().toString() == _stationLayer->activeQCParameter().c_str());
	}

	connect(_ui.menuQC, SIGNAL(triggered(QAction*)), this, SLOT(applyQCMode(QAction*)));

	_mapWidget->canvas().addLayer(_eventHeatLayer);
	_mapWidget->canvas().addLayer(_stationLayer);
	_mapWidget->canvas().addLayer(_eventLayer);
	_mapWidget->canvas().addLayer(_currentEventLayer);
	_mapWidget->canvas().addLayer(_annotationLayer);
	_mapWidget->canvas().addLayer(new ScaleLayer);

	_ui.menuSettings->addAction(this->_actionShowSettings);
	_ui.menuView->insertAction(_ui.actionShowChannelCodes, this->_actionToggleFullScreen);

	_mapDirty = false;

	connect(_eventListView, SIGNAL(eventAddedToList(Seiscomp::DataModel::Event*,bool)),
	        this, SLOT(eventAdded(Seiscomp::DataModel::Event*,bool)));
	connect(_eventListView, SIGNAL(eventUpdatedInList(Seiscomp::DataModel::Event*)),
	        this, SLOT(eventUpdated(Seiscomp::DataModel::Event*)));
	connect(_eventListView, SIGNAL(eventRemovedFromList(Seiscomp::DataModel::Event*)),
	        this, SLOT(eventRemoved(Seiscomp::DataModel::Event*)));
	connect(_eventListView, SIGNAL(reset()), this, SLOT(eventsCleared()));
	connect(_eventListView, SIGNAL(eventsUpdated()), this, SLOT(eventsUpdated()));

	switchTab(_ui.tabWidget->currentIndex());

	connect(&_updateTimer, SIGNAL(timeout()), this, SLOT(timeout()));

	connect(_ui.actionOpenEventTable, SIGNAL(triggered()), this, SLOT(showEventList()));

	_updateTimer.setInterval(1000);
	_updateTimer.start();

	_eventHeatLayer->setCompositionMode(true);

	Core::TimeWindow tw;
	tw.setEndTime(Core::Time::UTC());
	tw.setStartTime(tw.endTime() - global.eventTimeSpan);
	_eventListView->setInterval(tw);
	if ( !global.inputFile.empty() ) {
		readEventParameters(global.inputFile);
	}
	else {
		_eventListView->readFromDatabase();
	}
	updateEventTabText();
	resetView();

	if ( SCApp->query() ) {
		cerr << "= Total read events from database =" << endl;
		cerr << " * Number of events: " << _eventListView->eventCount() << endl;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::toggleFullScreen() {
	if ( _mapWidget->isFullScreen() )
		leaveFullScreen();
	else {
		if ( _mapWidget->parentWidget() )
			_mapWidget->parentWidget()->layout()->removeWidget(_mapWidget);

		_mapWidget->setWindowFlags(Qt::Window);
		_mapWidget->showFullScreen();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::leaveFullScreen() {
	_mapWidget->hide();
	_mapWidget->showNormal();
	_mapWidget->setWindowFlags(_mapWidget->windowFlags() & ~Qt::Window);
	switchTab(_ui.tabWidget->currentIndex());
	_mapWidget->show();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::resetView() {
	if ( !global.initialRegion.isNull() )
		_mapWidget->canvas().displayRect(global.initialRegion);
	else
		_mapWidget->canvas().displayRect(QRectF(-180, -90, 360, 180));

	if ( global.displayMode == "groundmotion" ) {
		_ui.tabWidget->setCurrentWidget(_ui.tabGM);
	}
	else if ( global.displayMode == "qualitycontrol" )
		_ui.tabWidget->setCurrentWidget(_ui.tabQC);
	else
		_ui.tabWidget->setCurrentWidget(_ui.tabNetwork);

	bool showLegend = SCScheme.map.showLegends;

	if ( global.showLegend ) {
		showLegend = true;
	}

	_ui.actionShowMapLegend->setChecked(showLegend);
	_mapWidget->setDrawLegends(_ui.actionShowMapLegend->isChecked());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::showEventList() {
	_ui.tabWidget->setCurrentWidget(_ui.tabEvents);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::switchTab(int index) {
	QWidget *w = _ui.tabWidget->widget(index);
	if ( w == _ui.tabGM || w == _ui.tabQC || w == _ui.tabNetwork ) {
		if ( !_mapWidget->isFullScreen() ) {
			// Reparent _mapWidget
			if ( _mapWidget->parentWidget() ) {
				_mapWidget->parentWidget()->layout()->removeWidget(_mapWidget);
			}
			w->layout()->addWidget(_mapWidget);
		}

		if ( w == _ui.tabNetwork )
			_stationLayer->setColorMode(NetworkLayer::Network);
		else if ( w == _ui.tabGM )
			_stationLayer->setColorMode(NetworkLayer::GroundMotion);
		else if ( w == _ui.tabQC )
			_stationLayer->setColorMode(NetworkLayer::QC);
		else
			_stationLayer->setColorMode(NetworkLayer::Default);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::timeout() {
	global.tickToggleState = !global.tickToggleState;

	if ( _mapDirty ) {
		_mapWidget->update();
		_mapDirty = false;
	}

	for ( auto & [model, data] : global.stationConfig ) {
		if ( data->infoData ) {
			static_cast<StationInfoDialog*>(data->infoData)->shiftData();
		}
	}

	_stationLayer->tick();
	_eventLayer->tick();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::openFile() {
	QString filename = QFileDialog::getOpenFileName(this,
		tr("Open XML file"), "", tr("XML files (*.xml);;All (*.*)"));

	if ( filename.isEmpty() ) {
		return;
	}

	readEventParameters(filename.toStdString());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::handleMessage(Core::Message* message) {
	auto dataMessage = Core::DataMessage::Cast(message);
	if ( dataMessage ) {
		for ( auto &obj : *dataMessage ) {
			DataModel::WaveformQuality *wfq = DataModel::WaveformQuality::Cast(obj);
			if ( wfq ) {
				addObject(QString(), wfq);
			}
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::addObject(const QString &, DataModel::Object *obj) {
	dm::Pick *pick = dm::Pick::Cast(obj);
	if ( pick ) {
		SEISCOMP_INFO("Received pick at %s on %s.%s.%s.%s",
		              pick->time().value().iso().c_str(),
		              pick->waveformID().networkCode().c_str(),
		              pick->waveformID().stationCode().c_str(),
		              pick->waveformID().locationCode().c_str(),
		              pick->waveformID().channelCode().c_str());
		string sta_id = pick->waveformID().networkCode() + "." + pick->waveformID().stationCode();
		auto it = global.stationIDConfig.find(sta_id);
		if ( it == global.stationIDConfig.end() ) {
			SEISCOMP_DEBUG("Station not registered, ignoring pick");
			return;
		}

		if ( !it->second->triggerTime ||
		     (pick->time().value() > *it->second->triggerTime) ) {
			it->second->triggerTime = pick->time().value();
		}

		return;
	}

	dm::WaveformQuality *wfq = dm::WaveformQuality::Cast(obj);
	if ( wfq ) {
		string sta_id = wfq->waveformID().networkCode() + "." + wfq->waveformID().stationCode();
		auto it = global.stationIDConfig.find(sta_id);
		if ( it == global.stationIDConfig.end() ) {
			SEISCOMP_DEBUG("%s: station not registered, ignoring waveform quality",
			               sta_id.c_str());
			return;
		}

		if ( !it->second->channel ) {
			SEISCOMP_DEBUG("%s.%s.%s: station channel not configured, ignoring waveform quality",
			               sta_id.c_str(),
			               wfq->waveformID().locationCode().c_str(),
			               wfq->waveformID().channelCode().c_str());
			return;
		}

		if ( wfq->waveformID().locationCode() != it->second->channel->sensorLocation()->code()
		  || wfq->waveformID().channelCode() != it->second->channel->code() ) {
			SEISCOMP_DEBUG("%s.%s.%s: channel not preferred, ignoring waveform quality",
			               sta_id.c_str(),
			               wfq->waveformID().locationCode().c_str(),
			               wfq->waveformID().channelCode().c_str());
			return;
		}

		it->second->qc[wfq->parameter()] = wfq;
		updateQC(it->second.get(), wfq);

		return;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::eventAdded(DataModel::Event *evtObj, bool) {
	try {
		if ( SCApp->isAgencyIDBlocked(evtObj->creationInfo().agencyID()) )
			return;
	}
	catch ( ... ) {}

	if ( _latestEvent ) {
		Event evt;
		evt.setEvent(evtObj, _cache);
		if ( _latestEvent.isMoreRecent(evt) )
			return;
		_latestEvent = evt;
	}
	else
		_latestEvent.setEvent(evtObj, _cache);

	updateCurrentEvent();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::eventUpdated(DataModel::Event *evt) {
	eventAdded(evt, true);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::eventRemoved(DataModel::Event *evt) {
	if ( _latestEvent && _latestEvent.event->publicID() == evt->publicID() ) {
		_latestEvent.reset();
		updateCurrentEvent();
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::eventsUpdated() {
	if ( _latestEvent ) return;

	// Check for a new latest event after the event list finished to update
	// its events

	QTreeWidget *eventTree = _eventListView->eventTree();
	int count = eventTree->topLevelItemCount();

	for ( int i = 0; i < count; ++i ) {
		if ( eventTree->topLevelItem(i)->isHidden() ) continue;
		DataModel::Event *evt = _eventListView->eventFromTreeItem(eventTree->topLevelItem(i));

		try {
			if ( SCApp->isAgencyIDBlocked(evt->creationInfo().agencyID()) )
				continue;
		}
		catch ( ... ) {}

		if ( _latestEvent ) {
			Event evtObj;
			evtObj.setEvent(evt, _cache);
			if ( _latestEvent.isMoreRecent(evtObj) ) continue;
			_latestEvent = evtObj;
		}
		else
			_latestEvent.setEvent(evt, _cache);
	}

	updateCurrentEvent();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::eventsCleared() {
	_latestEvent.reset();
	updateCurrentEvent();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::updateCurrentEvent() {
	if ( _latestEvent ) {
		_eventLayer->setCurrentEvent(_latestEvent.event.get());
		_currentEventLayer->setEvent(_latestEvent.event.get());
		if ( global.centerOrigins )
			_mapWidget->canvas().setMapCenter(_latestEvent.location());
	}
	else {
		_eventLayer->setCurrentEvent(nullptr);
		_currentEventLayer->setEvent(nullptr);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::hoverEvent(const std::string &eventID) {
	setToolTip(eventID.c_str());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::selectEvent(const std::string &eventID) {
	dm::EventPtr event = _cache.get<dm::Event>(eventID);
	if ( event ) {
		if ( !_eventDetails ) {
			_eventDetails = new EventInfoDialog(this, Qt::Tool);
			connect(_eventDetails, SIGNAL(destroyed(QObject*)),
			        this, SLOT(objectDestroyed(QObject*)));
			_eventDetails->setAttribute(Qt::WA_DeleteOnClose);
			_eventDetails->restoreGeometry(_eventDetailsState);
		}

		_eventDetails->setEvent(event.get());
		_eventDetails->show();
		_eventDetails->activateWindow();
	}
	else {
		QMessageBox::critical(
			nullptr, tr("Error"),
			tr("Event %1 could not be found").arg(eventID.c_str())
		);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::selectEventFromList(Seiscomp::DataModel::Event *evt) {
	dm::OriginPtr org = _cache.get<dm::Origin>(evt->preferredOriginID());
	if ( org ) {
		_mapWidget->canvas().setMapCenter(
			QPointF(
				org->longitude().value(),
				org->latitude().value()
			)
		);
	}
	_ui.tabWidget->setCurrentWidget(_ui.tabNetwork);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::stationEntered(DataModel::Station *station) {
	if ( statusBar() )
		statusBar()->showMessage((station->network()->code() + "." + station->code()).c_str());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::stationLeft() {
	if ( statusBar() )
		statusBar()->clearMessage();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::stationClicked(DataModel::Station *station) {
	Settings::StationData *data = nullptr;

	auto it = global.stationConfig.find(station);
	if ( it != global.stationConfig.end() ) {
		data = it->second.get();
	}

	StationInfoDialog dlg(station, data);

	data->infoData = &dlg;
	dlg.restoreGeometry(_lastInfoGeometry);
	dlg.exec();
	_lastInfoGeometry = dlg.saveGeometry();
	data->infoData = nullptr;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::readEventParameters(const std::string &file) {
	QApplication::setOverrideCursor(Qt::WaitCursor);

	IO::XMLArchive ar;
	if ( !ar.open(file.c_str()) ) {
		QApplication::restoreOverrideCursor();
		QMessageBox::critical(this, tr("Read EventParameters"),
			QString("Could not open file\n%1").arg(file.c_str())
		);
		return;
	}

	_eventListView->clear();
	_localEP = nullptr;
	ar >> _localEP;

	if ( !_localEP ) {
		QApplication::restoreOverrideCursor();
		QMessageBox::critical(this, tr("Read EventParameters"),
			QString("Invalid file\n%1").arg(file.c_str())
		);
		return;
	}

	for ( size_t i = 0; i < _localEP->eventCount(); ++i ) {
		_eventListView->add(_localEP->event(i), nullptr);
	}

	cerr << "= Total read events from file =" << endl;
	cerr << " * Number of events: " << _eventLayer->eventCount() << endl;

	QApplication::restoreOverrideCursor();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::updateQC(Settings::StationData *data,
                          DataModel::WaveformQuality *wfq) {
	if ( _stationLayer->colorMode() != NetworkLayer::QC )
		return;

	auto symbol = reinterpret_cast<NetworkLayerSymbol*>(data->viewData);
	if ( !symbol ) return;

	if ( wfq->parameter() == _stationLayer->activeQCParameter() ) {
		symbol->setColorFromValue(wfq->value());
		_mapDirty = true;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::updateGroundMotion(Settings::StationData *data) {
	auto symbol = reinterpret_cast<NetworkLayerSymbol*>(data->viewData);
	if ( !symbol ) {
		return;
	}

	if ( _stationLayer->colorMode() == NetworkLayer::GroundMotion ) {
		symbol->setColorFromValue(data->maximumAmplitude);
		_mapDirty = true;
	}

	if ( data->infoData ) {
		static_cast<StationInfoDialog*>(data->infoData)->processingDataUpdated(data);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::updateEventTabText() {
	int idx = _ui.tabWidget->indexOf(_ui.tabEvents);
	_ui.tabWidget->setTabText(idx, QString("Events (%1/%2)")
	                          .arg(_eventListView->visibleEventCount())
	                          .arg(_eventListView->eventCount()));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::applyQCMode(QAction *action) {
	_stationLayer->setActiveQCParameter(action->data().toString().toStdString());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::searchStation() {
	if ( !_currentSearch ) {
		_currentSearch = new SearchWidget(this);
		connect(_currentSearch, SIGNAL(destroyed(QObject*)),
		        this, SLOT(objectDestroyed(QObject*)));
		connect(_currentSearch, SIGNAL(filterView()),
		        this, SLOT(filterStations()));
	}

	_currentSearch->show();
	_currentSearch->activateWindow();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::filterStations() {
	if ( _currentSearch ) {
		auto result = _currentSearch->visibleData();
		_stationLayer->setStationsVisible(&result);

		if ( !result.isEmpty() ) {
			_mapWidget->canvas().setMapCenter(
				QPointF(
					(*result.begin())->longitude(),
					(*result.begin())->latitude()
				)
			);
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::objectDestroyed(QObject *obj) {
	if ( _currentSearch == obj ) {
		_currentSearch = nullptr;
		_stationLayer->setStationsVisible(nullptr);
	}
	else if ( _eventDetails == obj ) {
		_eventDetailsState = _eventDetails->saveGeometry();
		_eventDetails = nullptr;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MainWindow::toggleCentering(bool enable) {
	global.centerOrigins = enable;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
