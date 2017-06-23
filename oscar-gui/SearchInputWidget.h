#ifndef OSCAR_GUI_SEARCH_INPUT_WIDGET_H
#define OSCAR_GUI_SEARCH_INPUT_WIDGET_H
#include <QtGui/QWidget>

class QLineEdit;

namespace oscar_gui {

class SearchInputWidget: public QWidget {
Q_OBJECT
public:
	explicit SearchInputWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~SearchInputWidget();
public slots:
	void setSearchText(const QString &);
signals:
	void searchTextChanged(const QString&);
private:
	QLineEdit * m_search;
};

}//end namespace oscar_gui

#endif