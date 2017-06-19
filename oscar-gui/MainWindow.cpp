#include "MainWindow.h"

#include <QtGui/QHBoxLayout>

#include "MarbleMap.h"

namespace oscar_gui {

MainWindow::MainWindow(const liboscar::Static::OsmCompleter& cmp) :
m_completer(cmp)
{
	m_map = new MarbleMap(cmp.store());
	
	connect(this, SIGNAL(triangleAdded(uint32_t)), m_map, SLOT(addTriangle(uint32_t)));
	connect(this, SIGNAL(triangleRemoved(uint32_t)), m_map, SLOT(removeTriangle(uint32_t)));
	
	connect(this, SIGNAL(cellAdded(uint32_t)), m_map, SLOT(addCell(uint32_t)));
	connect(this, SIGNAL(cellRemoved(uint32_t)), m_map, SLOT(removeCell(uint32_t)));
	
	QHBoxLayout * mainLayout = new QHBoxLayout(this);
	mainLayout->addWidget(m_map);
	
	//set the central widget
	QWidget * centralWidget = new QWidget(this);
	centralWidget->setLayout(mainLayout);
	setCentralWidget(centralWidget);
}

void MainWindow::changeColorScheme(int index) {
	m_map->setColorScheme(index);
}


}//end namespace oscar_gui