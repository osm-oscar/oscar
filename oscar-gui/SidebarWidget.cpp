#include "SidebarWidget.h"

#include <QtGui/QTabWidget>
#include <QtGui/QHBoxLayout>

#include "SearchInputWidget.h"
#include "GeometryInputWidget.h"
#include "VisualizationOptionsWidget.h"
#include "SearchResultsWidget.h"
#include "ItemDetailsWidget.h"

namespace oscar_gui {

SidebarWidget::SidebarWidget(const States & states) :
m_tabs(new QTabWidget()),
m_si(new SearchInputWidget()),
m_sr(new SearchResultsWidget()),
m_id(new ItemDetailsWidget()),
m_gi(new GeometryInputWidget(states)),
m_vo(new VisualizationOptionsWidget())
{
	connect(this, SIGNAL(searchTextChanged(QString)), m_si, SIGNAL(searchTextChanged(QString)));

	QHBoxLayout * mainLayout = new QHBoxLayout();
	mainLayout->addWidget(m_tabs);
	m_tabs->addTab(m_si, "Search");
	m_tabs->addTab(m_sr, "Results");
	m_tabs->addTab(m_id, "Details");
	m_tabs->addTab(m_gi, "Geometry");
	m_tabs->addTab(m_vo, "Options");
	this->setLayout(mainLayout);
}


SidebarWidget::~SidebarWidget() {}

void SidebarWidget::setSearchText(const QString & s) {
	m_si->setSearchText(s);
}

} //end namespace oscar_gui