#ifndef OSCAR_GUI_SEARCH_GEOMETRY_MODEL_H
#define OSCAR_GUI_SEARCH_GEOMETRY_MODEL_H

#include "States.h"

#include <QAbstractItemModel>

namespace oscar_gui {

class SearchGeometryModel: public QAbstractItemModel {
Q_OBJECT
public:
	typedef enum {CD_NAME=0, CD_TYPE=1, CD_SHOW=2, CD_SHOW_TRIANGLES=3, CD_SHOW_CELLS=4, __CD_COUNT=5} ColDefs;
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
public slots:
	void doubleClicked(const QModelIndex&);
	void clicked(const QModelIndex&);
signals:
	void showGeometryClicked(int p);
	void showTrianglesClicked(int p);
	void showCellsClicked(int p);
	void geometryClicked(int p);
	
private:
	std::shared_ptr<SearchGeometryState> m_sgs;
};


};


#endif