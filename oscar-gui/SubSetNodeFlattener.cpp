#include "SubSetNodeFlattener.h"
#include <sserialize/utility/printers.h>
#include <sserialize/stats/TimeMeasuerer.h>

namespace oscar_gui {

void SubSetNodeFlattenerCommunicator::completedSignal(const sserialize::ItemIndex& resultIdx, uint64_t runId, qlonglong timeInUsecs) {
	emit completed(resultIdx, runId, timeInUsecs);
}

SubSetNodeFlattener::~SubSetNodeFlattener() {
	delete m_com;
}

void SubSetNodeFlattener::run() {
	sserialize::TimeMeasurer tm;
	tm.begin();
	sserialize::ItemIndex idx( m_subSet->idx(m_selectedNode) );
	tm.end();
	m_com->completedSignal(idx, m_runId, tm.elapsedUseconds());
}

}//end namespace