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


#ifndef SEISCOMP_MAPVIEWX_APP_H
#define SEISCOMP_MAPVIEWX_APP_H


#include <seiscomp/gui/core/application.h>
#include <seiscomp/gui/core/recordstreamthread.h>
#include <seiscomp/plugins/mvx/groundmotion.h>

#include <map>

#include "settings.h"
#include "processor.h"
#include "mainwindow.h"


namespace Seiscomp {
namespace MapViewX {


class Application : public Gui::Kicker<MainWindow> {
	Q_OBJECT


	public:
		Application(int &argc, char **argv);
		~Application();


	public:
		bool initPlugins() override;
		bool validateParameters() override;
		bool init() override;
		void setupUi(MainWindow *mw) override {
			_mainWindow = mw;
		}


	private slots:
		void handleRecord(Seiscomp::Record *rec);


	private:
		using WaveformProcessorMap = std::map<std::string, Settings::StationData*>;

		Gui::RecordStreamThread *_recordStreamThread;
		WaveformProcessorMap     _waveformProcessors;
		MainWindow              *_mainWindow;
		GroundMotionScalePtr     _gmScale;
};


}
}


#endif
