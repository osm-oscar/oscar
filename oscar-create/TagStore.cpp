#include "TagStore.h"
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/Static/Map.h>
#include <liboscar/StaticTagStore.h>
#include <sserialize/containers/ItemIndexFactory.h>

using namespace sserialize;

namespace oscar_create {

TagStore::TagStore() :
m_nodeCount(1),
m_root(new Node(0))
{
	m_nodePtrNodeIdMap[m_root] = 0;
	populateKeyToStore();
}

TagStore::~TagStore() {
	if (m_root)
		delete m_root;
}

void TagStore::clear() {
	if (m_root)
		delete m_root;
	m_root = new Node(0);
	m_nodeCount = 1;
	m_nodePtrNodeIdMap[m_root] = 0;
}


TagStore::Node* TagStore::at(const std::string& key, const std::string& value) {
	std::vector<std::string> treePath(createTreePath(key, value));
	return operator[](treePath);
}


void TagStore::populateKeyToStore() {
	m_keysToStore.insert("name");
	m_keysToStore.insert("amenity");
	m_keysToStore.insert("boundary");
	m_keysToStore.insert("building");
	m_keysToStore.insert("craft");
	m_keysToStore.insert("emergency");
	m_keysToStore.insert("geological");
	m_keysToStore.insert("highway");
	m_keysToStore.insert("historic");
	m_keysToStore.insert("landuse");
	m_keysToStore.insert("leisure");
	m_keysToStore.insert("man_made");
	m_keysToStore.insert("military");
	m_keysToStore.insert("natural");
	m_keysToStore.insert("office");
	m_keysToStore.insert("place");
	m_keysToStore.insert("public_transport");
	m_keysToStore.insert("railway");
	m_keysToStore.insert("route");
	m_keysToStore.insert("shop");
	m_keysToStore.insert("sport");
	m_keysToStore.insert("tourism");
	m_keysToStore.insert("waterway");

	m_keyAttributeStore["cycleway"] = std::string("highway");
	m_keyAttributeStore["sac_scale"] = std::string("highway");
	m_keyAttributeStore["surface"] = std::string("highway");
	m_keyAttributeStore["tracktype"] = std::string("highway");
	m_keyValAttributeStore["cuisine"] = std::pair<std::string, std::string>("amenity","restaurant");
	
}


TagStore::Node* TagStore::insert(const std::vector< std::string >& treePath, uint32_t itemId) {
	if (treePath.size() == 0) {
		std::cerr << "TagStore: Trying to add empty path with id " << itemId << std::endl;
	}
	Node * curNode = operator[](treePath);
	if (curNode)
		curNode->value().insert(itemId);
	return curNode;
}


TagStore::Node* TagStore::operator[](const std::vector< std::string >& treePath) {
	Node * curNode = m_root;
	for(size_t i = 0; i < treePath.size(); i++) {
		if (curNode->children().count(treePath[i]) == 0) {
			Node * newNode = new Node(curNode);
			curNode->children()[treePath[i]] = newNode;
			m_nodePtrNodeIdMap[newNode] = m_nodeCount;
			m_nodeCount++;
		}
		curNode = curNode->children().at(treePath[i]);
	}
	return curNode;
}

std::vector< std::string > TagStore::createTreePath(const std::string & key, const std::string & value) {
	std::vector<std::string> treePath;
	//chech which category this is, ie if this is a attribute
	if (m_keyValAttributeStore.count(key) > 0) {
		treePath.push_back(m_keyValAttributeStore[key].first);
		treePath.push_back(m_keyValAttributeStore[key].second);
	}
	else if (m_keyAttributeStore.count(key) > 0) {
		treePath.push_back(m_keyAttributeStore[key]);
	}
	if (key.size() > 0)
		treePath.push_back(key);
	if (value.size() > 0)
		treePath.push_back(value);
	return treePath;
}

uint32_t TagStore::insert(const std::string& key, const std::string& value) {
	if (key.size() == 0 || value.size() == 0) {
		std::cerr << "TagStore: Trying to add empty key/value" << std::endl;
	}
	Node * node = operator[](createTreePath(key, value));
	return m_nodePtrNodeIdMap.at(node);
}

uint32_t TagStore::insert(const std::string& key, const std::string& value, uint32_t itemId) {
	if (key.size() == 0) {
		std::cerr << "TagStore: Trying to add empty value with id " << itemId << std::endl;
	}
	else if (value.size() == 0) {
		std::cerr << "TagStore: Trying to add empty value with id " << itemId << std::endl;
	}
	Node * node = operator[](createTreePath(key, value));
	node->value().insert(itemId);
	return m_nodePtrNodeIdMap.at(node);
}

uint32_t TagStore::insert(const std::vector< std::string >& treePath) {
	if (treePath.size() == 0) {
		std::cerr << "TagStore: Trying to add empty path" << std::endl;
	}
	Node * node = operator[](treePath);
	return m_nodePtrNodeIdMap.at(node);
}

bool TagStore::saveTag(const std::string& key, const std::string& /*value*/) {
	return (m_keysToStore.count(key) > 0 || m_keyAttributeStore.count(key) > 0 || m_keyValAttributeStore.count(key) > 0);
}

/** deletes all nodes (if possible. ie. no children), if nodeId is in nodeIdsToDelete, modifys it accordingly */
bool TagStore::deleteNodes(Node * curNode, std::set<uint32_t> & nodeIdsToDelete) {
	if (curNode) {
		std::set<std::string> childrenToDelete;
		for(Node::ChildIterator it = curNode->children().begin(); it != curNode->children().end(); it++) {
			if (deleteNodes(it->second, nodeIdsToDelete)) {
				childrenToDelete.insert(it->first);
			}
		}
		
		for(std::set<std::string>::const_iterator it = childrenToDelete.begin(); it != childrenToDelete.end(); it++) {
			curNode->children().erase(*it);
		}
		
		uint32_t curNodeId = m_nodePtrNodeIdMap.count(curNode);
		if (nodeIdsToDelete.count(curNodeId) > 0) {
			if (curNode->children().size() == 0) {
				delete this;
				return true;
			}
			else {
				nodeIdsToDelete.erase(curNodeId);
			}
		}
	}
	return false;
}

void TagStore::getFullItemIndex(TagStore::Node* node, std::set< uint32_t >& nodeSet) const {
	if (node) {
		nodeSet.insert(node->value().begin(), node->value().end());
		for(Node::ConstChildIterator it = node->children().begin(); it != node->children().end(); it++) {
			getFullItemIndex(it->second, nodeSet);
		}
	}
}

bool TagStore::getNodesInLevelOrder(std::vector< oscar_create::TagStore::Node* >& nodesInLevelOrder) const {
	if (!m_root)
		return false;
	size_t i = nodesInLevelOrder.size();
	nodesInLevelOrder.push_back(m_root);
	while (i < nodesInLevelOrder.size()) {
		Node * curNode = nodesInLevelOrder[i];
		for(Node::ConstChildIterator it = curNode->children().begin(); it != curNode->children().end(); it++) {
			nodesInLevelOrder.push_back(it->second);
		}
		i++;
	}
	return true;
}

bool TagStore::equal(const liboscar::Static::TagStore & sTagStore) {
	std::vector< oscar_create::TagStore::Node* > nodes;
	getNodesInLevelOrder(nodes);
	std::map<oscar_create::TagStore::Node*, uint16_t> nodePtrToId;
	for(size_t i = 0; i < nodes.size(); i++) {
		nodePtrToId[nodes[i]] = i;
	}
	if (sTagStore.size() != nodes.size())
		return false;
	for(size_t i = 0; i< nodes.size(); i++) {
		oscar_create::TagStore::Node * node = nodes[i];
		liboscar::Static::TagStore::Node stNode( sTagStore.at(i) );
		if (stNode.children().size() != node->children().size())
			return false;
		else {
			uint32_t pos = 0;
			for(Node::ConstChildIterator it = node->children().begin(); it != node->children().end(); it++) {
				sserialize::Static::Pair<std::string, uint16_t> stPair(stNode.children().atPosition(pos));
				if (stPair.first() != it->first || stPair.second() != nodePtrToId.at(it->second)) {
					return false;
				}
				pos++;
			}
		}
		if (node->parent() == 0 && stNode.parentId() != 0)
			return false;
		if (node->parent() != 0 && nodePtrToId[node->parent()] != stNode.parentId())
			return false;

		std::set<uint32_t> nodeItems;
		getFullItemIndex(node, nodeItems);
		ItemIndex idx = sTagStore.complete(i);
		if (idx != nodeItems)
			return false;
	}
	return true;
}


bool TagStore::getNodesInLevelOrder(std::vector< oscar_create::TagStore::Node* >& nodesInLevelOrder, std::map< oscar_create::TagStore::Node*, oscar_create::TagStore::SerializationNodeInfo >& serInfo) const {
	if (!m_root)
		return false;
	nodesInLevelOrder.push_back(m_root);
	serInfo[m_root] = SerializationNodeInfo(0, 0);
	size_t i = 0;
	while (i < nodesInLevelOrder.size()) {
		Node * curNode = nodesInLevelOrder[i];
		uint32_t childCounter = 0;
		for(Node::ConstChildIterator it = curNode->children().begin(); it != curNode->children().end(); it++) {
			serInfo[it->second] = SerializationNodeInfo(nodesInLevelOrder.size(), childCounter);
			nodesInLevelOrder.push_back(it->second);
			childCounter++;
		}
		i++;
	}
	return true;
}

template<typename T, typename TCONTAINER>
std::unordered_map<T, uint32_t> unordered_mapTableFromLinearContainer(const TCONTAINER & s) {
	std::unordered_map<T, uint32_t> r;
	for(size_t i = 0; i < s.size(); ++i) {
		r[ s[i] ] = i;
	}
	return r;
}

std::unordered_map< TagStore::Node*, uint32_t > TagStore::enumerateNodes() {
	std::vector<Node*> nodeNums;
	getNodesInLevelOrder(nodeNums);
	return unordered_mapTableFromLinearContainer<Node*>(nodeNums);
}

bool TagStore::serialize(UByteArrayAdapter& destination, TagStore::Node * node, std::map< TagStore::Node*, SerializationNodeInfo >& idMap, ItemIndexFactory & indexFactory) const {
	destination << static_cast<uint16_t>(idMap.at(node->parent()).nodeId); //parent nodeId
	destination << static_cast<uint16_t>(idMap.at(node).posInParent); //posInParent
	std::set<uint32_t> nodeIndex;
	getFullItemIndex(node, nodeIndex);
	bool ok = true;
	uint32_t indexPtr = indexFactory.addIndex(nodeIndex);
	destination << indexPtr;
	nodeIndex.clear();

	std::map<std::string, uint16_t> childMap;
	for(Node::ConstChildIterator it = node->children().begin(); it != node->children().end(); it++) {
		uint32_t nodeId = idMap.at(it->second).nodeId;
		ok = (nodeId < std::numeric_limits<uint16_t>::max()) && ok;
		childMap[it->first] = nodeId;
	}
	destination << childMap;
	return ok;
}


UByteArrayAdapter& TagStore::serialize(UByteArrayAdapter& destination, ItemIndexFactory & indexFactory) const {
	std::vector<Node*> nodesInLevelOrder;
	std::map<Node*, SerializationNodeInfo> nodeInfo;
	getNodesInLevelOrder(nodesInLevelOrder, nodeInfo);
	nodeInfo[0] = SerializationNodeInfo(0, 0);

	UByteArrayAdapter nodeStore(new std::vector<uint8_t>(), true);
	std::set<uint32_t> nodeOffSets;
	for(size_t i = 0; i < nodesInLevelOrder.size(); i++) {
		nodeOffSets.insert(nodeStore.tellPutPtr());
		if (!serialize(nodeStore, nodesInLevelOrder[i], nodeInfo, indexFactory)) {
			std::cout << "failed to serialize node in TagStore" << std::endl;
			return destination;
		}
	}

	if (nodeOffSets.size() != nodesInLevelOrder.size()) {
		std::cout << "Failed to serialize all nodes in TagStore" << std::endl;
		return destination;
	}
	
	std::vector<uint8_t> nodeIndex;
	ItemIndexPrivateRegLine::create(nodeOffSets, nodeIndex, -1, true);
	
	//now put all the data
	destination.putUint8(0); //version
	destination.putUint32(nodeStore.size() ); //size
	destination.putData( nodeIndex );
	destination.putData( nodeStore );

	return destination;
}

void TagStore::absorb(std::vector<std::string> & path, TagStore::Node* foreignNode, const TagStore & tagStore, std::unordered_map< unsigned int, unsigned int >& remap) {
	if (foreignNode) {
		if (path.size()) {//node a root node 
			remap[tagStore.m_nodePtrNodeIdMap.at(foreignNode)] = insert(path);
		}
		for(Node::ConstChildIterator it = foreignNode->children().begin(); it != foreignNode->children().end(); it++) {
			path.push_back(it->first);
			absorb(path, it->second, tagStore, remap);
			path.pop_back();
		}
	}
}


std::unordered_map< unsigned int, unsigned int >
TagStore::merge(const oscar_create::TagStore& tagStore) {
	std::unordered_map<unsigned int, unsigned int> remap;
	if (tagStore.m_root) {
		//insert root node
		remap[tagStore.m_nodePtrNodeIdMap.at(tagStore.m_root)] = m_nodePtrNodeIdMap[m_root];
		std::vector<std::string> path;
		absorb(path, tagStore.m_root, tagStore, remap);
	}
	return remap;
}

}//end namespace


