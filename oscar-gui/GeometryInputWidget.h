#ifndef OSCAR_GUI_GEOMETRY_INPUT_WIDGET_H
#define OSCAR_GUI_GEOMETRY_INPUT_WIDGET_H
#include <QtGui/QWidget>

namespace oscar_gui {

class GeometryInputWidget: public QWidget {
Q_OBJECT
public:
	explicit GeometryInputWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~GeometryInputWidget();
};

}//end namespace oscar_gui

#endif