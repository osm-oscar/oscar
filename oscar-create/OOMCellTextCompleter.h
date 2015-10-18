#ifndef OSCAR_CREATE_OOM_CELL_TEXT_COMPLETER_H
#define OSCAR_CREATE_OOM_CELL_TEXT_COMPLETER_H
#include <liboscar/OsmKeyValueObjectStore.h>
#include <sserialize/containers/MMVector.h>
#include <sserialize/containers/HashBasedFlatTrie.h>
#include <sserialize/iterator/TransformIterator.h>

#include <limits>

namespace oscar_create {
namespace detail {
namespace OOMCellTextCompleter {

template<typename TNodeIdentifier>
class ValueEntry {
public:
	typedef TNodeIdentifier NodeIdentifier;
public:
	ValueEntry();
	~ValueEntry() {}
	uint32_t cellid() const;
	void cellId(uint32_t v);
	
	void setFullMatch();
	bool fullMatch() const;

	void itemId(uint32_t itemId);
	uint32_t itemId() const;

	const NodeIdentifier & nodeId() const;
	void nodeId(const NodeIdentifier & n);
private:
	constexpr const uint32_t FULL_MATCH = std::numeric_limits<uint32_t>::max();
	constexpr const uint32_t NULL_CELL = std::numeric_limits<uint32_t>::max();
private:
	uint32_t cellId;
	//stores either the item or FULL_MATCH meaning that this cell is fully matched 
	uint32_t itemId;
	TNodeIdentifier node;
};

template<typename TNodeIdentifier>
class ValueEntryItemIteratorMapper {
public:
	uint32_t operator()(ValueEntry<TNodeIdentifier> & e) { return e.cellId(); }
};

template<typename TNodeIdentifier, typename TValueEntryIterator>
using sserialize::TransformIterator< ValueEntryItemIteratorMapper<TNodeIdentifier>, uint32_t,  TValueEntryIterator> = ValueEntryItemIterator;

}}//end namespace detail::OOMCellTextCompleter

/**
  * This class creates the values stored in a text search data structure
  * The text search has to provide a deterministic mapping of strings to unsigned integers,
  * from now on called nodeId.
  * The fill function takes as argument an ItemDerefer, which has an operator()()
  * taking an OsmKeyValueObjectStore::Item and an output iterator (which should be templateized)
  * The fill() function maybe called multiple times
  * 
  * 
  * struct BaseTraits {
  *   typdef <some type> NodeIdentifier
  *   struct NodeIdentifierLessThanPredicate {
  *     bool operator()(NodeIdentifier a, NodeIdentifier b);
  *   };
  *   struct NodeIdentifierEqualPredicate {
  *     bool operator()(NodeIdentifier a, NodeIdentifier b);
  *   };
  * };
  * struct InsertionTraits {
  *   struct ItemDerefer {
  *     ///TOutputIterator dereferences to NodeIdentifier
  *     template<typename TOutputIterator>
  *     void operator()(liboscar::Static::OsmKeyValueObjectStore::Item item, TOutputIterator out);
  *   };
  *   ///returns if item produces full matches (in essence a region)
  *   struct FullMatchPredicate {
  *     bool operator()(liboscar::Static::OsmKeyValueObjectStore::Item item);
  *   }
  *   struct ItemId {
  *     uint32_t operator()(liboscar::Static::OsmKeyValueObjectStore::Item item);
  *   }
  *   struct ItemCells {
  *     template<typename TOutputIterator>
  *     void operator()(liboscar::Static::OsmKeyValueObjectStore::Item item);
  *   }
  *   struct ItemTextSearchNodes {
  *     template<typename TOutputIterator>
  *     void operator()(liboscar::Static::OsmKeyValueObjectStore::Item item);
  *   }
  *   NodeIdentifierLessThanComparator nodeIdentifierLessThanComparator();
  *   NodeIdentifierEqualComparator nodeIdentifierEqualComparator();
  *   NodeIdentifierHashFunction nodeIdentifierHashFunction();
  *   ItemDerefer itemDerefer();
  * }
  * struct OutputTraits {
  *   struct DataOutput {
  *     void operator()(BaseTraits::NodeIdentifier node, const sserialize::UByteArrayAdapter & nodePayload);
  *   };
  *   struct IndexFactoryOut {
  *     template<typename TInputIterator>
  *     uint32_t operator()(TInputIterator begin, TInputIterator end);
  *   };
  * };
  */

template<typename TBaseTraits>
class OOMCTCValuesCreator {
public:
	typedef TBaseTraits Traits;
public:
	OOMCellTextCompleter(Traits traits);
	template<typename TItemIterator, typename TInsertionTraits>
	bool insert(TItemIterator begin, const TItemIterator & end, const sserialize::Static::ItemIndexStore & idxStore, TInsertionTraits itraits);
	template<typename TOutputTraits>
	void append(TOutputTraits otraits);
private:
	typedef detail::OOMCellTextCompleter::ValueEntry ValueEntry;
	typedef sserialize::MMVector<ValueEntry> TreeValueEntries;
private:
	bool finalize();
private:
	Traits m_traits;
	TreeValueEntries m_entries;
};

template<typename TBaseTraits>
template<typename TItemIterator, typename TInputTraits>
bool
OOMCTCValuesCreator<TBaseTraits>::insert(TItemIterator begin, const TItemIterator & end, const sserialize::Static::ItemIndexStore& idxStore, TInputTraits itraits)
{
	typedef typename TInputTraits::ItemDerefer ItemDerefer;
	typedef typename TInputTraits::FullMatchPredicate FullMatchPredicate;
	typedef typename TInputTraits::ItemId ItemIdExtractor;
	typedef typename TInputTraits::ItemCells ItemCellsExtractor;
	typedef typename TInputTraits::ItemTextSearchNodes ItemTextSearchNodesExtractor;
	typedef TItemIterator ItemIterator;
	
	FullMatchPredicate fmPred(itraits.fullMatchPredicate());
	ItemDerefer itemDerefer(itraits.itemDerefer());
	ItemIdExtractor itemIdE(itraits.itemId());
	ItemCellsExtractor itemCellsE(itraits.itemCells());
	ItemTextSearchNodesExtractor nodesE(itraits.itemTextSearchNodes());
	
	std::vector<uint32_t> itemCells;
	std::vector<ValueEntry::NodeIdentifier> itemNodes;
	std::vector<ValueEntry> itemEntries;
	
	for(ItemIterator it(begin); it != end; ++it) {
		itemCells.clear();
		itemNodes.clear();
		itemEntries.clear();
		
		auto item = *it;
		ValueEntry e;
		if (fmPred(item)) {
			e.setFullMatch();
		}
		else {
			e.itemId(itemId(item));
		}
		itemCellsE(std::back_inserter<decltype(itemCells)>(itemCells));
		nodesE(std::back_inserter<decltype(itemNodes)>(itemNodes));
		for(const auto & node : itemNodes) {
			e.nodeId(node);
			for(uint32_t cellId : itemCells) {
				e.cellId(cellId);
				itemEntries.push_back(e);
			}
		}
		m_entries.push_back(itemEntries.cbegin(), itemEntries.cend());
	}
	return true;
}

template<typename TBaseTraits>
template<typename TOutputTraits>
void OOMCTCValuesCreator<TBaseTraits>::append(TOutputTraits otraits)
{
	typedef TOutputTraits OutputTraits;
	typedef typename Traits::NodeIdentifier NodeIdentifier;
	typedef typename Traits::NodeEqualPredicate NodeEqualPredicate;
	typedef typename OutputTraits::IndexFactoryOut IndexFactoryOut;
	typedef typename OutputTraits::DataOut DataOut;
	
	typedef typename detail::OOMCellTextCompleter::ValueEntryItemIterator<NodeIdentifier, TreeValueEntries::const_iterator> VEItemIterator;
	
	finalize();
	
	NodeEqualPredicate nep(m_traits.nodeEqualPredicate());
	IndexFactoryOut ifo(otraits.indexFactoryOut());
	DataOut dout(otraits.dataOut());
	
	
	
	struct SingleEntryState {
		std::vector<uint32_t> fmCellIds;
		std::vector<uint32_t> pmCellIds;
		std::vector<uint32_t> pmCellIdxPtrs;
		sserialize::UByteArrayAdapter sd;
		SingleEntryState() : sd(sserialize::UByteArrayAdapter::createCache(0, sserialize::MM_PROGRAM_MEMORY)) {}
		void clear() {
			pmCellIds.clear();
			pmCellIdxPtrs.clear();
			sd.resize(0);
		}
	} ses;
	
	for(TreeValueEntries::const_iterator eIt(m_entries.begin()), eEnd(m_entries.end()); eIt != eEnd; ++eIt) {
		const NodeIdentifier & ni = eIt->nodeId();
		for(; eIt != eEnd && nep(eIt->nodeId(), ni);) {
			//find the end of this cell
			TreeValueEntries::const_iterator cellBegin(eIt);
			uint32_t cellId = eIt->cellId();
			for(;eIt != eEnd && eIt->cellId() == cellId  && !eIt->fullMatch() && nep(eIt->nodeId(), ni); ++eIt) {}
			if (cellBegin != eIt) { //there are partial matches
				uint32_t indexId = ifo(VEItemIterator(cellBegin), VEItemIterator(eIt));
				ses.pmCellIds.push_back(cellId);
				ses.pmCellIdxPtrs.push_back(indexId);
			}
			//check if we have full matches
			if (eIt->cellId() == cellId && eIt->fullMatch() && nep(eIt->nodeId(), ni)) {
				ses.fmCellIds.push_back(cellId);
				//skip this entry, it's the only one since other fm were removed by finalize()
				++eIt;
			}
		}
		//serialize the data
		uint32_t fmIdxPtr = ifo(ses.fmCellIds.begin(), ses.fmCellIds.end());
		uint32_t pmIdxPtr = ifo(ses.pmCellIds.begin(), ses.pmCellIds.end());
		sserialize::RLEStream::Creator rlc(ses.sd);
		rlc.put(fmIdxPtr);
		rlc.put(pmIdxPtr);
		for(auto x : ses.pmCellIdxPtrs) {
			rlc.put(x);
		}
		rlc.flush();
		dout(ses.sd);
	}
}


///Sorts the storage and makes it unique
template<typename TBaseTraits>
bool OOMCTCValuesCreator<TBaseTraits>::finalize() {
	struct LessThan {
		typedef Traits::NodeIdentifierLessThanComparator NodeComparator;
		NodeComparator nc;
		LessThan(const NodeComparator & nc) : nc(nc) {}
		bool operator()(const ValueEntry & a, const ValueEntry & b) {
			if (nc(a.nodeId(), b.nodeId())) {
				return true;
			}
			else if (nc(b.nodeId(), a.nodeId())) {
				return false;
			}
			else { //they compare equal, check the cellId and then the nodeId/FullMatch
				if (a.cellId() < b.cellId()) {
					return true;
				}
				else if (b.cellId() < a.cellId()) {
					return false;
				}
				else { //cellId are the same
					return a.itemId() < b.itemId(); //this moves full match cells to the end
				}
			}
		}
	};
	struct Equal {
		typedef Traits::NodeIdentifierEqualPredicate NodeComparator;
		NodeComparator nc;
		LessThan(const NodeComparator & nc) : nc(nc) {}
		bool operator()(const ValueEntry & a, const ValueEntry & b) {
			return (nc(a.nodeId(), b.nodeId()) && a.cellId() == b.cellId() && a.itemId() == b.itemId());
		}
	};
	
	LessThan ltp(m_traits.nodeIdentifierLessThanComparator());
	Equal ep(m_traits.NodeIdentifierEqualPredicate());
	
	///BUG:this has to be out of memory
	sserialize::mt_sort(m_entries.begin(), m_entries.end(), ltp);
	///BUG:this has to be out of memory
	 std::unique(m_entries.begin(), m_entries.end(), ep);
}



}//end namespace
#endif