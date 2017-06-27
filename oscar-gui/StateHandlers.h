#ifndef OSCAR_GUI_STATE_HANDLERS_H
#define OSCAR_GUI_STATE_HANDLERS_H
#include "States.h"
#include "SearchGeometryHelper.h"

namespace oscar_gui {

class SearchGeometryStateHandler: QObject {
Q_OBJECT
public:
	SearchGeometryStateHandler(const States & states);
	~SearchGeometryStateHandler();
public slots:
	void dataChanged(int p);
private:
	std::shared_ptr<SearchGeometryState> m_sgs;
	SearchGeometryHelper m_sgh;
};

struct StateHandlers {
	std::shared_ptr<SearchGeometryStateHandler> sgsh;

	StateHandlers(const States & states);
};

}//end namespace oscar_gui


#endif