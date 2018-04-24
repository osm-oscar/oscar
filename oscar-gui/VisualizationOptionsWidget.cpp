#include "VisualizationOptionsWidget.h"
#include "MarbleMap.h"

#include <QComboBox>
#include <QVBoxLayout>
#include <QSlider>
#include <QLabel>

namespace oscar_gui {
	
void addWidgetWithLabel(QLayout * layout, QWidget * widget, const QString & label) {
	QLabel * l = new QLabel(label, widget);
	l->setBuddy(widget);
	layout->addWidget(l);
	layout->addWidget(widget);
}

VisualizationOptionsWidget::VisualizationOptionsWidget(const States & states) :
QWidget(),
m_dspCqrCells(new QComboBox()),
m_colorScheme(new QComboBox()),
m_cellOpacity(new QSlider(Qt::Orientation::Horizontal))
{
	connect(m_dspCqrCells, SIGNAL(currentIndexChanged(int)), this, SLOT(ps_displayCqrCells(int)));
	connect(m_colorScheme, SIGNAL(currentIndexChanged(int)), this, SLOT(ps_colorSchemeChanged(int)));
	connect(m_cellOpacity, &QSlider::valueChanged, this, &VisualizationOptionsWidget::cellOpacityChanged);
	
	m_dspCqrCells->addItem("No cqr cells", QVariant(false));
	m_dspCqrCells->addItem("Dispay cqr cells", QVariant(true));
	m_dspCqrCells->setCurrentIndex(0);
	
	m_colorScheme->addItem("Neighbors different", QVariant(MarbleMap::CS_DIFFERENT));
	m_colorScheme->addItem("All Same", QVariant(MarbleMap::CS_SAME)); 
	m_colorScheme->setCurrentIndex(0);
	
	m_cellOpacity->setMaximum(255);
	m_cellOpacity->setMinimum(50);
	m_cellOpacity->setValue(255);
	m_cellOpacity->setTracking(false);
	
	QVBoxLayout * mainLayout = new QVBoxLayout();
	addWidgetWithLabel(mainLayout, m_dspCqrCells, "Display cqr cells:");
	addWidgetWithLabel(mainLayout, m_colorScheme, "Cell color scheme: ");
	addWidgetWithLabel(mainLayout, m_cellOpacity, "Cell opacity: ");
	this->setLayout(mainLayout);
}

VisualizationOptionsWidget::~VisualizationOptionsWidget() {}

void VisualizationOptionsWidget::ps_displayCqrCells(int) {
	emit displayCqrCells( m_dspCqrCells->currentData().toBool() );
}

void VisualizationOptionsWidget::ps_colorSchemeChanged(int) {
	emit colorSchemeChanged( m_colorScheme->currentData().toInt() );
}

}//end namespace oscar_gui
