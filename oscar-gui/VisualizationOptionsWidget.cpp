#include "VisualizationOptionsWidget.h"

#include <QComboBox>
#include <QVBoxLayout>

namespace oscar_gui {

VisualizationOptionsWidget::VisualizationOptionsWidget(const States & states) :
QWidget(),
m_dspCqrCells(new QComboBox())
{
	connect(m_dspCqrCells, SIGNAL(currentIndexChanged(int)), this, SLOT(dspCqrCellsChanged(int)));
	
	m_dspCqrCells->addItem("No cqr cells", QVariant(false));
	m_dspCqrCells->addItem("Dispay cqr cells", QVariant(true));
	
	
	QVBoxLayout * mainLayout = new QVBoxLayout();
	mainLayout->addWidget(m_dspCqrCells);
	this->setLayout(mainLayout);
}

VisualizationOptionsWidget::~VisualizationOptionsWidget() {}

void VisualizationOptionsWidget::dspCqrCellsChanged(int) {
	emit displayCqrCells( m_dspCqrCells->currentData().toBool() );
}

}//end namespace oscar_gui
