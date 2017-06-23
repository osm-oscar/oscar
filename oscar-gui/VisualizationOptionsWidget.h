#ifndef OSCAR_GUI_VISUALIZATION_OPTIONS_WIDGET_H
#define OSCAR_GUI_VISUALIZATION_OPTIONS_WIDGET_H
#include <QtGui/QWidget>

namespace oscar_gui {

class VisualizationOptionsWidget: public QWidget {
Q_OBJECT
public:
	explicit VisualizationOptionsWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~VisualizationOptionsWidget();
};

}//end namespace oscar_gui

#endif