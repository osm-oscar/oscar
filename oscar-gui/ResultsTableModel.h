#ifndef OSCAR_GUI_RESULTS_TABLE_MODEL_H
#define OSCAR_GUI_RESULTS_TABLE_MODEL_H

#include "States.h"

#include <QAbstractItemModel>

#include <liboscar/OsmKeyValueObjectStore.h>

namespace oscar_gui {

class ResultsTableModel: public QAbstractItemModel {
Q_OBJECT
public:
	typedef enum {CD_ID=0, CD_SHOW=1, CD_SHOW_CELLS=2, CD_SCORE=3, CD_NAME=4, __CD_COUNT=5} ColDefs;
	static constexpr int ITEMID = Qt::UserRole;
public:
	ResultsTableModel(const States & states);
	virtual ~ResultsTableModel();
public:
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex& child) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
public slots:
	void doubleClicked(const QModelIndex&);
	void clicked(const QModelIndex&);
signals:
	void toggleItem(uint32_t itemId, ItemGeometryState::ActiveType at);
	void zoomToItem(uint32_t itemId);
private slots:
	void resetModel();
private:
	liboscar::Static::OsmKeyValueObjectStore m_store;
	uint32_t m_nameStrId;
	std::shared_ptr<ItemGeometryState> m_igs;
	std::shared_ptr<ResultListState> m_rls;
};


};


#endif
