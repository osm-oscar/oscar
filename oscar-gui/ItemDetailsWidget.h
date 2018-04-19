#ifndef OSCAR_GUI_ITEM_DETAILS_WIDGET_H
#define OSCAR_GUI_ITEM_DETAILS_WIDGET_H
#include <QWidget>

#include "States.h"

namespace oscar_gui {

class ItemDetailsWidget: public QWidget {
Q_OBJECT
public:
	explicit ItemDetailsWidget(const States & states);
	virtual ~ItemDetailsWidget();
};

}//end namespace oscar_gui

#endif
