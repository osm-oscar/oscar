#include "SearchGeometryModel.h"

namespace oscar_gui {

static QString sgt2Str(SearchGeometryState::DataType t) {
	switch (t) {
	case SearchGeometryState::DT_POINT:
		return "Point";
	case SearchGeometryState::DT_RECT:
		return "Rectangle";
	case SearchGeometryState::DT_PATH:
		return "Path";
	case SearchGeometryState::DT_POLYGON:
		return "Polygon";
	default:
		return "INVALID";
	};
}

SearchGeometryModel::SearchGeometryModel(const std::shared_ptr<SearchGeometryState> & sgs) :
m_sgs(sgs)
{
	connect(m_sgs.get(), SIGNAL(dataChanged()), this, SIGNAL(modelReset()));
}

SearchGeometryModel::~SearchGeometryModel() {}

int SearchGeometryModel::columnCount(const QModelIndex& /*parent*/) const {
	return __CD_COUNT;
}

int SearchGeometryModel::rowCount(const QModelIndex& /*parent*/) const {
	return m_sgs->size();
}

QModelIndex SearchGeometryModel::index(int row, int column, const QModelIndex& /*parent*/) const {
	return QAbstractItemModel::createIndex(row, column);
}

QModelIndex SearchGeometryModel::parent(const QModelIndex& /*child*/) const {
	return QModelIndex();
}

QVariant SearchGeometryModel::data(const QModelIndex& index, int role) const {
	if (index.row() < 0 || (std::size_t) index.row() >= m_sgs->size() || role != Qt::DisplayRole) {
		return QVariant();
	}
	switch (index.column()) {
	case CD_NAME:
		return QVariant( m_sgs->name(index.row()) );
	case CD_TYPE:
		return QVariant( sgt2Str( m_sgs->type(index.row()) ) );
	case CD_ACTIVE:
		return QVariant( m_sgs->active(index.row()) ? "True" : "False" );
	default:
		return QVariant();
	};
}

} //end namespace oscar_gui