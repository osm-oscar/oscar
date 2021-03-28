#include "GeometryInputWidget.h"

#include <QTableView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>

#include "SearchGeometryModel.h"

namespace oscar_gui {

GeometryInputWidget::GeometryInputWidget(const States& states):
QWidget(),
m_sgs(states.sgs)
{
	m_tbl = new QTableView();
	m_sgm = new SearchGeometryModel(m_sgs);
	
	m_tbl->setModel(m_sgm);
	m_tbl->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
	m_tbl->horizontalHeader()->setSectionsClickable(true);
	m_tbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_tbl->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	
	connect(m_tbl, &QTableView::doubleClicked, m_sgm, &SearchGeometryModel::doubleClicked);
	connect(m_tbl, &QTableView::clicked, m_sgm, &SearchGeometryModel::clicked);
	
	connect(m_sgm, &SearchGeometryModel::showGeometryClicked, this, &GeometryInputWidget::showGeometryClicked);
	connect(m_sgm, &SearchGeometryModel::showTrianglesClicked, this, &GeometryInputWidget::showTrianglesClicked);
	connect(m_sgm, &SearchGeometryModel::showCellsClicked, this, &GeometryInputWidget::showCellsClicked);
	
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
	
	connect(m_sgs.get(), SIGNAL(dataChanged(int)), this, SLOT(dataChanged()));
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

void GeometryInputWidget::dataChanged() {
	this->update();
}

}//end namespace oscar_gui
