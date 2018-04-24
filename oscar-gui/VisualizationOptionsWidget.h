#ifndef OSCAR_GUI_VISUALIZATION_OPTIONS_WIDGET_H
#define OSCAR_GUI_VISUALIZATION_OPTIONS_WIDGET_H
#include <QWidget>

#include "States.h"

class QComboBox;
class QSlider;

namespace oscar_gui {

class VisualizationOptionsWidget: public QWidget {
Q_OBJECT
public:
	explicit VisualizationOptionsWidget(const States & states);
	virtual ~VisualizationOptionsWidget();
signals:
	void displayCqrCells(bool enabled);
	void colorSchemeChanged(int scheme);
	void cellOpacityChanged(int value);
private slots:
	void ps_displayCqrCells(int index);
	void ps_colorSchemeChanged(int index);
private:
	QComboBox * m_dspCqrCells;
	QComboBox * m_colorScheme;
	QSlider * m_cellOpacity;

};

}//end namespace oscar_gui

#endif
