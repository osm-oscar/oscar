#ifndef OSCAR_CREATE_CELL_TEXT_COMPLETER_H
#define OSCAR_CREATE_CELL_TEXT_COMPLETER_H
#include <liboscar/OsmKeyValueObjectStore.h>
#include <sserialize/containers/UnicodeTrie.h>
#include <sserialize/strings/stringfunctions.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/Static/CellTextCompleter.h>
#include <sserialize/utility/printers.h>
#include <sserialize/stats/memusage.h>
#include <sserialize/containers/WindowedArray.h>
#include <sserialize/containers/OADHashTable.h>
#include <sserialize/containers/MMVector.h>
#include <sserialize/containers/HashBasedFlatTrie.h>
#include <sserialize/utility/debug.h>
#include <map>
#include <functional>

//TODO:Wenn intersect(partialMatches, fullMatches) != leer => entsprechende partial matches entfernen

namespace oscar_create {
namespace detail {
namespace CellTextCompleter {

//Flat storage container for items
//CellIds are stored in increasing order
//The least-signifcant bit tells if this it's a fully matched cell or not
//if bit==false => partially matched cell and the next uint tells the number of following items 
typedef sserialize::WindowedArray<uint32_t> Container;
struct MyNodeStorageHTKey {
	uint32_t nodeId;//NodeId
	uint32_t cellId;//CellId
	MyNodeStorageHTKey() : nodeId(0), cellId(0) {}
	MyNodeStorageHTKey(uint32_t nodeId, uint32_t cellId) : nodeId(nodeId), cellId(cellId) {}
	MyNodeStorageHTKey(const MyNodeStorageHTKey & o) : nodeId(o.nodeId), cellId(o.cellId) {}
	inline bool operator==(const MyNodeStorageHTKey & other) const { return nodeId == other.nodeId && cellId == other.cellId; }
	inline bool operator!=(const MyNodeStorageHTKey & other) const { return nodeId != other.nodeId || cellId != other.cellId; }
	inline bool operator<(const MyNodeStorageHTKey & other) const { return (nodeId == other.nodeId ? (cellId < other.cellId) : (nodeId < other.nodeId)); }
};

///Binds a NodeId and does a look-up only on the cells in that node
///Convention: The base hash is of the form Hash< std::pair<uint32_t, T_KEY>, T_VALUE>
template<typename T_HASHTABLE, typename T_VALUE>
class BoundedHashTable {
	T_HASHTABLE * m_hash;
	MyNodeStorageHTKey m_bK;
public:
	BoundedHashTable(T_HASHTABLE * hash) : m_hash(hash), m_bK(0xFFFFFFFF, 0xFFFFFFFF) {}
	~BoundedHashTable() {}
	void setNodeId(uint32_t id) { m_bK.nodeId = id; }
	///This is NOT thread-safe
	inline T_VALUE & operator[](uint32_t key) {
		m_bK.cellId = key;
		return (*m_hash)[m_bK];
	}
	inline T_VALUE & at(uint32_t key) {
		m_bK.cellId = key;
		return m_hash->at(m_bK);
	}
};

struct StlNodeStorageHashFunc1 {
	std::hash<uint64_t> hasher;
	inline size_t operator()(const MyNodeStorageHTKey & v) const {
		return hasher( (static_cast<uint64_t>(v.nodeId) << 32) | static_cast<uint64_t>(v.cellId));
	}
};

struct StlNodeStorageHashFunc2 {
	std::hash<uint64_t> hasher;
	inline size_t operator()(const MyNodeStorageHTKey & v) const {
		return hasher( (static_cast<uint64_t>(v.cellId) << 32) | static_cast<uint64_t>(v.nodeId));
	}
};

///Single threaded universal hash function
template<uint64_t T_PRIME>
struct UniversalNodeStorageHashFunc {
	mutable std::vector< std::pair<uint32_t, uint32_t> > m_as;
	UniversalNodeStorageHashFunc() {}
	void setParams(uint32_t /*nodeCount*/, uint32_t cellCount) {
		uint32_t paramsSize = cellCount;
		srand48(rand());
		m_as.resize(paramsSize);
		for(uint32_t i(0); i < paramsSize; ++i) {
			std::pair<uint32_t, uint32_t> & t = m_as[i];
			t.first = lrand48();
			t.second = lrand48();
		}
	}
	inline const std::pair<uint32_t, uint32_t> & getP(uint32_t param) const {
		assert(m_as.size() > param);
		return m_as[param];
	}
	inline size_t operator()(const MyNodeStorageHTKey & v) const {
		uint32_t paramSelector = v.cellId;
		uint32_t hashv = v.nodeId;
		const std::pair<uint32_t, uint32_t> & params = getP(paramSelector);
		return (static_cast<uint64_t>(hashv)*static_cast<uint64_t>(params.first)+static_cast<uint64_t>(params.second))%T_PRIME;
	}
};

typedef UniversalNodeStorageHashFunc<18446744073709551557ULL> NodeStorageHashFunc1;
typedef UniversalNodeStorageHashFunc<18446744073709551557ULL> NodeStorageHashFunc2;
// typedef StlNodeStorageHashFunc1 NodeStorageHashFunc1;
// typedef StlNodeStorageHashFunc2 NodeStorageHashFunc2;

typedef sserialize::MMVector< std::pair<MyNodeStorageHTKey, uint32_t> > MyNodeStorageHTValueStorage;
typedef sserialize::MMVector<uint32_t> MyNodeStorageHTStorage;
//maps from (nodeId, cellId)->#items in cell
typedef sserialize::OADHashTable<	MyNodeStorageHTKey,
									uint32_t,
									NodeStorageHashFunc1,
									NodeStorageHashFunc2,
									MyNodeStorageHTValueStorage,
									MyNodeStorageHTStorage
										> OADHashTableNodeStorage;
typedef OADHashTableNodeStorage MyNodeStorageHashTable;


typedef MyNodeStorageHashTable MyBoundedHashTableBase;
typedef BoundedHashTable<MyBoundedHashTableBase, uint32_t> MyBoundedHashTable;

//Layout of a MatchDesc:
//-------------------------------
//(CELLID_TYPE|ITEMS_COUNT|ITEMIDS)
//-------------------------------
class ContainerIterator {
private:
	const uint32_t * m_cellBegin;
public:
	ContainerIterator(const uint32_t * ptr) : m_cellBegin(ptr) {}
	~ContainerIterator() {}
	
	inline uint8_t fullyMatchedCell() const { return *m_cellBegin & 0x1; }
	inline uint32_t cellId() const { return *m_cellBegin >> 1; }
	inline uint32_t cellIdType() const { return *m_cellBegin; }
	inline uint32_t itemsCount() const { return m_cellBegin[1]; }
	
	inline const uint32_t * itemsBegin() const { return m_cellBegin+2; }
	inline const uint32_t * itemsEnd() const { return itemsBegin()+itemsCount(); }
	inline void next() {
		if (fullyMatchedCell()) {
			++m_cellBegin;
		}
		else {
			m_cellBegin = itemsEnd();
		}
	}
	inline const uint32_t * cellBegin() const { return m_cellBegin;}
	inline const uint32_t * cellEnd() const { return fullyMatchedCell() ? m_cellBegin+1 : itemsEnd();}
	inline bool operator!=(const ContainerIterator & other) { return m_cellBegin != other.m_cellBegin; }
	ContainerIterator & operator++() {
		next();
		return *this;
	}
};


struct MatchDesc {
	//During the first phase this holds the number of elements in a cell, or numeric_limits<uint32_t> if the cell is fully matched
	//In the second phase this holds the push-offset into the cellIds container or numeric_limits<uint32_t> if the cell is fully matched
	typedef MyBoundedHashTable CellIdToBeginOffsetMap;
	Container cellIds;

	template<typename T_CELL_ID_COUNT_IT>
	void insertRegion(MyBoundedHashTable & cellIdToBeginOffset, T_CELL_ID_COUNT_IT begin, const T_CELL_ID_COUNT_IT & end) {
		for(; begin != end; ++begin) {
			uint32_t cellId = begin->first;
			uint32_t count = begin->second;
			uint32_t & tmp = cellIdToBeginOffset[cellId];
			tmp = sserialize::saturatedAdd32(tmp, count);
		}
	}
	
	template<typename T_INPUT_ITERATOR>
	void insertItem(MyBoundedHashTable & cellIdToBeginOffset, T_INPUT_ITERATOR beginCells, const T_INPUT_ITERATOR & endCells) {
		for(;beginCells != endCells; ++beginCells) {
			uint32_t cellId = *beginCells;
			uint32_t & t = cellIdToBeginOffset[cellId]; //value is default initialized to zero
			t = sserialize::saturatedAdd32(t, 1);
		}
	}
	
	//c must hold at least cellIdsStorageNeed uints
	void initValueContainer(uint32_t* &dataBegin, const std::pair<uint32_t*, uint32_t*> & dataWindow, MyNodeStorageHashTable::iterator & begin, const MyNodeStorageHashTable::iterator & end);
	
	//This should only be called after values Container is setup
	template<typename T_ITEM_ID_IT>
	void insertRegionItems(MyBoundedHashTable & cellIdToBeginOffset, T_ITEM_ID_IT beginItems, const T_ITEM_ID_IT & endItems, uint32_t cellId) {
		uint32_t & offset = cellIdToBeginOffset.at(cellId);
		if (offset < std::numeric_limits<uint32_t>::max()) {
			for(;beginItems != endItems; ++beginItems) {
				cellIds[offset] = *beginItems;
				++offset;
			}
		}
	}
	
	//This should only be called after values Container is setup
	template<typename T_INPUT_ITERATOR>
	void insertItem(MyBoundedHashTable & cellIdToBeginOffset, T_INPUT_ITERATOR beginCells, const T_INPUT_ITERATOR & endCells, uint32_t itemId) {
		for(;beginCells != endCells; ++beginCells) {
			uint32_t & offset = cellIdToBeginOffset.at(*beginCells);
			if (offset < std::numeric_limits<uint32_t>::max()) {
				cellIds[offset] = itemId;
				++offset;
			}
		}
	}
	inline bool empty() const { return !cellIds.size(); }
	inline void clear() {
		cellIds = Container();
	}
	
	inline ContainerIterator begin() const { return ContainerIterator(cellIds.begin()); }
	inline ContainerIterator end() const { return ContainerIterator(cellIds.end()); }
	bool isConsistent(bool checkItemIndex) const;
	void makeConsistent(std::vector<uint32_t> & tmp);
	void dump(std::ostream & out);
};

///TODO:check if cell is fully matched after merging (all items of the cell are matched)
	///This is the case if the cellItemCount equals the number of items
struct ContainerMerger {
	typedef std::vector<uint32_t> CellItemCounts;
	typedef const CellItemCounts* CellItemCountsPtr;
	std::vector<uint32_t> m_tmpD;
	CellItemCountsPtr cellItemCounts;
	ContainerMerger(const std::vector<uint32_t> * cellItemCounts) : cellItemCounts(cellItemCounts) {}
	ContainerMerger(const ContainerMerger & other) :
	cellItemCounts(other.cellItemCounts)
	{}
	///merge srcDest and src, src has to come directly after srcDest
	void operator()(MatchDesc & srcDest, MatchDesc & src);
};

struct Data {
	Data() {}
	Data(const MatchDesc & exact, const MatchDesc & suffix) : exact(exact), suffix(suffix), id(0xFFFFFFFF) {}
	MatchDesc exact;
	MatchDesc suffix;
	uint32_t id;
	void clear() {
		exact.clear();
		suffix.clear();
		id = 0xFFFFFFFF;
	}
	inline bool isConsistent(bool checkItemIndex) const { return exact.isConsistent(checkItemIndex) && suffix.isConsistent(checkItemIndex); }
	inline void makeConsistent(std::vector<uint32_t> & tmp) {
		exact.makeConsistent(tmp);
		suffix.makeConsistent(tmp);
	}
	void dump(std::ostream & out);
};

inline void swap(MatchDesc & a, MatchDesc & b) {
	using std::swap;
	swap(a.cellIds, b.cellIds);
}

inline void swap(Data & a, Data & b) {
	using std::swap;
	swap(a.exact, b.exact);
	swap(a.suffix, b.suffix);
	//swaping invalidates ids
	a.id = 0xFFFFFFFF;
	b.id = 0xFFFFFFFF;
}

struct NodePtrHandler {
	static inline sserialize::UnicodeTrie::Trie<Data>::NodePtr childPtr(sserialize::UnicodeTrie::Trie<Data>::Node::iterator & childIt) {
		return childIt->second;
	}
	static inline sserialize::UnicodeTrie::Trie<Data>::ConstNodePtr childPtr(sserialize::UnicodeTrie::Trie<Data>::Node::const_iterator & childIt) {
		return childIt->second;
	}
	static inline sserialize::HashBasedFlatTrie<Data>::NodePtr childPtr(sserialize::HashBasedFlatTrie<Data>::Node::iterator & childIt) {
		return *childIt;
	}
	static inline sserialize::UnicodeTrie::Trie<Data>::NodePtr parentPtr(sserialize::UnicodeTrie::Trie<Data>::Node & node) {
		return node.parent();
	}
	static inline sserialize::HashBasedFlatTrie<Data>::NodePtr parentPtr(sserialize::HashBasedFlatTrie<Data>::Node & node) {
		return node.parent();
	}
	static inline sserialize::UnicodeTrie::Trie<Data>::NodePtr asPtr(sserialize::UnicodeTrie::Trie<Data>::Node & node) {
		return &node;
	}
	static inline sserialize::UnicodeTrie::Trie<Data>::ConstNodePtr asPtr(const sserialize::UnicodeTrie::Trie<Data>::Node & node) {
		return &node;
	}
	static inline sserialize::HashBasedFlatTrie<Data>::NodePtr asPtr(sserialize::HashBasedFlatTrie<Data>::Node & node) {
		return sserialize::HashBasedFlatTrie<Data>::make_nodeptr(node);
	}
	static inline sserialize::HashBasedFlatTrie<Data>::NodePtr asPtr(const sserialize::HashBasedFlatTrie<Data>::Node & node) {
		return sserialize::HashBasedFlatTrie<Data>::make_nodeptr(node);
	}
};


///Inserts items, calculates and distributes storage to the trie nodes
///Nodes need to have an id which has to be created using the IdDistributor
class StorageHandler {
public:
	typedef MyNodeStorageHashTable MyHashTable;
private:
	sserialize::MmappedMemoryType m_storageMMT;
	MyHashTable m_exactH;
	MyHashTable m_suffixH;
	uint64_t m_exactStorageNeed;
	uint64_t m_suffixStorageNeed;
	detail::CellTextCompleter::MyBoundedHashTable m_eBht;
	detail::CellTextCompleter::MyBoundedHashTable m_sBht;
	sserialize::MmappedMemory<uint32_t> m_mem;
private:
	template<typename TNodePtr>
	void distributeStorage(TNodePtr node, uint32_t* &exactDataPtr, uint32_t* &suffixDataPtr, MyHashTable::iterator & eIt, MyHashTable::iterator & sIt);
public:
	StorageHandler(sserialize::MmappedMemoryType hashMMT, sserialize::MmappedMemoryType storageMMT);
	~StorageHandler() {}
	void setHashParams(uint32_t nodeCount, uint32_t cellCount);
	MyHashTable & exactH() { return m_exactH; }
	MyHashTable & suffixH() { return m_suffixH; }
	template<typename T_CELL_ID_IT>
	void insertRegion(const T_CELL_ID_IT & begin, const T_CELL_ID_IT & end, bool suffix, Data & data);
	template<typename T_ITEM_ID_IT>
	void insertRegionItems(const T_ITEM_ID_IT & begin, const T_ITEM_ID_IT & end, uint32_t cellId, bool suffix, Data & data);
	template<typename T_CELL_ID_IT>
	void insertItem(const T_CELL_ID_IT & begin, const T_CELL_ID_IT & end, bool suffix, Data & data);
	template<typename T_CELL_ID_IT>
	void insertItem(const T_CELL_ID_IT & begin, const T_CELL_ID_IT & end, bool suffix, uint32_t itemId, Data & data);
	///call this after inserting all strings
	void finalizeHashTable();
	///call this after finalize and before distributeStorage
	void calculateStorage();
	///call this after calculateStorage and before inserting items
	template<typename TNodePtr>
	void distributeStorage(TNodePtr rootNode);
	void clearHashes();
	uint64_t exactSN() { return m_exactStorageNeed*sizeof(uint32_t); }
	uint64_t suffixSN() { return m_suffixStorageNeed*sizeof(uint32_t); }
};

template<typename T_CELL_ID_COUNT_IT>
void StorageHandler::insertRegion(const T_CELL_ID_COUNT_IT & begin, const T_CELL_ID_COUNT_IT & end, bool suffix, Data & data) {
	if (suffix) {
		m_sBht.setNodeId(data.id);
		data.suffix.insertRegion(m_sBht, begin, end);
	}
	else {
		m_eBht.setNodeId(data.id);
		data.exact.insertRegion(m_eBht, begin, end);
	}
}

template<typename T_ITEM_ID_IT>
void StorageHandler::insertRegionItems(const T_ITEM_ID_IT & begin, const T_ITEM_ID_IT & end, uint32_t cellId, bool suffix, Data & data) {
	if (suffix) {
		m_sBht.setNodeId(data.id);
		data.suffix.insertRegionItems(m_sBht, begin, end, cellId);
	}
	else {
		m_eBht.setNodeId(data.id);
		data.exact.insertRegionItems(m_eBht, begin, end, cellId);
	}
}

template<typename T_CELL_ID_IT>
void StorageHandler::insertItem(const T_CELL_ID_IT & begin, const T_CELL_ID_IT & end, bool suffix, uint32_t itemId, Data & data) {
	if (suffix) {
		m_sBht.setNodeId(data.id);
		data.suffix.insertItem(m_sBht, begin, end, itemId);
	}
	else {
		m_eBht.setNodeId(data.id);
		data.exact.insertItem(m_eBht, begin, end, itemId);
	}
}

template<typename T_CELL_ID_IT>
void StorageHandler::insertItem(const T_CELL_ID_IT & begin, const T_CELL_ID_IT & end, bool suffix, Data & data) {
	if (suffix) {
		m_sBht.setNodeId(data.id);
		data.suffix.insertItem(m_sBht, begin, end);
	}
	else {
		m_eBht.setNodeId(data.id);
		data.exact.insertItem(m_eBht, begin, end);
	}
}

template<typename TNodePtr>
void
StorageHandler::distributeStorage(TNodePtr rootNode) {
	m_mem = sserialize::MmappedMemory<uint32_t>(m_exactStorageNeed+m_suffixStorageNeed, m_storageMMT);
	uint32_t * exactDataPtr = m_mem.data();
	assert(exactDataPtr);
	uint32_t * suffixDataPtr = exactDataPtr + m_exactStorageNeed;
	MyHashTable::iterator eIt(m_exactH.begin()), sIt(m_suffixH.begin());
	distributeStorage(rootNode, exactDataPtr, suffixDataPtr, eIt, sIt);
	assert(exactDataPtr == m_mem.data()+m_exactStorageNeed);
	assert(suffixDataPtr == m_mem.data()+(m_exactStorageNeed+m_suffixStorageNeed));
	assert(eIt == m_exactH.end());
	assert(sIt == m_suffixH.end());
}

template<typename TNodePtr>
void StorageHandler::distributeStorage(TNodePtr node, uint32_t* &exactDataPtr, uint32_t* &suffixDataPtr, MyHashTable::iterator & eIt, MyHashTable::iterator & sIt) {
	{
		std::pair<uint32_t*, uint32_t*> dataWindow(m_mem.begin(), m_mem.end());
		uint32_t nodeId = node->value().id;
		if (eIt->first.nodeId == nodeId) {
			node->value().exact.initValueContainer(exactDataPtr, dataWindow, eIt, m_exactH.end());
		}
		else {
			node->value().exact.cellIds = detail::CellTextCompleter::Container(exactDataPtr, exactDataPtr, exactDataPtr);
		}
		if (sIt->first.nodeId == nodeId) {
			node->value().suffix.initValueContainer(suffixDataPtr, dataWindow, sIt, m_suffixH.end());
		}
		else {
			node->value().suffix.cellIds = detail::CellTextCompleter::Container(suffixDataPtr, suffixDataPtr, suffixDataPtr);
		}
	}
	for(auto it(node->begin()), end(node->end()); it != end; ++it) {
		distributeStorage( detail::CellTextCompleter::NodePtrHandler::childPtr(it), exactDataPtr, suffixDataPtr, eIt, sIt);
	}
}

///Finalizes the string content of a trie
class Finalizer {
private:
	uint32_t m_curId;
	template<typename TNodePtr>
	void distributeIds(TNodePtr node);
public:
	Finalizer() : m_curId(0) {}
	inline uint32_t id() { return m_curId; }
	///call this only once
	void finalize(sserialize::UnicodeTrie::Trie<Data> & tt);
	///call this only once
	void finalize(sserialize::HashBasedFlatTrie<Data> & ft);
};

template<typename TNodePtr>
void Finalizer::distributeIds(TNodePtr node) {
	node->value().id = m_curId;
	++m_curId;
	for(auto it(node->begin()), end(node->end()); it != end; ++it) {
		distributeIds(detail::CellTextCompleter::NodePtrHandler::childPtr(it));
	}
}


struct Payload {
	struct ResultDesc {
		uint32_t fmPtr; //fully matched cells index ptr
		uint32_t pPtr; //partial matched cell index ptr
		std::vector<uint32_t> pItemsPtrs; //ptrs to indexes contained in partial matched cells, UNSORTED
		ResultDesc() : fmPtr(0), pPtr(0) {}
	};
	sserialize::StringCompleter::QuerryType availableDescs;
	ResultDesc exact;
	ResultDesc prefix;
	ResultDesc suffix;
	ResultDesc substr;
	Payload() : availableDescs(sserialize::StringCompleter::QT_NONE) {}
	inline void addQt(sserialize::StringCompleter::QuerryType qt) {
		availableDescs = (sserialize::StringCompleter::QuerryType) (availableDescs | qt);
	}
};

uint8_t serialize(const Payload::ResultDesc & src, sserialize::UByteArrayAdapter & dest);
sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const Payload & src);


class PayloadHandler {
	std::vector<uint32_t> m_createResultDescFMIdx;
	std::vector<uint32_t> m_createResultDescPMIdx;
	ContainerMerger m_cm;
public:
	PayloadHandler(ContainerMerger::CellItemCountsPtr cellItemCounts) : m_cm(cellItemCounts) {}
	PayloadHandler(const PayloadHandler & other) :
	m_cm(other.m_cm),
	idxFactory(other.idxFactory)
	{}
	sserialize::ItemIndexFactory * idxFactory;
	Payload::ResultDesc createResultDesc(MatchDesc & src);
	
	template<typename TNodePtr>
	Payload operator()(TNodePtr node);
};

template<typename TNodePtr>
Payload PayloadHandler::operator()(TNodePtr node) {
	Data & d = node->value();
	Payload p;
	if (!d.exact.empty()) {
		p.addQt(sserialize::StringCompleter::QT_EXACT);
		p.exact = createResultDesc(d.exact);
	}
	if (!d.suffix.empty()) {
		p.addQt(sserialize::StringCompleter::QT_SUFFIX);
		p.suffix = createResultDesc(d.suffix);
	}
	
	if (node->hasChildren()) {
		for(auto it(node->begin()), end(node->end()); it != end; ++it) {
			TNodePtr child = detail::CellTextCompleter::NodePtrHandler::childPtr(it);
			m_cm(d.exact, child->value().exact);
			m_cm(d.suffix, child->value().suffix);
		}
		if (d.exact.cellIds.size()) {
			p.addQt(sserialize::StringCompleter::QT_PREFIX);
			p.prefix = createResultDesc(d.exact);
		}
		if (d.suffix.cellIds.size()) {
			p.addQt(sserialize::StringCompleter::QT_SUBSTRING);
			p.substr = createResultDesc(d.suffix);
		}
	}
	return p;
}

struct PayloadComparatorBase {
	struct StaticIndexDereferBase {
		virtual const sserialize::ItemIndex at(uint32_t id) const = 0;
	};
	
	StaticIndexDereferBase * idxDerfer;
	
	template<typename T_FUNCTOID>
	struct StaticIndexDerefer: public StaticIndexDereferBase {
		T_FUNCTOID fn;
		StaticIndexDerefer(T_FUNCTOID fn) : fn(fn) {}
		virtual const sserialize::ItemIndex at(uint32_t id) const override {
			return fn(id);
		};
	};
	
	struct DataGatherer {
		struct MatchDesc {
			std::unordered_set<uint32_t> fullyMatchedCells;
			std::unordered_map<uint32_t, std::set<uint32_t> > partialMatchedCells;
			void insert(const detail::CellTextCompleter::MatchDesc & matchDesc);
			inline bool empty() { return fullyMatchedCells.empty() && partialMatchedCells.empty(); }
		};
		MatchDesc prefix, substr;
		template<typename TNodePtr>
		void operator()(const TNodePtr & node) {
			prefix.insert(node->value().exact);
			substr.insert(node->value().suffix);
		}
	};
	
	bool eq(const DataGatherer::MatchDesc & a, const sserialize::Static::CellTextCompleter::Payload::Type & b) const;
	bool eq(const MatchDesc & a, const sserialize::Static::CellTextCompleter::Payload::Type & b) const;
};

template<typename TStaticTrie>
struct PayloadComparator: public PayloadComparatorBase {
	TStaticTrie trie;
	template<typename TNodePtr>
	bool operator()(TNodePtr node, const typename TStaticTrie::Node & snode) const {
		sserialize::Static::CellTextCompleter::Payload p( trie.payload(snode.payloadPtr() ) );
		DataGatherer dg;
		for(auto it(node->begin()), end(node->end()); it != end; ++it) {
			NodePtrHandler::childPtr(it)->apply(dg);
		}
		bool ok = ((!node->value().exact.empty()) == bool(p.types() & sserialize::StringCompleter::QT_EXACT));
		ok = ok && ((!node->value().suffix.empty()) == bool(p.types() & sserialize::StringCompleter::QT_SUFFIX));
		ok = ok && ((!dg.prefix.empty()) == bool(p.types() & sserialize::StringCompleter::QT_PREFIX));
		ok = ok && ((!dg.substr.empty()) == bool(p.types() & sserialize::StringCompleter::QT_SUBSTRING));
		if (!ok) {
			return false;
		}
		dg.prefix.insert(node->value().exact);
		dg.substr.insert(node->value().suffix);
		
		if (!node->value().exact.empty() && !eq(node->value().exact, p.type(sserialize::StringCompleter::QT_EXACT))) {
			return false;
		}
		if (!node->value().suffix.empty() && !eq(node->value().suffix, p.type(sserialize::StringCompleter::QT_SUFFIX))) {
			return false;
		}
		if (!dg.prefix.empty() && !eq(dg.prefix, p.type(sserialize::StringCompleter::QT_PREFIX))) {
			return false;
		}
		if (!dg.substr.empty() && !eq(dg.substr, p.type(sserialize::StringCompleter::QT_SUBSTRING))) {
			return false;
		}
		return true;
	}
};

///TSuffixStringsContainer
///TSuffixStringsContainer contains a struct/class with the following functions:
///baseString() the base string
///begin(), end() iterators to the suffix strings iterator which are themselves just iterator into baseString()
struct SampleSuffixStringsContainer {
	struct SuffixString {
		std::string base;
		std::vector<uint32_t> suffixes;
		SuffixString(const std::string & str) : base(str) {}
		SuffixString(SuffixString && o) : base(o.base), suffixes(o.suffixes) {}
		template<typename T_SEPARATOR_SET>
		SuffixString(const std::string & str, const T_SEPARATOR_SET & seps) : base(str) {
			std::string::const_iterator baseEnd(base.cend());
			std::string::const_iterator it(base.cbegin());
			if (it != baseEnd) {
				sserialize::nextSuffixString(it, baseEnd, seps);
			}
			if (it != baseEnd) {
				std::string::const_iterator baseBegin(base.cbegin());
				while (it != baseEnd) {
					suffixes.push_back(it-baseBegin);
					sserialize::nextSuffixString(it, baseEnd, seps);
				}
			}
		}
		inline const std::string & baseString() const { return base; }
		inline std::string::const_iterator baseCBegin() const { return base.cbegin(); }
		inline std::string::const_iterator baseCEnd() const { return base.cend(); }
		inline std::vector<uint32_t>::const_iterator cbegin() const { return suffixes.cbegin(); }
		inline std::vector<uint32_t>::const_iterator cend() const { return suffixes.cend(); }
		bool valid() const {
			std::string::const_iterator bBegin(base.cbegin()), bEnd(base.cend());
			for(uint32_t x : suffixes) {
				if (!utf8::is_valid(bBegin+x, bEnd)) {
					return false;
				}
			}
			return true;
		}
	};
	std::vector<SuffixString> strings;
	inline std::vector<SuffixString>::const_iterator cbegin() const { return strings.cbegin(); }
	inline std::vector<SuffixString>::const_iterator cend() const { return strings.cend(); }
	void push_back(const std::string & str) { strings.push_back(SuffixString(str)); }
	template<typename T_SEPARATOR_SET>
	void push_back(const std::string & str, const T_SEPARATOR_SET & seps) { strings.push_back(SuffixString(str, seps)); }
	void clear() {
		strings.clear();
	}
	bool empty() const { return strings.empty(); }
	bool valid() const {
		for(const SuffixString & x : this->strings) {
			if (!x.valid()) {
				return false;
			}
		}
		return true;
	}
	///@return true iff there is a suffix equal to @param str

	static constexpr uint32_t npos = 0xFFFFFFFF;
	uint32_t find(const std::string & str) const {
		uint32_t pos = 0;
		for(const SuffixString & sb : strings) {
			if (sb.base.size() < str.size()) {
				continue;
			}
			bool ok = true;
			for(std::string::const_reverse_iterator strIt(str.crbegin()), strEnd(str.crend()), sIt(sb.base.crbegin()); strIt != strEnd; ++strIt, ++sIt) {
				if (*strIt != *sIt) {
					ok = false;
					break;
				}
			}
			if (ok) {
				return pos;
			}
			++pos;
		}
		return npos;
	}
	
	inline bool contains(const std::string & str) const {
		return find(str) != npos;
	}
};

template<typename TSuffixStringsContainer, typename TPrefixStringsContainer>
struct ItemStringsContainer {
	typedef TSuffixStringsContainer SubStringsContainer;
	typedef TPrefixStringsContainer PrefixStringsContainer;
	TSuffixStringsContainer subString;
	TPrefixStringsContainer prefixOnly;
	void clear() { 
		subString.clear();
		prefixOnly.clear();
	}
	bool empty() const {
		return subString.empty() && prefixOnly.empty();
	}
	bool valid() const {
		for(auto x : prefixOnly) {
			if (!utf8::is_valid(x.cbegin(), x.cend())) {
				return false;
			}
		}
		return subString.valid();
	}
};

typedef ItemStringsContainer<SampleSuffixStringsContainer, std::vector<std::string> > SampleItemStringsContainer;

}}}//end namespace oscar_create::detail::CellTextCompleter

namespace std {

template<>
struct hash< sserialize::HashBasedFlatTrie<oscar_create::detail::CellTextCompleter::Data>::NodePtr > {
	sserialize::HashBasedFlatTrie<oscar_create::detail::CellTextCompleter::Data>::NodePtrHash hasher;
	inline std::size_t operator()(const sserialize::HashBasedFlatTrie<oscar_create::detail::CellTextCompleter::Data>::NodePtr & nodePtr) const {
		return hasher(nodePtr);
	}
};

}//end namespace std

namespace oscar_create {

template<typename TBaseTrieType = sserialize::UnicodeTrie::Trie<detail::CellTextCompleter::Data> >
class CellTextCompleter {
public:
	typedef detail::CellTextCompleter::Data TrieData;
	typedef TBaseTrieType MyTrie;
	typedef typename MyTrie::Node Node;
	typedef typename MyTrie::NodePtr NodePtr;
private:
	struct ItemStringsNodes {
		std::unordered_set<NodePtr> exactNodes;
		std::unordered_set<NodePtr> suffixNodes;
		inline void clear() {
			exactNodes.clear();
			suffixNodes.clear();
		}
	};
private:
	MyTrie m_trie;
	detail::CellTextCompleter::StorageHandler m_sh;
	detail::CellTextCompleter::ContainerMerger::CellItemCounts m_cellItemCounts;
private:
	NodePtr root() { return m_trie.root(); }
public:
	template<typename T_STRINGS_CONTAINER>
	void insert(const T_STRINGS_CONTAINER & itemStrings);
	
	template<typename T_STRINGS_CONTAINER>
	void findNodes(const T_STRINGS_CONTAINER & itemStrings, ItemStringsNodes & nodes);
public:
	CellTextCompleter() : m_sh(sserialize::MM_SHARED_MEMORY, sserialize::MM_SHARED_MEMORY) {}
	CellTextCompleter(sserialize::MmappedMemoryType mmt) : m_sh(sserialize::MM_SHARED_MEMORY, mmt) {}
	virtual ~CellTextCompleter() {}
	///@param itemDerefer a functor void operator(const liboscar::Static::OsmKeyValueObjectStore::Item & item, ItemStrings & itemStrings) defining the strings to be stored
	///@param itemDerfer There also has to be itemDerefer::StringsContainer to define the used StringsContainer
	template<typename TItemDerefer>
	bool create(const liboscar::Static::OsmKeyValueObjectStore& store, const sserialize::Static::ItemIndexStore& idxStore, TItemDerefer itemDerefer);

	sserialize::UByteArrayAdapter & append(sserialize::StringCompleter::SupportedQuerries sq, sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory & idxFactory, uint32_t threadCount);
	
	//this only valid before calling create, so in order to test the equality you have to create the data twice
	bool equal(sserialize::Static::CellTextCompleter sct, std::function< sserialize::ItemIndex(uint32_t)> idxDerefer);
	bool checkConsistency(bool checkItemIndex);
	bool checkParentChildRelations();
};

template<typename TBaseTrieType>
template<typename TItemDerefer>
bool
CellTextCompleter<TBaseTrieType>::create(const liboscar::Static::OsmKeyValueObjectStore & store, const sserialize::Static::ItemIndexStore & idxStore, TItemDerefer itemDerefer) {
	sserialize::ProgressInfo pinfo;
	uint32_t ghSize = store.geoHierarchy().regionSize();
	uint32_t storeSize = store.size();
	
	m_cellItemCounts.resize(store.geoHierarchy().cellSize());
	for(uint32_t i(0), s(store.geoHierarchy().cellSize()); i < s; ++i) {
		m_cellItemCounts[i] = store.geoHierarchy().cellItemsCount(i);
	}
	
	typename TItemDerefer::StringsContainer itemStrings;
	ItemStringsNodes itemStringNodes;
	std::vector<uint32_t> cellIndex;
	std::back_insert_iterator< std::vector<uint32_t> > cellIndexBackInserter(cellIndex);
	std::unordered_map<uint32_t, uint32_t> assocCellIdItemCount;//number of items in associated cells of a region
	std::unordered_map<uint32_t, uint32_t> assocCellIdOffset;//offset into cellIndex needed during inserting associated items
	sserialize::ItemIndex regionItems, regionCells;
	sserialize::BoundedCompactUintArray itemCells;
	sserialize::DynamicBitSet inConsistentMatchDescs;
	uint32_t nodeCount = 0;
	std::atomic<uint32_t> count(0);
	
	pinfo.begin(ghSize, "CTC: adding region strings");
	#pragma omp parallel for schedule(dynamic, 1)
	for(uint32_t i = 0; i < ghSize; ++i) {
		typename TItemDerefer::StringsContainer itemStrings;
		itemDerefer(store.at(i), itemStrings, false);
		if (itemStrings.empty()) {
			continue;
		}
		
		//insert strings into tree
		#pragma omp critical(insertItemStrings)
		{
			insert(itemStrings);
		}
		++count;
		pinfo(count);
	}
	pinfo.end();
	
	pinfo.begin(store.size(), "CTC: adding strings");
	count = 0;
	#pragma omp parallel for schedule(dynamic, 1)
	for(uint32_t i = 0; i < storeSize; ++i) {
		typename TItemDerefer::StringsContainer itemStrings;
		itemDerefer(store.at(i), itemStrings, true);
		if (itemStrings.empty()) {
			continue;
		}
		
		//insert strings into tree
		#pragma omp critical(insertItemStrings)
		{
			insert(itemStrings);
		}
		++count;
		pinfo(count);
	}
	pinfo.end();
	sserialize::MemUsage().print();
	{
		if (m_trie.size() > std::numeric_limits<uint32_t>::max()) {
			std::cout << "Unable to distribute nodeids as id space is too small" << std::endl;
		}
		
		std::cout << "Finalizing trie..." << std::endl;
		detail::CellTextCompleter::Finalizer finalizer;
		finalizer.finalize(m_trie);
		std::cout << "Trie has " << finalizer.id() << " nodes" << std::endl;
		nodeCount = finalizer.id();
		m_sh.setHashParams(nodeCount, store.geoHierarchy().cellSize());
	}
	
	pinfo.begin(ghSize, "CTC: processing regions");
	count = 0;
	#pragma omp parallel for schedule(dynamic, 1)
	for (uint32_t i = 0; i < ghSize; ++i) {
		typename TItemDerefer::StringsContainer itemStrings;
		ItemStringsNodes itemStringNodes;
		std::vector<uint32_t> cellIndex;
		std::unordered_map<uint32_t, uint32_t> assocCellIdItemCount;//number of items in associated cells of a region
		std::unordered_map<uint32_t, uint32_t> assocCellIdOffset;//offset into cellIndex needed during inserting associated items
		sserialize::ItemIndex regionItems, regionCells;
		sserialize::BoundedCompactUintArray itemCells;
		
		sserialize::Static::spatial::GeoHierarchy::Region r(store.geoHierarchy().regionFromStoreId(i));
		
		itemDerefer(store.at(r.storeId()), itemStrings, false);
		if (itemStrings.empty()) {
			continue;
		}
		
		//get the end nodes
		#pragma omp critical(findNodes)
		{
			findNodes(itemStrings, itemStringNodes);
		}
		{//insert info into trie
			regionCells = idxStore.at(r.cellIndexPtr());
			regionItems = idxStore.at(r.itemsPtr());
			for(uint32_t x : regionCells) {
				assocCellIdItemCount[x] = std::numeric_limits<uint32_t>::max();
			}
			for(sserialize::ItemIndex::const_iterator itemIt(regionItems.cbegin()), itemItEnd(regionItems.cend()); itemIt != itemItEnd; ++itemIt) {
				uint32_t itemId = *itemIt;
				if (itemId < ghSize) //skip regions
					continue;
				itemCells = store.cells(itemId);
				for(uint32_t j(0), js(itemCells.size()); j < js; ++j) {
					uint32_t & x = assocCellIdItemCount[itemCells.at(j)];
					x = sserialize::saturatedAdd32(x, 1);
				}
			}
			#pragma omp critical(insertRegionItemsExactNodes)
			{
				for(NodePtr node : itemStringNodes.exactNodes) {
					m_sh.insertRegion(assocCellIdItemCount.cbegin(), assocCellIdItemCount.cend(), false, node->value());
				}
			}
			#pragma omp critical(insertRegionItemsSuffixNodes)
			{
				for(NodePtr node : itemStringNodes.suffixNodes) {
					m_sh.insertRegion(assocCellIdItemCount.cbegin(), assocCellIdItemCount.cend(), true, node->value());
				}
			}
		}
		++count;
		pinfo(count);
	}
	pinfo.end();

	pinfo.begin(store.size(), "CTC: inserting item cells");
	count = 0;
	#pragma omp parallel for schedule(dynamic, 1)
	for (uint32_t i = 0; i < storeSize; ++i) {
		typename TItemDerefer::StringsContainer itemStrings;
		ItemStringsNodes itemStringNodes;
		
		itemDerefer(store.at(i), itemStrings, true);
		if (itemStrings.empty()) {
			continue;
		}
		sserialize::BoundedCompactUintArray cells(store.cells(i));
		std::vector<uint32_t> cellIndex(cells.cbegin(), cells.cend());
		
		//get the end nodes
		#pragma omp critical(findNodes)
		{
			findNodes(itemStrings, itemStringNodes);
		}
		#pragma omp critical(updateCellIdUsageExactNodes)
		{
			for(NodePtr node : itemStringNodes.exactNodes) {
				m_sh.insertItem(cellIndex.cbegin(), cellIndex.cend(), false, node->value());
			}
		}
		#pragma omp critical(updateCellIdUsageSuffixNodes)
		for(NodePtr node : itemStringNodes.suffixNodes) {
			m_sh.insertItem(cellIndex.cbegin(), cellIndex.cend(), true, node->value());
		}
		++count;
		pinfo(count);
	}
	pinfo.end();
	
	std::cout << "Finalzing hashtables..." << std::flush;
	m_sh.finalizeHashTable();
	std::cout << "done" << std::endl;
	std::cout << "Calculating storage need..." << std::flush;
	m_sh.calculateStorage();
	std::cout << "done" << std::endl;
	std::cout << "Exact index storage need: " << sserialize::prettyFormatSize(m_sh.exactSN()) << std::endl;
	std::cout << "Suffix index storage need: " << sserialize::prettyFormatSize(m_sh.suffixSN()) << std::endl;
	std::cout << "Distributing storage..." << std::flush;
	m_sh.distributeStorage(m_trie.root());
	std::cout << "done" << std::endl;
	
	checkConsistency(false);
	
	pinfo.begin(ghSize, "CTC: Inserting items associated with regions");
	count = 0;
	#pragma omp parallel for schedule(dynamic, 1)
	for (uint32_t i = 0; i < ghSize; ++i) {
		typename TItemDerefer::StringsContainer itemStrings;
		ItemStringsNodes itemStringNodes;
		std::vector<uint32_t> cellIndex;
		std::unordered_map<uint32_t, uint32_t> assocCellIdItemCount;//number of items in associated cells of a region
		std::unordered_map<uint32_t, uint32_t> assocCellIdOffset;//offset into cellIndex needed during inserting associated items
		sserialize::ItemIndex regionItems, regionCells;
		sserialize::BoundedCompactUintArray itemCells;

		sserialize::Static::spatial::GeoHierarchy::Region r(store.geoHierarchy().regionFromStoreId(i));
		
		itemDerefer(store.at(r.storeId()), itemStrings, false);
		if (itemStrings.empty()) {
			continue;
		}
		
		//get the end nodes
		#pragma omp critical(findNodes)
		{
			findNodes(itemStrings, itemStringNodes);
		}
		#pragma omp critical(inconstistentMatchDesc)
		{
			for(NodePtr node : itemStringNodes.exactNodes) {
				inConsistentMatchDescs.set(node->value().id);
			}
			for(NodePtr node : itemStringNodes.suffixNodes) {
				inConsistentMatchDescs.set(node->value().id);
			}
		}
		
		{//insert info into trie
			regionCells = idxStore.at(r.cellIndexPtr());
			regionItems = idxStore.at(r.itemsPtr());
			for(uint32_t x : regionCells) {
				assocCellIdItemCount[x] = std::numeric_limits<uint32_t>::max();
			}
			for(sserialize::ItemIndex::const_iterator itemIt(regionItems.cbegin()), itemItEnd(regionItems.cend()); itemIt != itemItEnd; ++itemIt) {
				uint32_t itemId = *itemIt;
				if (itemId < ghSize) //skip regions
					continue;
				itemCells = store.cells(itemId);
				for(uint32_t j(0), js(itemCells.size()); j < js; ++j) {
					uint32_t & x = assocCellIdItemCount[itemCells.at(j)];
					x = sserialize::saturatedAdd32(x, 1);
				}
			}
			uint32_t totalStorageNeed = 0;
			for(const std::pair<uint32_t, uint32_t> & x : assocCellIdItemCount) {
				if (x.second != std::numeric_limits<uint32_t>::max()) {
					assocCellIdOffset[x.first] = totalStorageNeed;
					totalStorageNeed += x.second;
				}
			}
			cellIndex.resize(totalStorageNeed);
			
			for(sserialize::ItemIndex::const_iterator itemIt(regionItems.cbegin()), itemItEnd(regionItems.cend()); itemIt != itemItEnd; ++itemIt) {
				uint32_t itemId = *itemIt;
				if (itemId < ghSize) //skip regions
					continue;
				itemCells = store.cells(itemId);
				for(uint32_t j(0), js(itemCells.size()); j < js; ++j) {
					uint32_t cellId = itemCells.at(j);
					uint32_t & cellItemCount = assocCellIdItemCount[cellId];
					if (cellItemCount != std::numeric_limits<uint32_t>::max()) {
						uint32_t & o = assocCellIdOffset[cellId];
						cellIndex[o] = itemId;
						++o;
					}
				}
			}
			{//recalculate offsets
				uint32_t totalStorageNeed = 0;
				for(const std::pair<uint32_t, uint32_t> & x : assocCellIdItemCount) {
					if (x.second != std::numeric_limits<uint32_t>::max()) {
						assocCellIdOffset[x.first] = totalStorageNeed;
						totalStorageNeed += x.second;
					}
				}
			}
			for(const std::pair<uint32_t, uint32_t> & x : assocCellIdItemCount) {
				if (x.second == std::numeric_limits<uint32_t>::max()) {
					continue;
				}
				uint32_t offset = assocCellIdOffset.at(x.first);
				std::vector<uint32_t>::const_iterator itemsBegin = cellIndex.cbegin()+offset;
				std::vector<uint32_t>::const_iterator itemsEnd = itemsBegin + x.second;
				#pragma omp critical(updateCellIdUsageExactNodes)
				{
					for(NodePtr node : itemStringNodes.exactNodes) {
						m_sh.insertRegionItems(itemsBegin, itemsEnd, x.first, false, node->value());
					}
				}
				#pragma omp critical(updateCellIdUsageSuffixNodes)
				{
					for(NodePtr node : itemStringNodes.suffixNodes) {
						m_sh.insertRegionItems(itemsBegin, itemsEnd, x.first, true, node->value());
					}
				}
			}
		}
		++count;
		pinfo(count);
	}
	pinfo.end();
	assocCellIdItemCount = decltype(assocCellIdItemCount)();
	assocCellIdOffset = decltype(assocCellIdOffset)();
	cellIndex = decltype(cellIndex)();
	
	pinfo.begin(store.size(), "CTC: Inserting items");
	for (uint32_t i(0), s(store.size()); i < s; ++i) {
		itemStringNodes.clear();
		cellIndex.clear();
		itemStrings.clear();
		store.cells(i).copyInto(cellIndexBackInserter);
		
		itemDerefer(store.at(i), itemStrings, true);
		if (itemStrings.empty()) {
			continue;
		}
		
		//insert strings into tree
		findNodes(itemStrings, itemStringNodes);

		for(NodePtr node : itemStringNodes.exactNodes) {
			m_sh.insertItem(cellIndex.cbegin(), cellIndex.cend(), false, i, node->value());
		}
		for(NodePtr node : itemStringNodes.suffixNodes) {
			m_sh.insertItem(cellIndex.cbegin(), cellIndex.cend(), true, i, node->value());
		}
		pinfo(i);
	}
	pinfo.end();
	
	sserialize::MemUsage().print();
	
	m_sh.clearHashes();
	
	{
		std::vector<uint32_t> & tmpData = cellIndex;
		pinfo(nodeCount, "CTC: fixing inconsistent MatchDesc");
		auto mkCsFunc = [&pinfo, &inConsistentMatchDescs, &tmpData](Node & node) {
			detail::CellTextCompleter::Data & data = node.value();
			if (inConsistentMatchDescs.isSet(data.id)) {
				data.makeConsistent(tmpData);
			}
		};
		m_trie.root()->apply(mkCsFunc);
		pinfo.end();
	}
	
	
#if defined(DEBUG_CHECK_ALL) || defined(DEBUG_OSCAR_CREATE_CELL_TEXT_COMPLETER)
	checkConsistency(true);
#endif
	return true;
}

template<typename TBaseTrieType>
template<typename T_STRINGS_CONTAINER>
void
CellTextCompleter<TBaseTrieType>::findNodes
(const T_STRINGS_CONTAINER & itemStrings, ItemStringsNodes & nodes) {
	for(auto it(itemStrings.prefixOnly.cbegin()), end(itemStrings.prefixOnly.cend()); it != end; ++it) {
		NodePtr n = m_trie.findNode(it->cbegin(), it->cend(), false);
		nodes.exactNodes.insert(n);
// 		nodes.suffixNodes.insert(n); //TODO: do we need this?
	}
	for(auto it(itemStrings.subString.cbegin()), end(itemStrings.subString.cend()); it != end; ++it) {
		auto baseStringBegin = it->baseCBegin();
		auto baseStringEnd = it->baseCEnd();
		{
			NodePtr n = m_trie.findNode(baseStringBegin, baseStringEnd, false);
			nodes.exactNodes.insert(n);
			nodes.suffixNodes.insert(n);
		}
		for(auto sit(it->cbegin()), send(it->cend()); sit != send; ++sit) {
			nodes.suffixNodes.insert( m_trie.findNode(baseStringBegin+*sit, baseStringEnd, false) );
		}
	}
}


template<typename TBaseTrieType>
bool CellTextCompleter<TBaseTrieType>::checkParentChildRelations() {
	if (m_trie.root()->parent()) {
		std::cout << "CellTextCompleter: root node has a parent" << std::endl;
		return false;
	}

	bool ok = true;
	auto childParentChecker = [&ok](const typename MyTrie::Node & n) {
		for(typename MyTrie::Node::const_iterator it(n.begin()), end(n.end()); it != end; ++it) {
			typename MyTrie::ConstNodePtr parent = detail::CellTextCompleter::NodePtrHandler::childPtr(it)->parent();
			if (parent != detail::CellTextCompleter::NodePtrHandler::asPtr(n)) {
				ok = false;
				return;
			}
		}
	};
	m_trie.root()->apply(childParentChecker);
	if (!ok) {
		std::cout << "Trie parent->child relations are broken" << std::endl;
	}
	else {
		std::cout << "Trie parent->child relations are ok" << std::endl;
	}
	return ok;
}

template<typename TBaseTrieType>
bool CellTextCompleter<TBaseTrieType>::checkConsistency(bool checkItemIndex) {
	bool pcrOk = checkParentChildRelations();
	bool ok = true;
	auto cC = [&ok, checkItemIndex](const Node & n) {ok = ok && n.value().isConsistent(checkItemIndex); };
	root()->apply(cC);
	if (!ok) {
		std::cout << "Trie values are NOT consistent" << std::endl;
	}
	else {
		std::cout << "Trie values are consistent" << std::endl;
	}
	return ok && pcrOk;
}

//specializations
template<>
template<typename T_STRINGS_CONTAINER>
void CellTextCompleter< sserialize::UnicodeTrie::Trie<detail::CellTextCompleter::Data> >::insert(const T_STRINGS_CONTAINER & itemStrings) {
	for(auto it(itemStrings.prefixOnly.cbegin()), end(itemStrings.prefixOnly.cend()); it != end; ++it) {
		m_trie.insert(it->cbegin(), it->cend());
	}
	for(auto it(itemStrings.subString.cbegin()), end(itemStrings.subString.cend()); it != end; ++it) {
		auto baseStringBegin = it->baseCBegin();
		auto baseStringEnd = it->baseCEnd();
		m_trie.insert(baseStringBegin, baseStringEnd);
		for(auto sit(it->cbegin()), send(it->cend()); sit != send; ++sit) {
			m_trie.insert(baseStringBegin+*sit, baseStringEnd);
		}
	}
}

template<>
template<typename T_STRINGS_CONTAINER>
void CellTextCompleter< sserialize::HashBasedFlatTrie<detail::CellTextCompleter::Data> >::insert(const T_STRINGS_CONTAINER & itemStrings) {
	for(auto it(itemStrings.prefixOnly.cbegin()), end(itemStrings.prefixOnly.cend()); it != end; ++it) {
		m_trie.insert(it->cbegin(), it->cend());
	}
	for(auto it(itemStrings.subString.cbegin()), end(itemStrings.subString.cend()); it != end; ++it) {
		auto baseStringBegin = it->baseCBegin();
		auto baseStringEnd = it->baseCEnd();
		MyTrie::StaticString tmp = m_trie.insert(baseStringBegin, baseStringEnd);
		for(auto sit(it->cbegin()), send(it->cend()); sit != send; ++sit) {
			m_trie.insert(tmp.addOffset(*sit));
		}
	}
}


///HashBasedFlatTrie has no support for node->parent()
template<>
inline bool CellTextCompleter< sserialize::HashBasedFlatTrie<detail::CellTextCompleter::Data> >::checkParentChildRelations() { return true; }


template<>
sserialize::UByteArrayAdapter&
CellTextCompleter< sserialize::UnicodeTrie::Trie<detail::CellTextCompleter::Data>  >::append(sserialize::StringCompleter::SupportedQuerries sq, sserialize::UByteArrayAdapter & dest, sserialize::ItemIndexFactory & idxFactory, uint32_t threadCount);

template<>
sserialize::UByteArrayAdapter&
CellTextCompleter< sserialize::HashBasedFlatTrie<detail::CellTextCompleter::Data> >::append(sserialize::StringCompleter::SupportedQuerries sq, sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory & idxFactory, uint32_t threadCount);

template<>
CellTextCompleter< sserialize::HashBasedFlatTrie<detail::CellTextCompleter::Data> >::CellTextCompleter(sserialize::MmappedMemoryType mmt);

template<>
bool
CellTextCompleter< sserialize::UnicodeTrie::Trie<detail::CellTextCompleter::Data>  >::
equal(sserialize::Static::CellTextCompleter sct, std::function< sserialize::ItemIndex(uint32_t)> idxDerefer);

template<>
bool
CellTextCompleter< sserialize::HashBasedFlatTrie<detail::CellTextCompleter::Data> >::
equal(sserialize::Static::CellTextCompleter sct, std::function< sserialize::ItemIndex(uint32_t)> idxDerefer);


typedef CellTextCompleter< sserialize::UnicodeTrie::Trie<detail::CellTextCompleter::Data> > CellTextCompleterUnicodeTrie;
typedef CellTextCompleter< sserialize::HashBasedFlatTrie<detail::CellTextCompleter::Data> > CellTextCompleterFlatTrie;

}//end namespace


#endif