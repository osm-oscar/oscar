#include "SubSetNodeFlattener.h"
#include <sserialize/utility/printers.h>
#include <sserialize/stats/TimeMeasuerer.h>

namespace oscar_gui {

void SubSetNodeFlattenerCommunicator::completedSignal(const sserialize::ItemIndex& resultIdx, const sserialize::ItemIndex & cells, uint64_t runId, qlonglong timeInUsecs) {
	emit completed(resultIdx, cells, runId, timeInUsecs);
}

SubSetNodeFlattener::~SubSetNodeFlattener() {
	delete m_com;
}

void SubSetNodeFlattener::run() {
	sserialize::TimeMeasurer tm;
	tm.begin();
	sserialize::ItemIndex idx( m_subSet->idx(m_selectedNode) );
	sserialize::ItemIndex cells( m_subSet->cells(m_selectedNode) );
	tm.end();
	m_com->completedSignal(idx, cells, m_runId, tm.elapsedUseconds());
}

}//end namespace