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


#include "searchwidget.h"
#include "settings.h"

#include <seiscomp/datamodel/network.h>

#include <QStandardItemModel>
#include <QSortFilterProxyModel>


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace Seiscomp {
namespace MapViewX {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SearchWidget::SearchWidget(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f) {
	_ui.setupUi(this);
	setWindowFlags(Qt::Tool);
	setAttribute(Qt::WA_DeleteOnClose);

	_ui.tableView->setSortingEnabled(true);
	_ui.tableView->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
	_ui.tableView->horizontalHeader()->setStretchLastSection(true);
	_ui.tableView->verticalHeader()->setHidden(true);

	connect(_ui.btnShow, SIGNAL(clicked()), this, SIGNAL(filterView()));
	connect(_ui.btnCancel, SIGNAL(clicked()), this, SLOT(close()));

	QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
	QStandardItemModel *sourceModel = new QStandardItemModel(this);
	sourceModel->setColumnCount(1);
	sourceModel->setRowCount(global.stationIDConfig.size());
	sourceModel->setHorizontalHeaderLabels(QStringList() << tr("Network/Station"));

	int row = 0;
	for ( auto &item : global.stationConfig ) {
		std::string cid = item.first->network()->code() + "." + item.first->code();
		if ( item.second->channel ) {
			cid += ".";
			cid += item.second->channel->sensorLocation()->code();
			cid += ".";
			cid += item.second->channel->code();
		}
		QStandardItem *modelItem = new QStandardItem(cid.c_str());
		modelItem->setFlags(modelItem->flags() & ~Qt::ItemIsEditable);
		modelItem->setData(QVariant::fromValue<void*>(item.first));
		sourceModel->setItem(row, 0, modelItem);
		++row;
	}

	proxyModel->setSourceModel(sourceModel);
	_ui.tableView->setModel(proxyModel);
	proxyModel->sort(0, _ui.tableView->horizontalHeader()->sortIndicatorOrder());

	connect(_ui.tableView, SIGNAL(activated(const QModelIndex&)),
	        this, SLOT(itemActivated(const QModelIndex&)));

	connect(_ui.lineEdit, SIGNAL(textEdited(const QString&)),
	        proxyModel, SLOT(setFilterWildcard(const QString&)));
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
QSet<const DataModel::Station *> SearchWidget::visibleData() const {
	QSortFilterProxyModel *proxyModel = static_cast<QSortFilterProxyModel*>(_ui.tableView->model());
	QStandardItemModel *sourceModel = static_cast<QStandardItemModel*>(proxyModel->sourceModel());
	QSet<const DataModel::Station*> result;

	auto indexes = _ui.tableView->selectionModel()->selectedIndexes();
	for ( const QModelIndex &idx : indexes ) {
		QModelIndex sourceIdx = proxyModel->mapToSource(idx);
		result.insert(reinterpret_cast<DataModel::Station*>(sourceModel->item(sourceIdx.row(), sourceIdx.column())->data().value<void*>()));
	}

	return result;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void SearchWidget::itemActivated(const QModelIndex &idx) {
	emit filterView();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
