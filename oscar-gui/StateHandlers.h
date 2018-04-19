#ifndef OSCAR_GUI_STATE_HANDLERS_H
#define OSCAR_GUI_STATE_HANDLERS_H
#include "States.h"
#include "SearchGeometryHelper.h"

namespace oscar_gui {
	
class SearchStateHandler: LockableState {
Q_OBJECT
public:
	SearchStateHandler(const States & states);
	virtual ~SearchStateHandler();
public slots:
	void searchTextChanged(const QString & queryString);
signals:
	void searchResultsChanged(const QString & queryString, const sserialize::ItemIndex & items);
private slots:
	void computeResult();
	void computeResult2(const QString & queryString);
	void computationCompleted(const QString & queryString, const sserialize::ItemIndex & items);
private:
	std::shared_ptr<TextSearchState> m_tss;
	std::shared_ptr<ResultListState> m_rls;
	std::shared_ptr<liboscar::Static::OsmCompleter> m_cmp;
	bool m_pendingUpdate;
	QString m_qs;
};

class SearchGeometryStateHandler: QObject {
Q_OBJECT
public:
	SearchGeometryStateHandler(const States & states);
	virtual ~SearchGeometryStateHandler();
public slots:
	void dataChanged(int p);
private:
	std::shared_ptr<SearchGeometryState> m_sgs;
	SearchGeometryHelper m_sgh;
};

struct StateHandlers {
	std::shared_ptr<SearchGeometryStateHandler> sgsh;
	std::shared_ptr<SearchStateHandler> ssh;

	StateHandlers(const States & states);
};

}//end namespace oscar_gui


#endif
