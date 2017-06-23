#ifndef OSCAR_GUI_SIDEBAR_WIDGET_H
#define OSCAR_GUI_SIDEBAR_WIDGET_H
#include <QtGui/QWidget>

class QTabWidget;

namespace oscar_gui {

class SearchInputWidget;
class GeometryInputWidget;
class VisualizationOptionsWidget;
class SearchResultsWidget;
class ItemDetailsWidget;

class SidebarWidget: public QWidget {
Q_OBJECT
public:
	explicit SidebarWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~SidebarWidget();
public slots:
	void setSearchText(const QString &);
signals:
	void searchTextChanged(const QString &);
private:
	QTabWidget * m_tabs;
	SearchInputWidget * m_si;
	SearchResultsWidget * m_sr;
	ItemDetailsWidget * m_id;
	GeometryInputWidget * m_gi;
	VisualizationOptionsWidget * m_vo;
};

} //end namespace oscar_gui

#endif