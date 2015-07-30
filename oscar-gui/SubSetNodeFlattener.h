#ifndef OSCAR_GUI_SUB_SET_NODE_FLATTENER_H
#define OSCAR_GUI_SUB_SET_NODE_FLATTENER_H
#include <sserialize/Static/GeoHierarchy.h>
#include <QRunnable>
#include <QObject>
#include "SubSetModel.h"

namespace oscar_gui {

class SubSetNodeFlattenerCommunicator: public QObject {
	Q_OBJECT
public:
	SubSetNodeFlattenerCommunicator(QObject * parent) : QObject(parent) {}
	~SubSetNodeFlattenerCommunicator() {}
	void completedSignal(const sserialize::ItemIndex & resultIdx, uint64_t runId, qlonglong timeInUsecs);
Q_SIGNALS:
	void completed(const sserialize::ItemIndex & resultIdx, uint64_t runId, qlonglong timeInUsecs);
};

class SubSetNodeFlattener: public QRunnable {
public:
	typedef std::shared_ptr< sserialize::Static::spatial::GeoHierarchy::SubSet > SubSet;
	typedef sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr SubSetNodePtr;
private:
	SubSet m_subSet;
	SubSetNodePtr m_selectedNode;
	uint64_t m_runId;
	SubSetNodeFlattenerCommunicator * m_com;
public:
	SubSetNodeFlattener(const SubSet & subSet,  const SubSetNodePtr & selectedNode, uint64_t runId) :
	m_subSet(subSet),
	m_selectedNode(selectedNode),
	m_runId(runId),
	m_com(new SubSetNodeFlattenerCommunicator(0))
	{}
	virtual ~SubSetNodeFlattener();
	virtual void run() override;
	SubSetNodeFlattenerCommunicator * com() { return m_com; }
};

}//end namespace
#endif