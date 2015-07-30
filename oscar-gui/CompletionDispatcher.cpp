#include "CompletionDispatcher.h"
#include <sserialize/stats/TimeMeasuerer.h>
#include <liboscar/StaticOsmCompleter.h>

namespace oscar_gui {

void CompletionDispatcher::setCompleter(const std::shared_ptr< liboscar::Static::OsmCompleter >& cmp) {
	m_completer = cmp;
}

void CompletionDispatcher::setTextSearchType(liboscar::TextSearch::Type t) {
	m_textSearchType = t;
}


void CompletionDispatcher::setSubSetType(bool sparseSubSet) {
	m_sparseSubSet = sparseSubSet;
}

void CompletionDispatcher::completionTextUpdated(const QString & text) {

	sserialize::TimeMeasurer m;
	QByteArray utf8QStr = text.toUtf8();
	std::string qstr; qstr.resize(utf8QStr.size());
	for(int i = 0; i < utf8QStr.size(); i++) {
		qstr[i] = utf8QStr.at(i);
	}
	m.begin();
	if (m_textSearchType == liboscar::TextSearch::GEOCELL && m_completer->textSearch().hasSearch(liboscar::TextSearch::GEOCELL) ) {
		m.begin();
		m_subSet = std::make_shared<sserialize::Static::spatial::GeoHierarchy::SubSet>(m_completer->clusteredComplete(qstr, m_sparseSubSet));
		m.end();
		m_lastStats.size = (m_subSet->root().priv() ? m_subSet->root()->maxItemsSize() : 0);
		m_lastStats.timeInUsec = m.elapsedTime();
		m_lastStats.cellCount = m_subSet->cqr().cellCount();
		emit statsUpdated( m_lastStats );
		emit subSetUpdated(m_subSet);
	}
	else if (m_textSearchType == liboscar::TextSearch::ITEMS && m_completer->textSearch().hasSearch(liboscar::TextSearch::ITEMS)) {
		m.begin();
		sserialize::ItemIndex idx = m_completer->complete(qstr).index();
		m.end();
		m_lastStats.size = idx.size();
		m_lastStats.timeInUsec = m.elapsedTime();
		m_lastStats.cellCount = 0;
		emit statsUpdated( m_lastStats );
		emit indexUpdated(idx);
	}
	else {
		std::cerr << "The currently slected TextSearchType is unsupported" << std::endl;
	}
}

void CompletionDispatcher::cursorPositionChanged(int /*oldPos*/, int /*newPos*/) {}

}//end namespace