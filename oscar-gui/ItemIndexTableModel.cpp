#include "ItemIndexTableModel.h"
#include <QRunnable>

oscar_gui::ItemIndexTableModel::ItemIndexTableModel(QObject* parent): QAbstractTableModel(parent)
{}

oscar_gui::ItemIndexTableModel::~ItemIndexTableModel() {}


int oscar_gui::ItemIndexTableModel::columnCount(const QModelIndex&) const {
	return CN_COL_COUNT; //osmid, key=value GeoPoints
}

int oscar_gui::ItemIndexTableModel::rowCount(const QModelIndex&) const {
	return m_idx.size();
}

QVariant oscar_gui::ItemIndexTableModel::data(const QModelIndex& index, int role) const {

	if (role == Qt::DisplayRole) {

		int col = index.column();
		int row = index.row();
		
		liboscar::Static::OsmKeyValueObjectStore::Item item = m_store.at( m_idx.at(row) );
		switch (col) {
		case CN_OSMID:
			return QVariant( QString::number(item.osmId()) );
		case CN_NAME:
			{
				uint32_t pos = item.findKey(m_nameTagId);
				if (pos != liboscar::Static::OsmKeyValueObjectStore::Item::npos) {
					return QVariant( QString::fromUtf8(item.value(pos).c_str()) );
				}
				else {
					return QString("NONAME");
				}
			}
		case CN_TAGS: //Name-Column
			return QVariant( QString::fromUtf8(item.getAllStrings().c_str()) );
		case CN_GEOPOINTS:
			return QVariant( QString::fromStdString(item.getAllGeoPointsAsString() ) );
		default:
			return QVariant();
		}
	}
	return QVariant();
}

QVariant oscar_gui::ItemIndexTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
			case (CN_OSMID):
				return QVariant("OSMID");
			case (CN_NAME):
				return QVariant("Name");
			case (CN_TAGS):
				return QVariant("Tags");
			case (CN_GEOPOINTS):
				return QVariant("Coordinates");
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


void oscar_gui::ItemIndexTableModel::setIndex(const sserialize::ItemIndex& idx) {
	emit beginResetModel();
	m_idx = idx;
	emit endResetModel();
}

void oscar_gui::ItemIndexTableModel::setStore(const liboscar::Static::OsmKeyValueObjectStore& store) {
	emit beginResetModel();
	m_store  = store;
	m_nameTagId = m_store.keyStringTable().find("name");
	emit endResetModel();
}

