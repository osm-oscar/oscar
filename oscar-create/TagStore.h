#ifndef TAG_STORE_H
#define TAG_STORE_H
#include <string>
#include <sserialize/containers/TreeNode.h>
#include <sserialize/algorithm/hashspecializations.h>
#include <unordered_set>
#include <unordered_map>
#include "types.h"

namespace oscar_create {

class TagStore {
public:
	typedef TreeNode< std::string, std::set<uint32_t> > Node;

private:
	struct SerializationNodeInfo {
		SerializationNodeInfo() : nodeId(0), posInParent(0) {}
		SerializationNodeInfo(uint32_t nodeId, uint32_t posInParent) : nodeId(nodeId), posInParent(posInParent) {}
		uint32_t nodeId;
		uint32_t posInParent;
	};
	
private:
	uint32_t m_nodeCount;
	//if a new node is create, then insert its temporary id here
	std::unordered_map<Node*, uint32_t> m_nodePtrNodeIdMap;
	Node * m_root;

private://data structures to chooses which tags to add and where to add them
	std::unordered_set<std::string> m_keysToStore;
	//maps from key to key, usefull for highway
	std::unordered_map<std::string, std::string> m_keyAttributeStore;
	//maps from attribute key to (key, value), usefull for amenity=restaurant + cuisine=*
	std::unordered_map<std::string, std::pair<std::string, std::string> > m_keyValAttributeStore;
	void populateKeyToStore();

private:
	std::vector<std::string> createTreePath(const std::string & key, const std::string & value);
	bool serialize(sserialize::UByteArrayAdapter& destination, oscar_create::TagStore::Node* node, std::map< oscar_create::TagStore::Node*, oscar_create::TagStore::SerializationNodeInfo >& idMap, sserialize::ItemIndexFactory& indexFactory) const;
	bool getNodesInLevelOrder(std::vector< TagStore::Node* > & nodesInLevelOrder, std::map<Node*, SerializationNodeInfo> & serInfo) const;
	std::unordered_map< TagStore::Node*, uint32_t > enumerateNodes();
	void getFullItemIndex(oscar_create::TagStore::Node* node, std::set< uint32_t >& nodeSet) const;
	bool deleteNodes(Node * curNode, std::set<uint32_t> & nodeIdsToDelete);
	
	void absorb(std::vector< std::string >& path, oscar_create::TagStore::Node * foreignNode, const oscar_create::TagStore & tagStore, std::unordered_map< unsigned int, unsigned int >& remap);
	
public:
	TagStore();
	~TagStore();
	inline uint32_t nodeCount() const { return m_nodeCount; }
	void clear();
	Node* operator[](const std::vector<std::string> & treePath);
	Node * at(const std::string & key, const std::string & value);
	TagStore::Node* insert(const std::vector< std::string >& treePath, uint32_t itemId);
	/** @return  temporary node id for (key,value) */
	uint32_t insert(const std::string & key, const std::string & value);
	uint32_t insert(const std::string & key, const std::string & value, uint32_t itemId);
	/** @return  temporary node id for treePath */
	uint32_t insert(const std::vector< std::string >& treePath);

	bool getNodesInLevelOrder(std::vector< TagStore::Node* > & nodesInLevelOrder) const;
	bool saveTag(const std::string & key, const std::string & value);


	sserialize::UByteArrayAdapter & serialize(sserialize::UByteArrayAdapter& destination, sserialize::ItemIndexFactory& indexFactory) const;
	bool equal(const liboscar::Static::TagStore & sTagStore);

	/** This function merges in tagStore and returns a map to remap all ids from tagStore to the new ids */
	std::unordered_map<unsigned int, unsigned int> merge(const oscar_create::TagStore& tagStore);

};

}//end namespace


#endif