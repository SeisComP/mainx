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


#include <seiscomp/datamodel/arrival.h>
#include <seiscomp/datamodel/event.h>
#include <seiscomp/datamodel/origin.h>
#include <seiscomp/datamodel/utils.h>
#include <seiscomp/gui/core/application.h>
#include <seiscomp/gui/core/utils.h>
#include <seiscomp/seismology/regions.h>

#include <QColor>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "eventinfodialog.h"


namespace Seiscomp {
namespace MapViewX {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
EventInfoDialog::EventInfoDialog(QWidget *parent, Qt::WindowFlags f)
: QDialog(parent, f) {
	_ui.setupUi(this);

	_inspector = new Gui::Inspector;
	_inspector->layout()->setContentsMargins(0, 0, 0, 0);

	// Summary of the preferred origin, similar to scolv's location tab.
	_regionLabel = new QLabel;
	_timeLabel = new QLabel;
	_latitudeLabel = new QLabel;
	_longitudeLabel = new QLabel;
	_depthLabel = new QLabel;
	_magnitudeLabel = new QLabel;
	_agencyLabel = new QLabel;
	_evaluationLabel = new QLabel;
	_eventTypeLabel = new QLabel;
	_arrivalCountLabel = new QLabel;
	_rmsLabel = new QLabel;

	QFormLayout *leftForm = new QFormLayout;
	leftForm->setContentsMargins(0, 0, 0, 0);
	leftForm->addRow(tr("Origin time:"), _timeLabel);
	leftForm->addRow(tr("Region:"), _regionLabel);
	leftForm->addRow(tr("Latitude:"), _latitudeLabel);
	leftForm->addRow(tr("Longitude:"), _longitudeLabel);
	leftForm->addRow(tr("Depth:"), _depthLabel);
	leftForm->addRow(tr("Magnitude:"), _magnitudeLabel);

	QFormLayout *rightForm = new QFormLayout;
	rightForm->setContentsMargins(0, 0, 0, 0);
	rightForm->addRow(tr("Agency:"), _agencyLabel);
	rightForm->addRow(tr("Evaluation:"), _evaluationLabel);
	rightForm->addRow(tr("Phases:"), _arrivalCountLabel);
	rightForm->addRow(tr("RMS:"), _rmsLabel);
	rightForm->addRow(tr("Event type:"), _eventTypeLabel);

	// Two columns aligned to the bottom (the top stretch pushes the shorter
	// right column down so both bottom rows line up).
	QVBoxLayout *leftColumn = new QVBoxLayout;
	leftColumn->addStretch(1);
	leftColumn->addLayout(leftForm);

	QVBoxLayout *rightColumn = new QVBoxLayout;
	rightColumn->addStretch(1);
	rightColumn->addLayout(rightForm);

	QHBoxLayout *columns = new QHBoxLayout;
	columns->setContentsMargins(4, 4, 4, 4);
	columns->addLayout(leftColumn);
	columns->addLayout(rightColumn);

	QVBoxLayout *vl = new QVBoxLayout;
	vl->setContentsMargins(0, 0, 0, 0);
	vl->addLayout(columns);
	vl->addWidget(_inspector);

	_ui.frame->setLayout(vl);

	connect(_ui.btnShowDetails, &QPushButton::clicked, this, &EventInfoDialog::showDetails);

	_ui.btnShowDetails->setAutoDefault(false);
	_ui.btnShowDetails->setDefault(false);
	_ui.btnClose->setAutoDefault(true);
	_ui.btnClose->setDefault(true);
	_ui.btnClose->setFocus();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventInfoDialog::setEvent(const DataModel::Event *event,
                              const DataModel::Origin *origin,
                              const DataModel::Magnitude *magnitude){
	_event = event;
	_inspector->setObject(const_cast<DataModel::Event*>(_event.get()));
	setWindowTitle(tr("Event details for %1").arg(event->publicID().c_str()));

	// Event type (from the event itself).
	try {
		_eventTypeLabel->setText(event->type().toString());
	}
	catch ( ... ) {
		_eventTypeLabel->setText("-");
	}

	// Preferred magnitude with its type in brackets.
	if ( magnitude ) {
		QString text = QString::number(magnitude->magnitude().value(), 'f',
		                               SCScheme.precision.magnitude);
		if ( !magnitude->type().empty() ) {
			text += QString(" (%1)").arg(magnitude->type().c_str());
		}
		_magnitudeLabel->setText(text);
	}
	else {
		_magnitudeLabel->setText("-");
	}

	if ( !origin ) {
		_regionLabel->setText("-");
		_timeLabel->setText("-");
		_latitudeLabel->setText("-");
		_longitudeLabel->setText("-");
		_depthLabel->setText("-");
		_agencyLabel->setText("-");
		_evaluationLabel->setText("-");
		_arrivalCountLabel->setText("-");
		_rmsLabel->setText("-");
		return;
	}

	double lat = origin->latitude().value();
	double lon = origin->longitude().value();

	_regionLabel->setText(Regions::getRegionName(lat, lon).c_str());

	std::string timeFormat = "%Y-%m-%d %H:%M:%S";
	if ( SCScheme.precision.originTime > 0 ) {
		timeFormat += ".%" + std::to_string(SCScheme.precision.originTime) + "f";
	}
	QString timeZone = SCScheme.dateTime.useLocalTime
	                   ? QString(Core::Time::LocalTimeZone().c_str())
	                   : QString("UTC");
	_timeLabel->setText(Gui::timeToString(origin->time().value(), timeFormat.c_str())
	                    + QString(" (%1)").arg(timeZone));

	_latitudeLabel->setText(Gui::latitudeToString(lat, true, true, SCScheme.precision.location));
	_longitudeLabel->setText(Gui::longitudeToString(lon, true, true, SCScheme.precision.location));

	try {
		_depthLabel->setText(Gui::depthToString(origin->depth().value(),
		                                        SCScheme.precision.depth) + " km");
	}
	catch ( ... ) {
		_depthLabel->setText("-");
	}

	try {
		std::string agency = origin->creationInfo().agencyID();
		_agencyLabel->setText(agency.empty() ? "-" : agency.c_str());
	}
	catch ( ... ) {
		_agencyLabel->setText("-");
	}

	// Evaluation status as a single letter (like the event list); "A" when the
	// status is unset. Colored according to the evaluation mode.
	char statusChar = DataModel::objectEvaluationStatusToChar(origin);
	_evaluationLabel->setText(statusChar ? QString(QChar(statusChar)) : QString("A"));

	QColor statusColor;
	try {
		std::string mode = origin->evaluationMode().toString();
		if ( mode == "automatic" ) {
			statusColor = SCScheme.colors.originStatus.automatic;
		}
		else if ( mode == "manual" ) {
			statusColor = SCScheme.colors.originStatus.manual;
		}
	}
	catch ( ... ) {}
	_evaluationLabel->setStyleSheet(statusColor.isValid()
	                                ? QString("color: %1;").arg(statusColor.name())
	                                : QString());

	// Phases: used arrivals (bold) / total arrival count, like scolv. Count the
	// used arrivals from the loaded arrivals when present, otherwise fall back
	// to the stored quality counts.
	int arrivalCount = static_cast<int>(origin->arrivalCount());
	int usedArrivals = -1;
	if ( arrivalCount > 0 ) {
		usedArrivals = 0;
		for ( size_t i = 0; i < origin->arrivalCount(); ++i ) {
			DataModel::Arrival *arrival = origin->arrival(i);
			try {
				if ( arrival->weight() > 0.0 ) {
					++usedArrivals;
				}
			}
			catch ( ... ) {
				// No weight set (older origins): count as used.
				++usedArrivals;
			}
		}
	}
	else {
		try { usedArrivals = origin->quality().usedPhaseCount(); }
		catch ( ... ) {}
		try { arrivalCount = origin->quality().associatedPhaseCount(); }
		catch ( ... ) { arrivalCount = -1; }
	}

	QString used = usedArrivals >= 0 ? QString::number(usedArrivals) : QString("-");
	QString total = arrivalCount >= 0 ? QString::number(arrivalCount) : QString("-");
	_arrivalCountLabel->setText(QString("<b>%1</b> / %2").arg(used, total));

	try {
		_rmsLabel->setText(QString("%1 s").arg(origin->quality().standardError(), 0, 'f',
		                                       SCScheme.precision.rms));
	}
	catch ( ... ) {
		_rmsLabel->setText("-");
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void EventInfoDialog::showDetails() {
	SCApp->sendCommand(Gui::CM_SHOW_ORIGIN, _event->preferredOriginID().c_str());
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
