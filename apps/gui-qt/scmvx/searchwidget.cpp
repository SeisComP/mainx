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

#include <QCheckBox>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
namespace Seiscomp {
namespace MapViewX {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


namespace {


const int ClosedRole = Qt::UserRole + 2;
const int VisibleRole = Qt::UserRole + 3;


class StationSearchProxyModel : public QSortFilterProxyModel {
	public:
		explicit StationSearchProxyModel(QObject *parent = nullptr)
		: QSortFilterProxyModel(parent) {}

		void setIncludeClosed(bool enable) {
			_includeClosed = enable;
			invalidateFilter();
		}

		void setOnlyVisible(bool enable) {
			_onlyVisible = enable;
			invalidateFilter();
		}

		void setSearchText(const QString &text) {
			_searchText = text;
			setFilterWildcard(text);
		}

	protected:
		bool filterAcceptsRow(int row, const QModelIndex &parent) const override {
			QModelIndex idx = sourceModel()->index(row, 0, parent);

			// When restricted to the map, list only stations currently shown
			// there. The map already applies the network/closed filters, so no
			// further open/closed filtering is done in this case.
			if ( _onlyVisible ) {
				if ( !idx.data(VisibleRole).toBool() ) {
					return false;
				}
				if ( !_searchText.isEmpty() ) {
					return QSortFilterProxyModel::filterAcceptsRow(row, parent);
				}
				return true;
			}

			// A search string lifts the open/closed restriction so that any
			// station can be found.
			if ( !_searchText.isEmpty() ) {
				return QSortFilterProxyModel::filterAcceptsRow(row, parent);
			}

			// Without a search string, closed stations are listed only when
			// explicitly requested.
			if ( !_includeClosed && idx.data(ClosedRole).toBool() ) {
				return false;
			}

			return true;
		}

	private:
		bool    _includeClosed{false};
		bool    _onlyVisible{false};
		QString _searchText;
};


}




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
SearchWidget::SearchWidget(const std::set<std::string> &closedCodes,
                           const std::set<std::string> &visibleCodes,
                           QWidget *parent, Qt::WindowFlags f)
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

	StationSearchProxyModel *proxyModel = new StationSearchProxyModel(this);
	QStandardItemModel *sourceModel = new QStandardItemModel(this);
	sourceModel->setColumnCount(1);
	sourceModel->setRowCount(global.stationIDConfig.size());
	sourceModel->setHorizontalHeaderLabels(QStringList() << tr("Network/Station"));

	int row = 0;
	for ( auto &item : global.stationConfig ) {
		std::string netSta = item.first->network()->code() + "." + item.first->code();
		std::string cid = netSta;
		if ( item.second->channel ) {
			cid += ".";
			cid += item.second->channel->sensorLocation()->code();
			cid += ".";
			cid += item.second->channel->code();
		}
		QStandardItem *modelItem = new QStandardItem(cid.c_str());
		modelItem->setFlags(modelItem->flags() & ~Qt::ItemIsEditable);
		modelItem->setData(QVariant::fromValue<void*>(item.first));
		modelItem->setData(closedCodes.find(netSta) != closedCodes.end(), ClosedRole);
		modelItem->setData(visibleCodes.find(netSta) != visibleCodes.end(), VisibleRole);
		sourceModel->setItem(row, 0, modelItem);
		++row;
	}

	proxyModel->setSourceModel(sourceModel);
	_ui.tableView->setModel(proxyModel);
	proxyModel->sort(0, _ui.tableView->horizontalHeader()->sortIndicatorOrder());

	connect(_ui.tableView, SIGNAL(activated(const QModelIndex&)),
	        this, SLOT(itemActivated(const QModelIndex&)));

	connect(_ui.lineEdit, &QLineEdit::textEdited, this,
	        [proxyModel](const QString &text) {
		proxyModel->setSearchText(text);
	});

	connect(_ui.checkIncludeClosed, &QCheckBox::toggled, this,
	        [proxyModel](bool enable) {
		proxyModel->setIncludeClosed(enable);
	});

	connect(_ui.checkOnlyVisible, &QCheckBox::toggled, this,
	        [proxyModel](bool enable) {
		proxyModel->setOnlyVisible(enable);
	});
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
