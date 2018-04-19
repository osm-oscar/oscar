#ifndef OSCAR_GUI_VISUALIZATION_OPTIONS_WIDGET_H
#define OSCAR_GUI_VISUALIZATION_OPTIONS_WIDGET_H
#include <QWidget>

#include "States.h"

namespace oscar_gui {

class VisualizationOptionsWidget: public QWidget {
Q_OBJECT
public:
	explicit VisualizationOptionsWidget(const States & states);
	virtual ~VisualizationOptionsWidget();
};

}//end namespace oscar_gui

#endif
