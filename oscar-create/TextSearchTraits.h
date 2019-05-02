#ifndef OSCAR_CREATE_TEXT_SEARCH_TRAITS_H
#define OSCAR_CREATE_TEXT_SEARCH_TRAITS_H
#include <liboscar/OsmKeyValueObjectStore.h>
#include <sserialize/strings/DiacriticRemover.h>
#include "Config.h"
#include "CellTextCompleter.h"

//TODO: should regions be included as items in the cells they enclose?
//This is currently not the case but should be done
//There should probably be an extra query to support that so that one can explicitly search for regions as items

//BUG: If an item spans multiple regions then the item also has to be added to the respective reiong search strings
//consider the query (@highway fellbach) / (@highway:stuttgart) which should result in all highways that are in stuttgart and fellbach
//in order for this to work correctly fellbach/stuttgart needs to deref into the enclosed full-match cells AND partial-match cells containing the items that are part of fellbach/stuttgart AND part of something else

namespace oscar_create {

class FilterState {
public:
	class SingleFilter {
	public:
		std::unordered_set<uint32_t> keys;
	public:
		SingleFilter() {}
		SingleFilter(const TextSearchConfig::SearchCapabilities & src, const sserialize::Static::KeyValueObjectStore & kv);
	};
	std::unordered_map<uint32_t, std::string> keyId2Str;
	//[ItemType][TagType][QueryType] -> SearchCapabilites
	SingleFilter filterInfo[2][2][2];
public:
	FilterState(const sserialize::Static::KeyValueObjectStore & kv, const TextSearchConfig & cfg);
};

struct BaseSearchTraitsState {
	FilterState fs;
	TextSearchConfig cfg;
	sserialize::DiacriticRemover dr;
	BaseSearchTraitsState(const sserialize::Static::KeyValueObjectStore & kv, const TextSearchConfig & cfg) : fs(kv, cfg), cfg(cfg) {
		dr.init();
	}
};

struct OOM_SA_CTC_TraitsState {
	liboscar::Static::OsmKeyValueObjectStore store;
	sserialize::Static::spatial::GeoHierarchy gh;
	sserialize::Static::ItemIndexStore idxStore;
	bool withForeignObjects;
	OOM_SA_CTC_TraitsState(const liboscar::Static::OsmKeyValueObjectStore & store, const sserialize::Static::ItemIndexStore & idxStore, bool withForeignObjects) :
	store(store),
	gh(store.geoHierarchy()),
	idxStore(idxStore),
	withForeignObjects(withForeignObjects)
	{}
};

template<TextSearchConfig::ItemType T_ITEM_TYPE = TextSearchConfig::ItemType::ITEM>
class OOM_SA_CTC_Traits {
public:
// 	static constexpr TextSearchConfig::ItemType T_ITEM_TYPE = TextSearchConfig::ItemType::ITEM;
public:
	typedef liboscar::Static::OsmKeyValueObjectStore::Item item_type;
	typedef BaseSearchTraitsState State;
	typedef std::shared_ptr<State> StateSharedPtr;

	template<TextSearchConfig::QueryType T_QUERY_TYPE>
	class StringsExtractor {
	private:
		std::shared_ptr<State> m_state;
		sserialize::DiacriticRemover m_dr;
	protected:
		template<typename TOutputIterator>
		void assign(TOutputIterator & out, const TextSearchConfig::SearchCapabilities & cap, const std::string & str) const {
			if (cap.caseSensitive) {
				*out = str;
				++out;
				if (cap.diacritcInSensitive) {
					*out = m_dr(str);
					++out;
				}
			}
			else {
				std::string tmp = sserialize::unicode_to_lower(str);
				*out = tmp;
				++out;
				if (cap.diacritcInSensitive) {
					*out = sserialize::unicode_to_lower(m_dr(tmp));
					++out;
				}
			}
		}
	public:
		StringsExtractor(const std::shared_ptr<State> & state) : m_state(state), m_dr(m_state->dr) {}
		template<typename TOutputIterator>
		void operator()(const item_type & item, TOutputIterator out) const {
			const State & state = *m_state;
			int it = (int) T_ITEM_TYPE;
			int tt = (int) TextSearchConfig::TagType::VALUES;
			int qt = (int) T_QUERY_TYPE;
			auto scp = state.cfg.searchCapabilites;
			auto fs = state.fs.filterInfo;
			
			for(uint32_t i(0), s(item.size()); i < s; ++i) {
				uint32_t keyId = item.keyId(i);
				
				tt = (int) TextSearchConfig::TagType::VALUES;
				if (scp[it][tt][qt].enabled && fs[it][tt][qt].keys.count(keyId)) {
					assign(out, scp[it][tt][qt], item.value(i));
				}
				
				tt = (int) TextSearchConfig::TagType::TAGS;
				if (scp[it][tt][qt].enabled && fs[it][tt][qt].keys.count(keyId)) {
					assign(out, scp[it][tt][qt], std::string("@") + state.fs.keyId2Str.at(keyId) + ":" + item.value(i));
				}
			}
		}
	};
	
	typedef StringsExtractor<oscar_create::TextSearchConfig::QueryType::PREFIX> ExactStrings;
	typedef StringsExtractor<oscar_create::TextSearchConfig::QueryType::SUBSTRING> SuffixStrings;
	
	struct ItemId {
		static constexpr bool HasCellLocalIds = false;
		inline uint32_t operator()(const item_type & item) { return item.id(); }
		inline uint32_t operator()(const item_type & item, uint32_t /*cellId*/) { return (*this)(item); }
	};
	
	class ItemCells {
	public:
		ItemCells(const std::shared_ptr<OOM_SA_CTC_TraitsState> & state) : m_state(state) {}
		template<typename TOutputIterator>
		void operator()(const item_type & item, TOutputIterator out) {
			if (T_ITEM_TYPE == TextSearchConfig::ItemType::ITEM) {
				//this adds the region as item to the cell
				//This may not be what we really want
				//Since then a query to Stuttgart also yields Germany as an item of Stuttgart!
				if (false && item.isRegion()) {
					addFromGh(item, out);
				}
				else {
					addFromItem(item, out);
				}
			}
			else if (T_ITEM_TYPE == TextSearchConfig::ItemType::REGION) {
				if (item.isRegion()) {
					addFromGh(item, out);
				}
			}
		}
	private:
		template<typename TOutputIterator>
		void addFromItem(const item_type & item, TOutputIterator & out) {
			for(const auto & x : item.cells()) {
				*out = (uint32_t) x;
				++out;
			}
		}
		template<typename TOutputIterator>
		void addFromGh(const item_type & item, TOutputIterator & out) {
			//fetch the cells from the geohierarchy
			const auto & gh = m_state->gh;
			uint32_t ghId = gh.storeIdToGhId(item.id());
			uint32_t cellPtr = gh.regionCellIdxPtr(ghId);
			sserialize::ItemIndex cells(m_state->idxStore.at(cellPtr));
			using std::copy;
			copy(cells.begin(), cells.end(), out);
		}
	private:
		std::shared_ptr<OOM_SA_CTC_TraitsState> m_state;
	};
	
	class ForeignObjectsPredicate {
	private:
		bool m_withForeignObjects;
	public:
		ForeignObjectsPredicate(bool foreignMatches) : m_withForeignObjects(foreignMatches) {}
		bool operator()(const item_type &) {
			return T_ITEM_TYPE == TextSearchConfig::ItemType::REGION && m_withForeignObjects;
		}
	};
	
	class ForeignObjects {
	public:
		ForeignObjects(const std::shared_ptr<OOM_SA_CTC_TraitsState> & state) :
		m_state(state)
		{}
	public:
		template<typename TOutputIterator>
		void cells(const item_type & item, TOutputIterator out) {
			if (currentItemId() != item.id()) {
				extract(item);
			}
			cells(out);
		}
		template<typename TOutputIterator>
		void items(const item_type & item, uint32_t cellId, TOutputIterator out) {
			if (currentItemId() != item.id()) {
				extract(item);
			}
			items(cellId, out);
		}
	private:
		uint32_t currentItemId() const {
			return m_cItemId;
		}
		
		void extract(item_type const & item) {
			if (!item.isRegion()) {
				return;
			}
			uint32_t ghId = m_state->gh.storeIdToGhId(item.id());
			
			sserialize::ItemIndex regionCells(m_state->idxStore.at(m_state->gh.regionCellIdxPtr(ghId)));
			m_regionCells.insert(regionCells.begin(), regionCells.end());
			
			sserialize::ItemIndex regionItems;
			
			if (m_state->gh.hasRegionItems()) {
				regionItems = m_state->idxStore.at(m_state->gh.regionItemsPtr(ghId));
			}
			else {
				std::vector<sserialize::ItemIndex> tmp;
				for(uint32_t cellId : regionCells) {
					tmp.emplace_back(m_state->idxStore.at(m_state->gh.cellItemsPtr(cellId)));
				}
				regionItems = sserialize::ItemIndex::unite(tmp);
			}
			
			for(uint32_t itemId : regionItems) {
				auto itemCells = m_state->store.at(itemId).cells();
				for(uint32_t cellId : itemCells) {
					if (!m_regionCells.count(cellId)) {
						m_cell2Items[cellId].push_back(itemId);
					}
				}
			}
			
			m_regionCells.clear();
			m_cItemId = item.id();
		}
		
		template<typename TOutputIterator>
		void cells(TOutputIterator out) const {
			for(auto const & x : m_cell2Items) {
				*out = x.first;
				++out;
			}
		}
		
		template<typename TOutputIterator>
		void items(uint32_t cellId, TOutputIterator out) const {
			if (m_cell2Items.count(cellId)) {
				auto const & x = m_cell2Items.at(cellId);
				std::copy(x.begin(), x.end(), out);
			}
		}
	private:
		std::shared_ptr<OOM_SA_CTC_TraitsState> m_state;
		uint32_t m_cItemId{liboscar::Static::OsmKeyValueObjectStore::npos};
		std::unordered_set<uint32_t> m_regionCells;
		std::unordered_map<uint32_t, std::vector<uint32_t>> m_cell2Items;
	};
public:
	inline ExactStrings exactStrings() { return ExactStrings(m_state); }
	inline SuffixStrings suffixStrings() { return SuffixStrings(m_state); }
	inline ItemId itemId() { return ItemId(); }
	inline ItemCells itemCells() { return ItemCells(m_itemCellsState); }
	inline ForeignObjectsPredicate foreignObjectsPredicate() { return ForeignObjectsPredicate(m_itemCellsState->withForeignObjects); }
	inline ForeignObjects foreignObjects() { return ForeignObjects(m_itemCellsState); }
public:
	OOM_SA_CTC_Traits(const OOMGeoCellConfig & cfg, const liboscar::Static::OsmKeyValueObjectStore & store, const sserialize::Static::ItemIndexStore & idxStore) : 
	m_state(new State(store.kvStore(), cfg)),
	m_itemCellsState(new OOM_SA_CTC_TraitsState(store, idxStore, cfg.foreignObjects)) {}
	OOM_SA_CTC_Traits(const std::shared_ptr<State> & state) : m_state(state) {}
protected:
	inline const StateSharedPtr & state() const { return m_state; }
	inline StateSharedPtr & state() { return m_state; }
	inline const std::shared_ptr<OOM_SA_CTC_TraitsState> & itemCellsState() const { return m_itemCellsState; }
	inline std::shared_ptr<OOM_SA_CTC_TraitsState> & itemCellsState() { return m_itemCellsState; }
private:
	StateSharedPtr m_state;
	std::shared_ptr<OOM_SA_CTC_TraitsState> m_itemCellsState;
};

struct OOM_SA_CTC_CellLocalIds_TraitsState {
	sserialize::MMVector< std::pair<uint32_t, uint32_t> > localIds;
	sserialize::MMVector<uint64_t> itemId2LocalId;
	OOM_SA_CTC_CellLocalIds_TraitsState(const OOM_SA_CTC_CellLocalIds_TraitsState&) = delete;
	OOM_SA_CTC_CellLocalIds_TraitsState(OOM_SA_CTC_CellLocalIds_TraitsState&&) = delete;
	OOM_SA_CTC_CellLocalIds_TraitsState(const sserialize::Static::spatial::GeoHierarchy & gh, const sserialize::Static::ItemIndexStore & idxStore);
};

template<TextSearchConfig::ItemType T_ITEM_TYPE = TextSearchConfig::ItemType::ITEM>
class OOM_SA_CTC_CellLocalIds_Traits: public OOM_SA_CTC_Traits<T_ITEM_TYPE> {
public:
	using MyBaseClass = OOM_SA_CTC_Traits<T_ITEM_TYPE>;
	using typename MyBaseClass::item_type;
	using typename MyBaseClass::ExactStrings;
	using typename MyBaseClass::SuffixStrings;
	using typename MyBaseClass::ItemCells;
	
	class ItemId {
	public:
		static constexpr bool HasCellLocalIds = true;
	public:
		inline uint32_t operator()(const item_type & item) { return item.id(); }
		uint32_t operator()(const item_type & item, uint32_t cellId) {
			uint32_t id = item.id();
			uint64_t i = m_state->itemId2LocalId.at(id);
			uint64_t s =  id+1 < m_state->itemId2LocalId.size() ? m_state->itemId2LocalId.at(id+1) : m_state->localIds.size();
			for(; i < s; ++i) {
				const std::pair<uint32_t, uint32_t> & cId2lId = m_state->localIds[i];
				if (cId2lId.first == cellId) {
					return cId2lId.second;
				}
			}
			throw std::out_of_range("Could not find item " + std::to_string(id) + " in cell " + std::to_string(cellId));
			return 0;
		}
	public:
		ItemId(const std::shared_ptr<OOM_SA_CTC_CellLocalIds_TraitsState> & state) : m_state(state) {}
		~ItemId() {}
	private:
		std::shared_ptr<OOM_SA_CTC_CellLocalIds_TraitsState> m_state;
	};
public:
	using MyBaseClass::exactStrings;
	using MyBaseClass::suffixStrings;
	using MyBaseClass::itemCells;
	inline ItemId itemId() { return ItemId(m_cellLocalItemIds); }
public:
	OOM_SA_CTC_CellLocalIds_Traits(
		const OOMGeoCellConfig & tsc,
		const liboscar::Static::OsmKeyValueObjectStore & store,
		const sserialize::Static::ItemIndexStore & idxStore,
		const std::shared_ptr<OOM_SA_CTC_CellLocalIds_TraitsState> & cellLocalItemIds) : 
	MyBaseClass(tsc, store, idxStore),
	m_cellLocalItemIds(cellLocalItemIds)
	{}
private:
	std::shared_ptr<OOM_SA_CTC_CellLocalIds_TraitsState> m_cellLocalItemIds;
};

class SimpleSearchBaseTraits {
public:
	typedef liboscar::Static::OsmKeyValueObjectStore::Item item_type;
public:
	SimpleSearchBaseTraits(const TextSearchConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store);
	~SimpleSearchBaseTraits() {}
protected:
	typedef OOM_SA_CTC_Traits<TextSearchConfig::ItemType::ITEM> ItemSearchTraits;
	typedef OOM_SA_CTC_Traits<TextSearchConfig::ItemType::REGION> RegionSearchTraits;
	typedef ItemSearchTraits::ExactStrings ItemExactStrings;
	typedef ItemSearchTraits::SuffixStrings ItemSuffixStrings;
	typedef RegionSearchTraits::ExactStrings RegionExactStrings;
	typedef RegionSearchTraits::SuffixStrings RegionSuffixStrings;
protected:
	std::shared_ptr<BaseSearchTraitsState> m_state;
	ItemExactStrings m_ies;
	ItemSuffixStrings m_iss;
	RegionExactStrings m_res;
	RegionSuffixStrings m_rss;
};

class SimpleSearchTraits: public SimpleSearchBaseTraits {
public:
	typedef std::vector<std::string> value_type;
public:
	SimpleSearchTraits(const TextSearchConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store);
	~SimpleSearchTraits() {}
	void operator()(
		uint32_t itemId,
		sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer & exactStrings,
		sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer & suffixStrings
	) const;
protected:
	liboscar::Static::OsmKeyValueObjectStore m_store;
};

class InMemoryCTCSearchTraits: SimpleSearchBaseTraits {
public:
	typedef detail::CellTextCompleter::SampleItemStringsContainer StringsContainer;
public:
	InMemoryCTCSearchTraits(const GeoCellConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store);
	void operator()(const liboscar::Static::OsmKeyValueObjectStore::Item & item, StringsContainer & itemStrings, bool insertsAsItem) const;
protected:
	std::unordered_set<uint32_t> m_suffixDelimeters;
};

}//end namespace oscar_create


#endif
