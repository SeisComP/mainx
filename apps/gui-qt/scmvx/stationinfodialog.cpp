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
#include <seiscomp/math/filter.h>
#include <seiscomp/gui/core/compat.h>
#include <seiscomp/gui/core/icon.h>

#include "stationinfodialog.h"


#define NUMBER_REFERENCE_STRING " -1.00E-999"


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace Seiscomp {
namespace MapViewX {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
StationInfoDialog::StationInfoDialog(const DataModel::Station *station,
                                     const Settings::StationData *stationData,
                                     QWidget *parent, Qt::WindowFlags f)
: QDialog(parent, f) {
	_ui.setupUi(this);

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

	if ( stationData->channel ) {
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
	if ( stationData->channel )
		_ui.labelCode->setText((station->network()->code() + "." + station->code() + "." +
		                        stationData->channel->sensorLocation()->code() + "." +
		                        stationData->channel->code()).c_str());
	else
		_ui.labelCode->setText((station->network()->code() + "." + station->code()).c_str());
	_ui.labelNetwork->setText(station->network()->description().c_str());
	_ui.labelDescription->setText(station->description().c_str());

	if ( stationData ) {
		QIcon icon;

		switch ( stationData->state ) {
			case Settings::OK:
				icon = Gui::icon("check", QColor(Qt::darkGreen));
				_ui.labelIssueText->setText(tr("No issues detected."));
				break;

			case Settings::Unknown:
				icon = Gui::icon("question");
				_ui.labelIssueText->setText(tr("The station is unknown to the system."));
				break;

			case Settings::Unconfigured:
				icon = Gui::icon("wrench", QColor(255,128,0));
				_ui.labelIssueText->setText(tr("The station does not have global bindings."));
				break;

			case Settings::NoPrimaryStream:
				icon = Gui::icon("wrench", QColor(255,128,0));
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
