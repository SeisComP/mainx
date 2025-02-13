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
#include <seiscomp/system/pluginregistry.h>
#include <QMessageBox>

#include "app.h"


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace Seiscomp {
namespace MapViewX {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Application::Application(int &argc, char **argv)
: Gui::Kicker<MainWindow>(argc, argv, DEFAULT | LOAD_STATIONS | LOAD_CONFIGMODULE)
, _recordStreamThread(nullptr)
, _mainWindow(nullptr) {
	setLoadRegionsEnabled(true);
	addMessagingSubscription("PICK");
	addMessagingSubscription("AMPLITUDE");
	addMessagingSubscription("LOCATION");
	addMessagingSubscription("MAGNITUDE");
	addMessagingSubscription("EVENT");
	addMessagingSubscription("QC");

	bindSettings(&global);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Application::~Application() {
	if ( _recordStreamThread ) {
		_recordStreamThread->stop(true);
		delete _recordStreamThread;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Application::initPlugins() {
	System::PluginRegistry::Instance()->addPackagePath("mapview");
	return Gui::Kicker<MainWindow>::initPlugins();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Application::validateParameters() {
	if ( !Gui::Kicker<MainWindow>::validateParameters() ) {
		return false;
	}

	if ( !global.inputFile.empty() ) {
		global.offline = true;
		if ( !isInventoryDatabaseEnabled() && !isConfigDatabaseEnabled() ) {
			setDatabaseEnabled(false, false);
		}
	}

	if ( global.offline ) {
		setMessagingEnabled(false);
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool Application::init() {
	if ( !Gui::Application::init() )
		return false;

	if ( !global.displayMode.empty()
	   && global.displayMode != "groundmotion"
	   && global.displayMode != "qualitycontrol" ) {
		SEISCOMP_ERROR("Invalid displaymode: %s", global.displayMode.c_str());
		return false;
	}

	try {
		auto scale = configGetString("groundMotionScale");
		_gmScale = GroundMotionScaleFactory::Create(scale);
		if ( !_gmScale ) {
			SEISCOMP_ERROR("Invalid ground motion scale: %s", scale);
			SEISCOMP_DEBUG(" adasdsd");
			auto services = GroundMotionScaleFactory::Services();
			SEISCOMP_DEBUG(" %p", static_cast<void*>(services));
			/*
			for ( auto &s : *services ) {
				SEISCOMP_DEBUG(" %s", s);
			}
			delete services;
			*/
			return false;
		}
	}
	catch ( ... ) {}

	global.bindings = new Util::Bindings;
	global.bindings->init(configModule(), name(), true);

	// Subscribe to real-time channels
	Client::Inventory *gInv = Client::Inventory::Instance();
	DataModel::Inventory *inv = gInv->inventory();
	if ( !inv ) {
		SEISCOMP_ERROR("No inventory available, nothing to do");
		return false;
	}

	size_t n, s;
	Core::Time refTime = Core::Time::UTC();

	for ( n = 0; n < inv->networkCount(); ++n ) {
		DataModel::Network *net = inv->network(n);

		for ( s = 0; s < net->stationCount(); ++s ) {
			DataModel::Station *sta = net->station(s);

			if ( refTime < sta->start() ) {
				continue;
			}

			try {
				if ( sta->end() <= refTime ) {
					continue;
				}
			}
			catch ( ... ) {}

			const Util::KeyValues *keys;
			keys = global.bindings->getKeys(net->code(), sta->code());

			Settings::StationDataPtr data = new Settings::StationData;
			data->enabled = keys != nullptr;
			global.stationConfig[sta] = data;
			global.stationIDConfig[net->code() + "." + sta->code()] = data;

			data->bindings = keys;

			if ( !keys ) {
				data->state = Settings::Unconfigured;
				continue;
			}

			if ( !keys->getString(data->detecStream, "detecStream") ) {
				data->state = Settings::NoPrimaryStream;
				continue;
			}

			keys->getString(data->detecLocid, "detecLocid");

			for ( size_t l = 0; l < sta->sensorLocationCount(); ++l ) {
				DataModel::SensorLocation* loc = sta->sensorLocation(l);
				if ( loc->code() != data->detecLocid ) continue;

				if ( refTime < loc->start() ) continue;

				try {
					if ( loc->end() <= refTime ) continue;
				}
				catch (...) {}

				DataModel::Stream *cha = nullptr;

				if ( data->detecStream.size() < 3 ) {
					cha = DataModel::getVerticalComponent(loc, data->detecStream.c_str(), refTime);

					if ( !cha ) {
						SEISCOMP_ERROR("Unable to find meta data for vertical channel of %s.%s.%s.%s",
						               net->code().c_str(), sta->code().c_str(),
						               loc->code().c_str(), data->detecStream.c_str());
					}
				}
				else {
					for ( size_t c = 0; c < loc->streamCount(); ++c ) {
						DataModel::Stream *stream = loc->stream(c);
						if ( stream->code() != data->detecStream ) continue;

						try {
							if ( stream->end() <= refTime ) continue;
						}
						catch (...) {}

						if ( refTime < stream->start() ) continue;

						cha = stream;
						break;
					}

					if ( !cha ) {
						SEISCOMP_ERROR("Unable to find meta data for channel %s.%s.%s.%s",
						               net->code().c_str(), sta->code().c_str(),
						               loc->code().c_str(), data->detecStream.c_str());
					}
				}

				if ( cha ) {
					SEISCOMP_INFO("Register channel %s%s for station %s.%s",
					              loc->code().c_str(), cha->code().c_str(),
					              net->code().c_str(), sta->code().c_str());

					data->channel = cha;

					if ( !_recordStreamThread && !global.offline ) {
						_recordStreamThread = new Gui::RecordStreamThread(recordStreamURL());
						if ( !_recordStreamThread->connect() ) {
							QMessageBox::critical(nullptr, tr("Error"), tr("Failed to create data stream from:\n%1").arg(recordStreamURL().c_str()));
							return false;
						}

						_recordStreamThread->setStartTime(Core::Time::UTC() - global.ringBuffer);
					}

					std::string cid = net->code() + "." + sta->code() + "." + loc->code() + "." + cha->code();

					data->proc = new GroundMotionProcessor;
					data->proc->streamConfig(GroundMotionProcessor::VerticalComponent).init(cha);

					typedef Processing::Settings PS;
					if ( !data->proc->setup(PS(configModuleName(),
					                           net->code(), sta->code(),
					                           loc->code(), cha->code(),
					                           &configuration(), keys)) ) {
						SEISCOMP_ERROR("Failed to setup proc on channel %s.%s.%s.%s",
						               net->code().c_str(), sta->code().c_str(),
						               loc->code().c_str(), data->detecStream.c_str());
					}
					else {
						if ( _recordStreamThread ) {
							_recordStreamThread->addStream(net->code(), sta->code(), loc->code(), cha->code());
						}
						_waveformProcessors[cid] = data.get();
					}
				}

				break;
			}

			// Update configuration state
			if ( !data->bindings ) {
				data->state = Settings::Unconfigured;
			}
			else if ( !data->channel ) {
				data->state = Settings::NoMetaData;
			}
		}
	}

	if ( _recordStreamThread ) {
		connect(_recordStreamThread, SIGNAL(receivedRecord(Seiscomp::Record*)),
		        this, SLOT(handleRecord(Seiscomp::Record*)));
		_recordStreamThread->start();
	}

	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void Application::handleRecord(Record *rec) {
	// This instance must be managed otherwise a memory leak is caused
	RecordPtr tmp(rec);
	std::string cid = rec->streamID();
	WaveformProcessorMap::iterator it = _waveformProcessors.find(cid);

	if ( it == _waveformProcessors.end() ) {
		SEISCOMP_WARNING("Received record for unregistered channel: %s",
		                 cid.c_str());
		return;
	}

	it->second->proc->feed(rec);

	// Store the amplitude in nano units
	it->second->maximumAmplitude = _gmScale ?
		_gmScale->convert(it->second->proc->amplitude(), nullptr)
		:
		it->second->proc->amplitude() * 1E9
	;
	it->second->maximumAmplitudeTimeStamp = it->second->proc->timestamp();

	if ( _mainWindow ) {
		_mainWindow->updateGroundMotion(it->second);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int main(int argc, char **argv) {
	Seiscomp::MapViewX::Application app(argc, argv);
	return app();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
