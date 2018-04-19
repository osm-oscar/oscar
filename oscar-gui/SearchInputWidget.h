#ifndef OSCAR_GUI_SEARCH_INPUT_WIDGET_H
#define OSCAR_GUI_SEARCH_INPUT_WIDGET_H
#include <QWidget>

#include "States.h"

class QPushButton;
class QLineEdit;

namespace oscar_gui {

class SearchInputWidget: public QWidget {
Q_OBJECT
public:
	explicit SearchInputWidget();
	virtual ~SearchInputWidget();
public slots:
	void setSearchText(const QString &);
signals:
	void searchTextChanged(const QString&);
private slots:
	void goButtonPressed();
	void clearButtonPressed();
private:
	QPushButton * m_goButton;
	QPushButton * m_clearButton;
	QLineEdit * m_search;
};

}//end namespace oscar_gui

#endif
