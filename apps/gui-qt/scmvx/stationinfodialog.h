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


#ifndef SEISCOMP_MAPVIEWX_STATIONINFODIALOG_H
#define SEISCOMP_MAPVIEWX_STATIONINFODIALOG_H


#include <seiscomp/datamodel/station.h>
#include <seiscomp/gui/core/recordwidget.h>
#include <seiscomp/gui/core/timescale.h>
#include <seiscomp/gui/core/vruler.h>

#include "settings.h"
#include "ui_stationinfodialog.h"


namespace Seiscomp {
namespace MapViewX {


class StationInfoDialog : public QDialog {
	Q_OBJECT

	public:
		explicit StationInfoDialog(const DataModel::Station *station,
		                           const Settings::StationData *stationData,
		                           QWidget *parent = 0, Qt::WindowFlags f = Qt::WindowFlags());

	public:
		void processingDataUpdated(const Settings::StationData *stationData);
		void shiftData();

	private:
		Ui::StationInfoDialog  _ui;
		Gui::RecordWidget     *_trace[2];
		Gui::TimeScale        *_timeScale;
		Gui::VRuler           *_scale[2];
};


}
}


#endif
