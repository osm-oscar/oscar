#ifndef OSCAR_GUI_ITEM_INDEX_TABLE_MODEL_H
#define OSCAR_GUI_ITEM_INDEX_TABLE_MODEL_H
#include <QAbstractTableModel>
#include <sserialize/spatial/GeoHierarchy.h>
#include <liboscar/OsmKeyValueObjectStore.h>
#include <sserialize/Static/ItemIndexStore.h>

namespace oscar_gui {

class ItemIndexTableModel: public QAbstractTableModel {
	Q_OBJECT
private:
	typedef enum { CN_OSMID=0, CN_NAME=1, CN_TAGS=2, CN_GEOPOINTS=3, CN_COL_COUNT=CN_TAGS+1} ColNames;
private:
	liboscar::Static::OsmKeyValueObjectStore m_store;
	uint32_t m_nameTagId;
	sserialize::ItemIndex m_idx;
	
public:
	ItemIndexTableModel(QObject * parent);
	virtual ~ItemIndexTableModel();
	virtual int rowCount(const QModelIndex&) const;
	virtual int columnCount(const QModelIndex&) const;
	virtual QVariant data(const QModelIndex & index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual Qt::ItemFlags flags(const QModelIndex& /*index*/) const {
		return (Qt::ItemIsEnabled);
	}
	void setStore(const liboscar::Static::OsmKeyValueObjectStore & store);
public slots:
	void setIndex(const sserialize::ItemIndex & idx);
};

}

#endif