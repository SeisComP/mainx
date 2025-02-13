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


#ifndef SEISCOMP_MAPVIEWX_EVENTINFODIALOG_H
#define SEISCOMP_MAPVIEWX_EVENTINFODIALOG_H


#include <seiscomp/datamodel/event.h>
#include <seiscomp/gui/core/inspector.h>

#include "settings.h"
#include "ui_eventinfodialog.h"


namespace Seiscomp {
namespace MapViewX {


class EventInfoDialog : public QDialog {
	Q_OBJECT

	public:
		explicit EventInfoDialog(QWidget *parent = 0, Qt::WindowFlags f = Qt::WindowFlags());

	public:
		void setEvent(const DataModel::Event *event);

	private slots:
		void showDetails();

	private:
		Ui::EventInfoDialog   _ui;
		Gui::Inspector       *_inspector;
		DataModel::EventCPtr  _event;
};


}
}


#endif
