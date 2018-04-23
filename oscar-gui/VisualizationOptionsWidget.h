#ifndef OSCAR_GUI_VISUALIZATION_OPTIONS_WIDGET_H
#define OSCAR_GUI_VISUALIZATION_OPTIONS_WIDGET_H
#include <QWidget>

#include "States.h"

class QComboBox;

namespace oscar_gui {

class VisualizationOptionsWidget: public QWidget {
Q_OBJECT
public:
	explicit VisualizationOptionsWidget(const States & states);
	virtual ~VisualizationOptionsWidget();
signals:
	void displayCqrCells(bool enabled);
private slots:
	void dspCqrCellsChanged(int index);
private:
	QComboBox * m_dspCqrCells;
};

}//end namespace oscar_gui

#endif
