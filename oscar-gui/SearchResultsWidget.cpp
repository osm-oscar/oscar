#include "SearchResultsWidget.h"
#include "ResultsTableModel.h"

#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>

namespace oscar_gui {

SearchResultsWidget::SearchResultsWidget(const States & states) :
QWidget(),
m_rtbl(new QTableView()),
m_rmdl(new ResultsTableModel(states))
{
	m_rtbl->setModel(m_rmdl);
	m_rtbl->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
	m_rtbl->horizontalHeader()->setSectionsClickable(true);
	m_rtbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_rtbl->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	connect(m_rtbl, &QTableView::doubleClicked, m_rmdl, &ResultsTableModel::doubleClicked);
	connect(m_rtbl, &QTableView::clicked, m_rmdl,  &ResultsTableModel::clicked);

	QVBoxLayout * mainLayout = new QVBoxLayout();
	mainLayout->addWidget(m_rtbl);
	this->setLayout(mainLayout);
}

SearchResultsWidget::~SearchResultsWidget() {}


}//end namespace oscar_gui
