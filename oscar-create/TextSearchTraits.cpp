#include "TextSearchTraits.h"
#include "helpers.h"
#include <regex>
#include <unordered_set>

namespace oscar_create {


FilterState::SingleFilter::SingleFilter(const TextSearchConfig::SearchCapabilities & src, const sserialize::Static::KeyValueObjectStore & kv) {
	if (!src.enabled || !sserialize::MmappedFile::fileExists(src.fileName)) {
		return;
	}
	std::unordered_set<std::string> keysToStore;

	readKeysFromFile(src.fileName, std::inserter(keysToStore, keysToStore.end()));
	
	if (!keysToStore.size()) {
		return;
	}

	auto keyStringTable = kv.keyStringTable();
	
	std::regex keysToStoreRegex;
	{
		std::string regexString("(");
		for(const std::string & x : keysToStore) {
			regexString += x + "|";
		}
		regexString.back() = ')';
		keysToStore.clear();
		
		keysToStoreRegex = std::regex(regexString);
	}
	
	for(uint32_t i = 0, s = (uint32_t) keyStringTable.size(); i < s; ++i) {
		std::string t = keyStringTable.at(i);
		if (std::regex_match(t, keysToStoreRegex, std::regex_constants::match_any)) {
			this->keys.insert(i);
		}
	}
}

FilterState::FilterState(const sserialize::Static::KeyValueObjectStore& kv, const TextSearchConfig& cfg) {
	for(uint32_t itemType(0); itemType < (int) TextSearchConfig::ItemType::ITEM_TYPE_COUNT; ++itemType) {
		for(uint32_t tagType(0); tagType < (int) TextSearchConfig::TagType::TAG_TYPE_COUNT; ++tagType) {
			for(uint32_t queryType(0); queryType < (int) TextSearchConfig::QueryType::QUERY_TYPE_COUNT; ++queryType) {
				filterInfo[itemType][tagType][queryType] = SingleFilter(cfg.searchCapabilites[itemType][tagType][queryType], kv);
			}
		}
	}
	for(uint32_t itemType(0); itemType < (int) TextSearchConfig::ItemType::ITEM_TYPE_COUNT; ++itemType) {
		for(uint32_t queryType(0); queryType < (int) TextSearchConfig::QueryType::QUERY_TYPE_COUNT; ++queryType) {
			const SingleFilter & sf = filterInfo[itemType][(int)TextSearchConfig::TagType::TAGS][queryType];
			for(uint32_t x : sf.keys) {
				keyId2Str[x];
			}
		}
	}
	for(auto & x : keyId2Str) {
		x.second = kv.keyStringTable().at(x.first);
	}
}

OOM_SA_CTC_CellLocalIds_TraitsState::OOM_SA_CTC_CellLocalIds_TraitsState(
	const sserialize::Static::spatial::GeoHierarchy& gh,
	const sserialize::Static::ItemIndexStore& idxStore
)
{
	std::cout << "Calculating cell local item ids..." << std::flush;
	//compute number of entries per item
	std::vector<uint32_t> tmp;
	for(uint32_t cellId(0), cs(gh.cellSize()); cellId < cs; ++cellId) {
		uint32_t itemIdxId = gh.cellItemsPtr(cellId);
		sserialize::ItemIndex itemIdx( idxStore.at(itemIdxId) );
		
		
		itemIdx.putInto(tmp);
		for(uint32_t itemId : tmp) {
			if (itemId2LocalId.size() <= itemId) {
				itemId2LocalId.resize(itemId+1, 0);
			}
			itemId2LocalId[itemId] += 1;
		}
		tmp.clear();
	}
	
	uint64_t itemBeginOffset = 0;
	for(uint32_t itemId(0), is(itemId2LocalId.size()); itemId < is; ++itemId) {
		uint64_t itemCellCount = itemId2LocalId[itemId];
		itemId2LocalId[itemId] = itemBeginOffset;
		itemBeginOffset += itemCellCount;
	}
	localIds.resize(itemBeginOffset);
	
	//now compute the map
	for(uint32_t cellId(0), cs(gh.cellSize()); cellId < cs; ++cellId) {
		uint32_t itemIdxId = gh.cellItemsPtr(cellId);
		sserialize::ItemIndex itemIdx( idxStore.at(itemIdxId) );
		
		
		itemIdx.putInto(tmp);
		for(uint32_t localId(0), ls(tmp.size()); localId < ls; ++localId) {
			uint32_t itemId = tmp[localId];
			uint64_t & off = itemId2LocalId[itemId];
			std::pair<uint32_t, uint32_t> & cId2lId = localIds.at(off);
			cId2lId.first = cellId;
			cId2lId.second = localId;
			++off;
		}
		tmp.clear();
	}
	if (itemId2LocalId.size()) {
		//the first entry points to the second, second to the beginning of the third and so on, so shift them accordingly
		for(uint32_t i(itemId2LocalId.size()-1); i > 0; --i) {
			itemId2LocalId[i] = itemId2LocalId[i-1];
		}
		itemId2LocalId.at(0) = 0;
	}
	std::cout << "done" << std::endl;
}


SimpleSearchBaseTraits::SimpleSearchBaseTraits(const TextSearchConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store) : 
m_state(new BaseSearchTraitsState(store.kvStore(), tsc)),
m_ies(m_state),
m_iss(m_state),
m_res(m_state),
m_rss(m_state)
{}

SimpleSearchTraits::SimpleSearchTraits(const TextSearchConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store) :
SimpleSearchBaseTraits(tsc, store),
m_store(store)
{}

void SimpleSearchTraits::operator()(
	uint32_t itemId,
	sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer & exactStrings,
	sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer & suffixStrings
) const
{
	std::insert_iterator< sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer > eIt(exactStrings, exactStrings.end());
	std::insert_iterator< sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer > sIt(suffixStrings, suffixStrings.end());

	auto gh = m_store.geoHierarchy();
	//first insert the item strings, then the strings inherited from regions
	item_type item = m_store.at(itemId);
	m_ies(item, eIt);
	m_iss(item, sIt);
	for(auto cellId : item.cells()) {
		auto cell = gh.cell((uint32_t) cellId);
		for(uint32_t i(0), s(cell.parentsSize()); i < s; ++i) {
			uint32_t parentGhId = cell.parent(i);
			uint32_t parentStoreId = gh.ghIdToStoreId(parentGhId);
			item_type parentItem = m_store.at(parentStoreId);
			m_res(parentItem, eIt);
			m_rss(parentItem, sIt);
		}
	}
}


InMemoryCTCSearchTraits::InMemoryCTCSearchTraits(const GeoCellConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store) :
SimpleSearchBaseTraits(tsc, store),
m_suffixDelimeters(tsc.suffixDelimeters)
{}

void InMemoryCTCSearchTraits::operator()(const item_type & item, StringsContainer & itemStrings, bool insertsAsItem) const {
	std::unordered_set<std::string> strs;
	std::insert_iterator< std::unordered_set<std::string> > inserter(strs, strs.end());
	if (insertsAsItem) {
		m_ies(item, inserter);
		itemStrings.prefixOnly.assign(strs.begin(), strs.end());
		strs.clear();
		m_iss(item, inserter);
		for(const std::string & str : strs) {
			itemStrings.subString.push_back(str, m_suffixDelimeters);
		}
	}
	else {
		m_res(item, inserter);
		itemStrings.prefixOnly.assign(strs.begin(), strs.end());
		strs.clear();
		m_rss(item, inserter);
		for(const std::string & str : strs) {
			itemStrings.subString.push_back(str, m_suffixDelimeters);
		}
	}
}

}//end namespace oscar_create