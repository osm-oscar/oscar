#include "MainWindow.h"

#include <QHBoxLayout>
#include <QDockWidget>

#include "MarbleMap.h"
#include "SidebarWidget.h"
#include "VisualizationOptionsWidget.h"

namespace oscar_gui {
	
MainWindow::MainWindow(const std::shared_ptr<liboscar::Static::OsmCompleter> & cmp) :
m_states(cmp),
m_stateHandlers(m_states)
{
	m_map = new MarbleMap(cmp->store(), m_states);
	m_sidebar = new SidebarWidget(m_states);
	
	connect(this, &MainWindow::triangleAdded, m_map, &MarbleMap::addTriangle);
	connect(this, &MainWindow::triangleRemoved, m_map, &MarbleMap::removeTriangle);
	
	connect(this, &MainWindow::cellAdded, m_map, &MarbleMap::addCell);
	connect(this, &MainWindow::cellRemoved, m_map, &MarbleMap::removeCell);
	connect(m_sidebar->vo(), &VisualizationOptionsWidget::displayCqrCells, m_map, &MarbleMap::displayCqrCells);
	connect(m_sidebar->vo(), &VisualizationOptionsWidget::colorSchemeChanged, m_map, &MarbleMap::setColorScheme);
	connect(m_sidebar->vo(), &VisualizationOptionsWidget::cellOpacityChanged, m_map, &MarbleMap::setCellOpacity);
	connect(m_stateHandlers.ssh.get(), &SearchStateHandler::searchResultsChanged, this, &MainWindow::searchResultsChanged);
	
	QHBoxLayout * mainLayout = new QHBoxLayout();
	QDockWidget * dock = new QDockWidget("Sidebar", this);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dock->setWidget(m_sidebar);
	addDockWidget(Qt::LeftDockWidgetArea, dock);
	mainLayout->addWidget(m_map);
	
	//set the central widget
	QWidget * centralWidget = new QWidget(this);
	centralWidget->setLayout(mainLayout);
	setCentralWidget(centralWidget);
	
	m_sidebar->focus();
}


void MainWindow::searchResultsChanged(const QString &, const sserialize::CellQueryResult & cqr, const sserialize::ItemIndex &) {
	m_map->zoomTo( cqr.boundary() );
}

MainWindow::~MainWindow() {}

}//end namespace oscar_gui
