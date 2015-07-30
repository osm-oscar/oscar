#ifndef STATIC_TAG_STORE_H
#define STATIC_TAG_STORE_H
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/Map.h>


/**FileFormat
 *-------------------------------------------
 *VERSION|SIZE|NodeOffSets|Nodes
 *-------------------------------------------
 *   1   | 4  |ItemIndex  |*
 *-------------------------------------------
 *
 *Node file format:
 *--------------------------------------------
 *ParentId|PosInParent|IndexPtr|ChildIdMap
 *--------------------------------------------
 *    2   |    2      |   4    |StaticMap
 *
 */

namespace liboscar {
namespace Static {

class TagStore {
public:
	class Node {
	private:
		sserialize::Static::Map<std::string, uint16_t> m_children; 
		sserialize::UByteArrayAdapter m_data; //from the beginning
	public:
		Node() {}
		Node(const sserialize::UByteArrayAdapter & data) : m_children(data + 8), m_data(data) {}
		~Node() {}
		uint16_t parentId() const { return m_data.getUint16(0);}
		uint16_t posInParent() const { return m_data.getUint16(2);}
		uint32_t getIndexPtr() const { return m_data.getUint32(4);}
		sserialize::Static::Map<std::string, uint16_t> & children() { return m_children; }
		bool valid() { return m_data.size() > 0;}
	};

private:
	sserialize::ItemIndex m_nodeIndex;
	sserialize::UByteArrayAdapter m_nodes;
	sserialize::Static::ItemIndexStore m_indexStore;
	Node getNodeById(uint16_t nodeId) const;
public:
	TagStore();
	TagStore(const sserialize::UByteArrayAdapter & data, const sserialize::Static::ItemIndexStore & index);
	~TagStore();

	sserialize::ItemIndex complete(const std::vector< uint16_t >& treePath) const;
	sserialize::ItemIndex complete(const std::vector< std::string >& treePath) const;
	sserialize::ItemIndex complete(const uint16_t nodeId) const;
	std::vector< uint16_t > ancestors(uint16_t nodeId) const;
	std::vector<std::string> ancestorNames(uint16_t nodeId) const;
	Node at(uint16_t nodeId) const;
	Node at(const std::vector<uint16_t> & treePath) const;
	Node at(const std::vector<std::string> & treePath) const;
	bool nodeId(const std::vector<std::string> & treePath, uint16_t & nodeId) const;
	std::string nodeString(const uint16_t nodeId);
	uint32_t size() const { return m_nodeIndex.size();}
	uint32_t getSizeInBytes() const { return 1 + m_nodeIndex.getSizeInBytes() + m_nodes.size(); }
	std::ostream& printStats(std::ostream& out) const;
	
	const sserialize::Static::ItemIndexStore & itemIndexStore() const { return m_indexStore; }
	sserialize::Static::ItemIndexStore & itemIndexStore() { return m_indexStore; }
};

}}//end namespace

#endif