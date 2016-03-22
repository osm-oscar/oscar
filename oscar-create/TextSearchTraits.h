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
	sserialize::Static::spatial::GeoHierarchy gh;
	sserialize::Static::ItemIndexStore idxStore;
	OOM_SA_CTC_TraitsState(const sserialize::Static::spatial::GeoHierarchy & gh, const sserialize::Static::ItemIndexStore & idxStore) :
	gh(gh),
	idxStore(idxStore)
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
	protected:
		template<typename TOutputIterator>
		void assign(TOutputIterator & out, const TextSearchConfig::SearchCapabilities & cap, const std::string & str) const {
			if (cap.caseSensitive) {
				*out = str;
				++out;
				if (cap.diacritcInSensitive) {
					*out = m_state->dr(str);
					++out;
				}
			}
			else {
				std::string tmp = sserialize::unicode_to_lower(str);
				*out = tmp;
				++out;
				if (cap.diacritcInSensitive) {
					*out = m_state->dr(tmp);
					++out;
				}
			}
		}
	public:
		StringsExtractor(const std::shared_ptr<State> & state) : m_state(state) {}
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
		inline uint32_t operator()(const item_type & item) { return item.id(); }
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
				*out = x;
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
public:
	inline ExactStrings exactStrings() { return ExactStrings(m_state); }
	inline SuffixStrings suffixStrings() { return SuffixStrings(m_state); }
	inline ItemId itemId() { return ItemId(); }
	inline ItemCells itemCells() { return ItemCells(m_itemCellsState); }
public:
	OOM_SA_CTC_Traits(const TextSearchConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store, const sserialize::Static::ItemIndexStore & idxStore) : 
	m_state(new State(store.kvStore(), tsc)),
	m_itemCellsState(new OOM_SA_CTC_TraitsState(store.geoHierarchy(), idxStore)) {}
	OOM_SA_CTC_Traits(const std::shared_ptr<State> & state) : m_state(state) {}
private:
	StateSharedPtr m_state;
	std::shared_ptr<OOM_SA_CTC_TraitsState> m_itemCellsState;
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