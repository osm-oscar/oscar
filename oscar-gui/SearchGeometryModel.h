#ifndef OSCAR_GUI_SEARCH_GEOMETRY_MODEL_H
#define OSCAR_GUI_SEARCH_GEOMETRY_MODEL_H

#include "States.h"

#include <QAbstractItemModel>

namespace oscar_gui {

class SearchGeometryModel: public QAbstractItemModel {
public:
	typedef enum {CD_NAME=0, CD_TYPE=1, CD_ACTIVE=2, __CD_COUNT=3} ColDefs;
	static constexpr int GEOMETRY = Qt::UserRole;
public:
	SearchGeometryModel(const std::shared_ptr<SearchGeometryState> & sgs);
	virtual ~SearchGeometryModel();
public:
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex& child) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
private:
	std::shared_ptr<SearchGeometryState> m_sgs;
};


};


#endif