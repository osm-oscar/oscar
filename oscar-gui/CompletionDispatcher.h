#ifndef OSCAR_GUI_COMPLETION_DISPATCHER_H
#define OSCAR_GUI_COMPLETION_DISPATCHER_H
#include <QObject>
#include <liboscar/StaticOsmItemSet.h>
#include <liboscar/CompleterStaticOsm.h>
#include <liboscar/StaticOsmCompleter.h>

namespace oscar_gui {

class CompletionDispatcher: public QObject {
Q_OBJECT
public:
	struct CompletionStats {
		CompletionStats() : size(0), timeInUsec(0), cellCount(0) {}
		unsigned int size;
		long timeInUsec;
		unsigned int cellCount;
	};
	typedef enum {CC_COMPLEX=0, CC_SIMPLE=1} CompletionComplexity;
private:
	std::shared_ptr< liboscar::Static::OsmCompleter > m_completer;
	liboscar::TextSearch::Type m_textSearchType;
	std::shared_ptr<sserialize::Static::spatial::GeoHierarchy::SubSet> m_subSet;
	bool m_sparseSubSet;
	CompletionStats m_lastStats;
public:
	CompletionDispatcher( QObject* parent) : QObject(parent), m_textSearchType(liboscar::TextSearch::INVALID) ,m_sparseSubSet(false) {}
	virtual ~CompletionDispatcher() {}
	void setCompleter(const std::shared_ptr< liboscar::Static::OsmCompleter >& cmp);
	void setTextSearchType(liboscar::TextSearch::Type t);
public slots:
	void completionTextUpdated(const QString & text) ;
	void cursorPositionChanged(int oldPos, int newPos);
	void setSubSetType(bool sparseSubSet);
signals:
	void subSetUpdated(const std::shared_ptr<sserialize::Static::spatial::GeoHierarchy::SubSet> & subSet);
	void indexUpdated(const sserialize::ItemIndex & idx);
	void statsUpdated(const oscar_gui::CompletionDispatcher::CompletionStats & stats);
};

}//end namespace

#endif