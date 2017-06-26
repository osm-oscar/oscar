#include "MainWindow.h"

#include <QtGui/QHBoxLayout>

#include "MarbleMap.h"
#include "SidebarWidget.h"

namespace oscar_gui {

MainWindow::MainWindow(const std::shared_ptr<liboscar::Static::OsmCompleter> & cmp) :
m_completer(cmp)
{
	m_map = new MarbleMap(cmp->store(), m_states);
	m_sidebar = new SidebarWidget();
	
	connect(this, SIGNAL(triangleAdded(uint32_t)), m_map, SLOT(addTriangle(uint32_t)));
	connect(this, SIGNAL(triangleRemoved(uint32_t)), m_map, SLOT(removeTriangle(uint32_t)));
	
	connect(this, SIGNAL(cellAdded(uint32_t)), m_map, SLOT(addCell(uint32_t)));
	connect(this, SIGNAL(cellRemoved(uint32_t)), m_map, SLOT(removeCell(uint32_t)));
	
	connect(m_map, SIGNAL(toggleCellClicked(uint32_t)), this, SLOT(toggleCell(uint32_t)));
	
	QHBoxLayout * mainLayout = new QHBoxLayout();
	mainLayout->addWidget(m_sidebar, 1);
	mainLayout->addWidget(m_map, 2);
	
	//set the central widget
	QWidget * centralWidget = new QWidget(this);
	centralWidget->setLayout(mainLayout);
	setCentralWidget(centralWidget);
}

MainWindow::~MainWindow() {}

void MainWindow::toggleCell(uint32_t cellId) {
	;
}

void MainWindow::changeColorScheme(int index) {
	m_map->setColorScheme(index);
}

}//end namespace oscar_gui