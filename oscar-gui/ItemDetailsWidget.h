#ifndef OSCAR_GUI_ITEM_DETAILS_WIDGET_H
#define OSCAR_GUI_ITEM_DETAILS_WIDGET_H
#include <QtGui/QWidget>

namespace oscar_gui {

class ItemDetailsWidget: public QWidget {
Q_OBJECT
public:
	explicit ItemDetailsWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
	virtual ~ItemDetailsWidget();
};

}//end namespace oscar_gui

#endif