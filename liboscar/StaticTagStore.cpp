#include "StaticTagStore.h"
#include <sserialize/Static/TreeNode.h>

namespace liboscar {
namespace Static {

TagStore::TagStore() {}

TagStore::TagStore(const sserialize::UByteArrayAdapter& data, const sserialize::Static::ItemIndexStore& index) :
m_nodeIndex(data+5),
m_indexStore(index)
{
	m_nodes = sserialize::UByteArrayAdapter(data, 5 + m_nodeIndex.getSizeInBytes(), data.getUint32(1));
}

TagStore::~TagStore() {}

TagStore::Node TagStore::getNodeById(uint16_t nodeId) const {
	if (m_nodeIndex.size() <= nodeId)
		return Node();
	else
		return Node(m_nodes + m_nodeIndex.at(nodeId));
}


TagStore::Node TagStore::at(uint16_t nodeId) const {
	if (nodeId >= size())
		return Node();
	return getNodeById(nodeId);
}

TagStore::Node TagStore::at(const std::vector< uint16_t >& treePath) const {
	uint16_t curChildPos;
	Node curNode(getNodeById(0));
	for(size_t i = 0; i < treePath.size(); i++) {
		curChildPos = treePath[i];
		if (curChildPos >= curNode.children().size())
			return Node();
		curNode = getNodeById(curNode.children().atPosition(curChildPos).second());
	}
	return curNode;
}

TagStore::Node TagStore::at(const std::vector< std::string >& treePath) const {
	Node curNode(getNodeById(0));
	for(size_t i = 0; i < treePath.size(); i++) {
		int32_t curChildPos = curNode.children().findPosition(treePath[i]);
		if (curChildPos < 0)
			return Node();
		curNode = getNodeById(curNode.children().atPosition(curChildPos).second());
	}
	return curNode;
}

sserialize::ItemIndex TagStore::complete(const std::vector< uint16_t >& treePath) const {
	Node curNode = at(treePath);
	if (!curNode.valid())
		return sserialize::ItemIndex();
	uint32_t indexId = curNode.getIndexPtr();
	return  m_indexStore.at(indexId);
}

sserialize::ItemIndex TagStore::complete(const std::vector< std::string >& treePath) const {
	Node curNode = at(treePath);
	if (!curNode.valid())
		return sserialize::ItemIndex();
	uint32_t indexId = curNode.getIndexPtr();
	return m_indexStore.at(indexId);
}

sserialize::ItemIndex TagStore::complete(const uint16_t nodeId) const {
	if (nodeId >= m_nodeIndex.size())
		return sserialize::ItemIndex();
	uint32_t indexId = getNodeById(nodeId).getIndexPtr();
	return m_indexStore.at(indexId);
}

std::vector< uint16_t > TagStore::ancestors(uint16_t nodeId) const {
	if (nodeId >= m_nodeIndex.size() || nodeId == 0)
		return std::vector< uint16_t >();
	Node curNode(getNodeById(nodeId));
	
	std::vector< uint16_t > treePath;
	while (curNode.parentId() != 0) {
		treePath.push_back(curNode.posInParent());
		curNode = getNodeById(curNode.parentId());
	}
	treePath.push_back(curNode.posInParent());
	std::reverse(treePath.begin(), treePath.end());
	return treePath;
}

std::vector< std::string > TagStore::ancestorNames(uint16_t nodeId) const {
	if (nodeId >= m_nodeIndex.size() || nodeId == 0)
		return std::vector< std::string >();
	Node curNode(getNodeById(nodeId));
	uint16_t curNodeId = nodeId;
	Node parent;
	
	std::vector< std::string > treePath;
	uint16_t posInParent;
	while (curNodeId != 0) {
		posInParent = curNode.posInParent();
		curNodeId = curNode.parentId();
		curNode = getNodeById(curNodeId);
		treePath.push_back(curNode.children().atPosition(posInParent).first());
	}
	std::reverse(treePath.begin(), treePath.end());
	return treePath;
}

std::string TagStore::nodeString(const uint16_t nodeId) {
	if (nodeId >= m_nodeIndex.size())
		return std::string();
	Node node = getNodeById(nodeId);
	Node parent = getNodeById(node.parentId());
	return parent.children().atPosition(node.posInParent()).first();
}

bool TagStore::nodeId(const std::vector< std::string >& treePath, uint16_t & nodeId) const {
	uint16_t curNodeId = 0;
	Node curNode(getNodeById(curNodeId));
	for(size_t i = 0; i < treePath.size(); i++) {
		int32_t curChildPos = curNode.children().findPosition(treePath[i]);
		if (curChildPos < 0)
			return false;
		curNodeId = curNode.children().atPosition(curChildPos).second();
		curNode = getNodeById(curNodeId);
	}
	nodeId = curNodeId;
	return true;
}

std::ostream& TagStore::printStats(std::ostream& out) const {
	out << "Static::TagStore -- Stats -- BEGIN" << std::endl;
	out << "Node count: " << size() << std::endl;
	out << "Storage size: " << getSizeInBytes() << std::endl;
	out << "Static::TagStore -- Stats -- END" << std::endl;
	return out;
}

}}//end namespace