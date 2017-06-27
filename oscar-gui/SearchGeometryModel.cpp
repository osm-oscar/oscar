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
	connect(m_sgs.get(), SIGNAL(dataChanged(int)), this, SIGNAL(modelReset()));
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
	case CD_SHOW:
		return QVariant( m_sgs->active(index.row()) & SearchGeometryState::AT_SHOW ? "True" : "False" );
	case CD_SHOW_TRIANGLES:
		return QVariant( m_sgs->active(index.row()) & SearchGeometryState::AT_TRIANGLES ? "True" : "False" );
	case CD_SHOW_CELLS:
		return QVariant( m_sgs->active(index.row()) & SearchGeometryState::AT_CELLS ? "True" : "False" );
	default:
		return QVariant();
	};
}

QVariant SearchGeometryModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
			case (CD_NAME):
				return QVariant("Name");
			case (CD_TYPE):
				return QVariant("Type");
			case (CD_SHOW):
				return QVariant("Show");
			case (CD_SHOW_CELLS):
				return QVariant("Show cells");
			case (CD_SHOW_TRIANGLES):
				return QVariant("Show Triangles");
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

void SearchGeometryModel::doubleClicked(const QModelIndex & index) {
	switch (index.column()) {
	case CD_SHOW:
		emit showGeometryClicked(index.row());
		break;
	case CD_SHOW_TRIANGLES:
		emit showTrianglesClicked(index.row());
		break;
	case CD_SHOW_CELLS:
		emit showCellsClicked(index.row());
		break;
	default:
		break;
	}
}

void SearchGeometryModel::clicked(const QModelIndex& index) {
	emit geometryClicked(index.row());
}


} //end namespace oscar_gui