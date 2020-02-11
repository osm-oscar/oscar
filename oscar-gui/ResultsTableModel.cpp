#include "ResultsTableModel.h"

namespace oscar_gui {

ResultsTableModel::ResultsTableModel(const States & states) :
m_store(states.cmp->store()),
m_igs(states.igs),
m_rls(states.rls)
{
	m_nameStrId = m_store.keyStringTable().find("name");
	connect(m_igs.get(), &ItemGeometryState::dataChanged, this, &ResultsTableModel::resetModel);
	connect(m_rls.get(), &ResultListState::dataChanged, this, &ResultsTableModel::resetModel);
	connect(this, &ResultsTableModel::toggleItem, m_igs.get(), &ItemGeometryState::toggleItem);
	connect(this, &ResultsTableModel::zoomToItem, m_igs.get(), &ItemGeometryState::zoomToItem);
}

ResultsTableModel::~ResultsTableModel() {}

int ResultsTableModel::columnCount(const QModelIndex& /*parent*/) const {
	return __CD_COUNT;
}

int ResultsTableModel::rowCount(const QModelIndex& /*parent*/) const {
	return m_rls->size();
}

QModelIndex ResultsTableModel::index(int row, int column, const QModelIndex& /*parent*/) const {
	return QAbstractItemModel::createIndex(row, column);
}

QModelIndex ResultsTableModel::parent(const QModelIndex& /*child*/) const {
	return QModelIndex();
}

QVariant ResultsTableModel::data(const QModelIndex& index, int role) const {
	if (index.row() < 0 || (std::size_t) index.row() >= m_rls->size() || role != Qt::DisplayRole) {
		return QVariant();
	}
	auto itemId = m_rls->itemId(index.row());
	auto item = m_store.at(itemId);

	switch (index.column()) {
	case CD_ID:
		return QVariant( itemId );
	case CD_SHOW:
		return QVariant( m_igs->active(itemId) & SearchGeometryState::AT_SHOW ? "True" : "False" );
	case CD_SHOW_CELLS:
		return QVariant( m_igs->active(itemId) & SearchGeometryState::AT_CELLS ? "True" : "False" );
	case CD_SCORE:
		return QVariant( item.score() );
	case CD_NAME:
	{
		auto namePos = item.findKey(m_nameStrId);
		QString name;
		if (namePos != item.npos) {
			name = QString::fromStdString( item.value(namePos) );
		}
		return QVariant(name);
	}
	default:
		return QVariant();
	};
}

QVariant ResultsTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
			case CD_ID:
				return QVariant("Id");
			case CD_SHOW:
				return QVariant("Show");
			case CD_SHOW_CELLS:
				return QVariant("Show cells");
			case CD_SCORE:
				return QVariant("Score");
			case CD_NAME:
				return QVariant("Name");
			default:
				return QVariant();
			}
		}
		else {
			return QVariant(section);
		}
	}
	return QVariant();
}

void ResultsTableModel::doubleClicked(const QModelIndex & index) {
	
	auto itemId = m_rls->itemId(index.row());
	switch (index.column()) {
	case CD_SHOW:
		emit toggleItem(itemId, GeometryState::AT_SHOW);
		break;
	case CD_SHOW_CELLS:
		emit toggleItem(itemId, GeometryState::AT_CELLS);
		break;
	default:
		emit zoomToItem(itemId);
		break;
	}
}

void ResultsTableModel::clicked(const QModelIndex& index) {
	doubleClicked(index);
}

void ResultsTableModel::headerClicked(int /*index*/) {
	;
}

void ResultsTableModel::resetModel() {
	beginResetModel();
	endResetModel();
}

} //end namespace oscar_gui
