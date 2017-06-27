#ifndef OSCAR_GUI_GEOMETRY_INPUT_WIDGET_H
#define OSCAR_GUI_GEOMETRY_INPUT_WIDGET_H
#include <QtGui/QWidget>

#include <liboscar/StaticOsmCompleter.h>

#include "States.h"

class QTableView;
class QLineEdit;
class QPushButton;

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
	void dataChanged();
private:
	QTableView * m_tbl;
	SearchGeometryModel * m_sgm;
	QLineEdit * m_sgt; //search geometry text input
	QLineEdit * m_sgtr; //search geometry text input result
	QPushButton * m_sgtb; //search geometry text accept button
	std::shared_ptr<SearchGeometryState> m_sgs;
	
};

}//end namespace oscar_gui

#endif