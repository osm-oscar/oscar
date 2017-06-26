#ifndef OSCAR_GUI_GEOMETRY_INPUT_WIDGET_H
#define OSCAR_GUI_GEOMETRY_INPUT_WIDGET_H
#include <QtGui/QWidget>

#include <liboscar/StaticOsmCompleter.h>

#include "States.h"

class QTableView;

namespace oscar_gui {

class SearchGeometryModel;

class GeometryInputWidget: public QWidget {
Q_OBJECT
public:
	GeometryInputWidget(const States & states);
	virtual ~GeometryInputWidget();
signals:
	void zoomTo(const Marble::GeoDataLatLonBox & bbox);
private slots:
	void showGeometryClicked(int p);
	void showTrianglesClicked(int p);
	void showCellsClicked(int p);
	void geometryClicked(int p);
private:
	QTableView * m_tbl;
	SearchGeometryModel * m_sgm;
	std::shared_ptr<SearchGeometryState> m_sgs;
	
};

}//end namespace oscar_gui

#endif