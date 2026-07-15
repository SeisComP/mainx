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


#include <seiscomp/datamodel/network.h>
#include <seiscomp/datamodel/pick.h>
#include <seiscomp/datamodel/sensorlocation.h>
#include <seiscomp/datamodel/stream.h>
#include <seiscomp/math/filter.h>
#include <seiscomp/gui/core/application.h>
#include <seiscomp/gui/core/compat.h>
#include <seiscomp/gui/core/icon.h>

#include "stationinfodialog.h"


#define NUMBER_REFERENCE_STRING " -1.00E-999"


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace Seiscomp {
namespace MapViewX {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


namespace {


// Returns true if the station has streams and all stream epochs ended before
// 'ref'. On success 'lastEnd' holds the most recent stream end time.
bool allStreamsClosed(const DataModel::Station *sta, const Core::Time &ref,
                      Core::Time &lastEnd) {
	bool found = false;

	for ( size_t l = 0; l < sta->sensorLocationCount(); ++l ) {
		DataModel::SensorLocation *loc = sta->sensorLocation(l);
		for ( size_t c = 0; c < loc->streamCount(); ++c ) {
			Core::Time end;
			try {
				end = loc->stream(c)->end();
			}
			catch ( ... ) {
				// Open-ended stream epoch
				return false;
			}

			if ( ref < end ) {
				// Stream epoch still open at the reference time
				return false;
			}

			if ( !found || (lastEnd < end) ) {
				lastEnd = end;
				found = true;
			}
		}
	}

	return found;
}


}




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
StationInfoDialog::StationInfoDialog(const DataModel::Station *station,
                                     const Settings::StationData *stationData,
                                     QWidget *parent, Qt::WindowFlags f)
: QDialog(parent, f) {
	_ui.setupUi(this);

	setWindowTitle(tr("Station information: %1.%2")
	               .arg(station->network()->code().c_str())
	               .arg(station->code().c_str()));

	connect(_ui.buttonOK, &QPushButton::clicked, this, &QDialog::accept);

	_ui.buttonToggleEnabled->setAutoDefault(false);
	_ui.buttonToggleEnabled->setDefault(false);
	_ui.buttonOK->setAutoDefault(true);
	_ui.buttonOK->setDefault(true);
	_ui.buttonOK->setFocus();

	_timeScale = new Gui::TimeScale(this);
	_timeScale->setAbsoluteTimeEnabled(true);
	_timeScale->setAutoScaleEnabled(true);

	for ( int i = 0; i < 2; ++i ) {
		_trace[i] = new Gui::RecordWidget(this);
		_trace[i]->setAutoFillBackground(false);
		_trace[i]->showScaledValues(true);
		_trace[i]->setDrawOffset(false);

		_scale[i] = new Gui::VRuler(this);
		_scale[i]->setFixedWidth(QT_FM_WIDTH(fontMetrics(), NUMBER_REFERENCE_STRING) + fontMetrics().height() + 4);

		connect(_timeScale, SIGNAL(changedInterval(double,double,double)),
		        _trace[i], SLOT(setGridSpacing(double,double,double)));
		connect(_scale[i], SIGNAL(changedInterval(double,double,double)),
		        _trace[i], SLOT(setGridVSpacing(double,double,double)));
		connect(_scale[i], SIGNAL(scaleChanged(double)),
		        _trace[i], SLOT(setGridVScale(double)));
		connect(_timeScale, SIGNAL(scaleChanged(double)),
		        _trace[i], SLOT(setTimeScale(double)));
		connect(_timeScale, SIGNAL(rangeChangeRequested(double,double)),
		        _trace[i], SLOT(setTimeRange(double,double)));
		connect(_trace[i], SIGNAL(traceUpdated(Seiscomp::Gui::RecordWidget*)),
		        _scale[i], SLOT(updateScale(Seiscomp::Gui::RecordWidget*)));
	}

	_trace[1]->setRecordFilter(0, Gui::RecordWidget::Filter::Create("self"));
	_trace[1]->setRecordColor(0, Qt::darkBlue);
	_trace[1]->setRecordAntialiasing(0, true);
	_trace[1]->setRecordFilter(1, Gui::RecordWidget::Filter::Create("self*-1"));
	_trace[1]->setRecordColor(1, Qt::blue);
	_trace[1]->setRecordAntialiasing(1, true);
	_trace[1]->setRecordFilter(2, Gui::RecordWidget::Filter::Create("self"));
	_trace[1]->enableRecordFiltering(0, true);
	_trace[1]->enableRecordFiltering(1, true);
	_trace[1]->enableRecordFiltering(2, true);
	_trace[1]->setDrawMode(Gui::RecordWidget::Stacked);

	if ( stationData && stationData->channel ) {
		_scale[0]->setAnnotation((stationData->channel->gainUnit() + " * 1E09").c_str());
		_scale[1]->setAnnotation("M/S * 1E09");
	}
	else {
		_scale[0]->setAnnotation(tr("???"));
		_scale[1]->setAnnotation(tr("???"));
	}

	QWidget *dummy = new QWidget;

	QLabel *rawLabel = new QLabel;
	rawLabel->setText(tr("Unfiltered data as acquired from the sensor with applied gain."));
	rawLabel->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));

	QLabel *processedLabel = new QLabel;
	processedLabel->setText(tr("Processed data: filtered, converted to velocity and computed maximum over past %1s.").arg((double)global.maximumAmplitudeTimeSpan));
	processedLabel->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));

	QGridLayout *gl = new QGridLayout;
	gl->addWidget(rawLabel, 0, 0, 1, 2);
	gl->addWidget(_scale[0], 1, 0);
	gl->addWidget(_trace[0], 1, 1);
	gl->addWidget(processedLabel, 2, 0, 1, 2);
	gl->addWidget(_scale[1], 3, 0);
	gl->addWidget(_trace[1], 3, 1);
	gl->addWidget(_timeScale, 4, 1);

	dummy->setLayout(gl);
	dummy->setBackgroundRole(QPalette::Base);
	dummy->setAutoFillBackground(true);

	QVBoxLayout *vl = new QVBoxLayout;
	vl->setContentsMargins(0, 0, 0, 0);
	vl->addWidget(dummy);

	_ui.frameTrace->setLayout(vl);

	_ui.labelFilter->setText(global.filter.c_str());
	if ( stationData && stationData->channel )
		_ui.labelCode->setText((station->network()->code() + "." + station->code() + "." +
		                        stationData->channel->sensorLocation()->code() + "." +
		                        stationData->channel->code()).c_str());
	else
		_ui.labelCode->setText((station->network()->code() + "." + station->code()).c_str());
	_ui.labelNetwork->setText(station->network()->description().c_str());
	_ui.labelDescription->setText(station->description().c_str());

	Core::Time lastEnd;
	if ( allStreamsClosed(station, Core::Time::UTC(), lastEnd) ) {
		_ui.labelIssueIcon->setPixmap(Gui::icon("close", QColor(192, 0, 0)).pixmap(fontMetrics().height() * 2));
		_ui.labelIssueText->setText(tr("All streams from the station closed in the past. Last end time: %1")
		                            .arg(lastEnd.toString("%F %T").c_str()));
	}
	else if ( stationData ) {
		QIcon icon;

		switch ( stationData->state ) {
			case Settings::OK:
				icon = Gui::icon("check", QColor(Qt::darkGreen));
				_ui.labelIssueText->setText(tr("No issues detected."));
				break;

			case Settings::Unknown:
				icon = Gui::icon("question_mark");
				_ui.labelIssueText->setText(tr("The station is unknown to the system."));
				break;

			case Settings::Unconfigured:
				icon = Gui::icon("settings", QColor(255,128,0));
				_ui.labelIssueText->setText(tr("The station does not have global bindings."));
				break;

			case Settings::NoPrimaryStream:
				icon = Gui::icon("settings", QColor(255,128,0));
				_ui.labelIssueText->setText(tr("The parameter 'detecStream' is not configured by global bindings."));
				break;

			case Settings::NoChannelGroupMetaData:
				icon = Gui::icon("database", QColor(Qt::darkRed));
				_ui.labelIssueText->setText(tr("The configured bindings channel %1%2 is not part of the stations metadata.")
				                            .arg(stationData->detecLocid.c_str(), stationData->detecStream.c_str()));
				break;

			case Settings::NoVerticalChannelMetaData:
				icon = Gui::icon("database", QColor(Qt::darkRed));
				_ui.labelIssueText->setText(tr("The configured bindings channel group %1%2 has no defined vertical channel in the stations metadata.")
				                            .arg(stationData->detecLocid.c_str(), stationData->detecStream.c_str()));
				break;

			default:
				break;
		}

		if ( !icon.isNull() ) {
			_ui.labelIssueIcon->setPixmap(icon.pixmap(fontMetrics().height() * 2));
		}

		if ( stationData->proc ) {
			_trace[0]->setRecordScale(0, stationData->proc->dataScale() * 1E9);
			_trace[1]->setRecordScale(0, stationData->proc->dataScale() * 1E9);
			processingDataUpdated(stationData);
			shiftData();
		}
	}
	else {
		_ui.labelIssueIcon->setPixmap(Gui::pixmap(this, "question", Qt::darkGray, 2.0));
		_ui.labelIssueText->setText(tr("The station is unknown to the system."));
	}

	_hasData = stationData != nullptr;
	_enabled = stationData && stationData->enabled;
	updateEnabledUi();

	if ( stationData ) {
		addPickMarkers(stationData);
	}

	connect(_ui.buttonToggleEnabled, &QPushButton::clicked, this, [this]() {
		if ( !_hasData ) {
			return;
		}

		_enabled = !_enabled;
		updateEnabledUi();
		emit setStationEnabled(_enabled);
	});
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationInfoDialog::updateEnabledUi() {
	if ( !_hasData ) {
		_ui.labelStatus->setText(tr("-"));
		_ui.buttonToggleEnabled->setEnabled(false);
		return;
	}

	_ui.labelStatus->setText(_enabled ? tr("Enabled") : tr("Disabled"));
	_ui.buttonToggleEnabled->setText(_enabled ? tr("Disable") : tr("Enable"));
	_ui.buttonToggleEnabled->setToolTip(_enabled ? tr("Disable this station")
	                                             : tr("Enable this station"));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationInfoDialog::addPickMarkers(const Settings::StationData *stationData) {
	for ( const DataModel::PickPtr &pick : stationData->picks ) {
		addPick(pick.get());
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationInfoDialog::addPick(DataModel::Pick *pick) {
	Gui::RecordMarker *marker =
		new Gui::RecordMarker(nullptr, pick->time().value());

	QString phaseCode;
	try {
		phaseCode = pick->phaseHint().code().c_str();
	}
	catch ( ... ) {}
	marker->setText(phaseCode);
	marker->setMovable(false);

	try {
		switch ( pick->evaluationMode() ) {
			case DataModel::AUTOMATIC:
				marker->setColor(SCScheme.colors.picks.automatic);
				break;
			case DataModel::MANUAL:
				marker->setColor(SCScheme.colors.picks.manual);
				break;
			default:
				marker->setColor(SCScheme.colors.picks.undefined);
				break;
		}
	}
	catch ( ... ) {
		marker->setColor(SCScheme.colors.picks.undefined);
	}

	_trace[0]->addMarker(marker);
	_trace[0]->update();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationInfoDialog::processingDataUpdated(const Settings::StationData *stationData) {
	RecordSequence *seq = stationData->proc->rawData();
	if ( seq ) {
		_trace[0]->setRecords(0, seq, false);
	}

	seq = stationData->proc->processedData();
	if ( seq ) {
		_trace[1]->setRecords(0, seq, false);
		_trace[1]->setRecords(1, seq, false);
	}

	seq = stationData->proc->velocityData();
	if ( seq ) {
		_trace[1]->setRecords(2, seq, false);
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void StationInfoDialog::shiftData() {
	Core::Time now = Core::Time::UTC();

	auto left = static_cast<double>(now - global.ringBuffer);
	auto right = static_cast<double>(now);

	for ( int i = 0; i < 2; ++i ) {
		_trace[i]->showTimeRange(left, right);
	}

	_timeScale->showRange(left, right);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
