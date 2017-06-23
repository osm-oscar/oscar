#ifndef OSCAR_GUI_SEARCH_RESULTS_WIDGET_H
#define OSCAR_GUI_SEARCH_RESULTS_WIDGET_H
#include <QtGui/QWidget>

namespace oscar_gui {

class SearchResultsWidget: public QWidget {
Q_OBJECT
public:
	explicit SearchResultsWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~SearchResultsWidget();
};

}//end namespace oscar_gui

#endif