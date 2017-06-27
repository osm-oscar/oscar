#include "GeometryInputWidget.h"

#include <QtGui/QTableView>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>

#include "SearchGeometryModel.h"

namespace oscar_gui {

GeometryInputWidget::GeometryInputWidget(const States& states):
QWidget(),
m_sgs(states.sgs)
{
	m_tbl = new QTableView();
	m_sgm = new SearchGeometryModel(m_sgs);
	
	m_tbl->setModel(m_sgm);
	m_tbl->horizontalHeader()->setResizeMode(QHeaderView::ResizeMode::ResizeToContents);
	m_tbl->horizontalHeader()->setClickable(true);
	m_tbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_tbl->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	
	connect(m_tbl, SIGNAL(doubleClicked(QModelIndex)), m_sgm, SLOT(doubleClicked(QModelIndex)));
	connect(m_tbl, SIGNAL(clicked(QModelIndex)), m_sgm, SLOT(clicked(QModelIndex)));
	
	connect(m_sgm, SIGNAL(showGeometryClicked(int)), this, SLOT(showGeometryClicked(int)));
	connect(m_sgm, SIGNAL(showTrianglesClicked(int)), this, SLOT(showTrianglesClicked(int)));
	connect(m_sgm, SIGNAL(showCellsClicked(int)), this, SLOT(showCellsClicked(int)));
	
	m_sgt = new QLineEdit();
	m_sgt->setPlaceholderText("Search query to create geometry");
	m_sgtr = new QLineEdit();
	m_sgtr->setPlaceholderText("Geometry created from search query");
	m_sgtb = new QPushButton("Accept");
	
	QVBoxLayout * sgtLayout = new QVBoxLayout();
	sgtLayout->addWidget(m_sgt);
	sgtLayout->addWidget(m_sgtr);
	sgtLayout->addWidget(m_sgtb);
	QWidget * sgtWidget = new QWidget();
	sgtWidget->setLayout(sgtLayout);
	
	QVBoxLayout * mainLayout = new QVBoxLayout();
	mainLayout->addWidget(sgtWidget);
	mainLayout->addWidget(m_tbl);
	this->setLayout(mainLayout);
}

GeometryInputWidget::~GeometryInputWidget() {}

void GeometryInputWidget::geometryClicked(int p) {
	emit zoomTo( m_sgs->data(p).latLonAltBox() );
}

void GeometryInputWidget::showCellsClicked(int p) {
	m_sgs->toggle(p, SearchGeometryState::AT_CELLS);
}

void GeometryInputWidget::showGeometryClicked(int p) {
	m_sgs->toggle(p, SearchGeometryState::AT_SHOW);
}

void GeometryInputWidget::showTrianglesClicked(int p) {
	m_sgs->toggle(p, SearchGeometryState::AT_TRIANGLES);
}


}//end namespace oscar_gui