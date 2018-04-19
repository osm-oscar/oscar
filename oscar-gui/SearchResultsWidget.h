#ifndef OSCAR_GUI_SEARCH_RESULTS_WIDGET_H
#define OSCAR_GUI_SEARCH_RESULTS_WIDGET_H
#include <QWidget>

#include "States.h"

class QTableView;

namespace oscar_gui {
	
class ResultsTableModel;

class SearchResultsWidget: public QWidget {
Q_OBJECT
public:
	explicit SearchResultsWidget(const States & states);
	virtual ~SearchResultsWidget();
private:
	QTableView * m_rtbl;
	ResultsTableModel * m_rmdl;
};

}//end namespace oscar_gui

#endif
