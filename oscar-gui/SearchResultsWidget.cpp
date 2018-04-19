#include "SearchResultsWidget.h"
#include "ResultsTableModel.h"

#include <QTableView>
#include <QHeaderView>

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
	
	connect(m_rtbl, SIGNAL(doubleClicked(QModelIndex)), m_rmdl, SLOT(doubleClicked(QModelIndex)));
	connect(m_rtbl, SIGNAL(clicked(QModelIndex)), m_rmdl, SLOT(clicked(QModelIndex)));
}

SearchResultsWidget::~SearchResultsWidget() {}


}//end namespace oscar_gui
