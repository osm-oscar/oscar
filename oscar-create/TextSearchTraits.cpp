#include "TextSearchTraits.h"
#include <regex>

namespace oscar_create {

OOM_SA_CTC_Traits::OOM_SA_CTC_Traits(const TextSearchConfig& tsc, const liboscar::Static::OsmKeyValueObjectStore& store) :
m_state(new State())
{

}



OsmKeyValueObjectStoreDerfer::OsmKeyValueObjectStoreDerfer(const TextSearchConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store) :
m_store(store),
m_filter( std::make_shared< std::unordered_set<uint32_t> >() ),
m_tagPrefixSearchFilter( std::make_shared< std::unordered_map<uint32_t, std::string> >() ),
m_tagSuffixSearchFilter( std::make_shared< std::unordered_map<uint32_t, std::string> >() ),
m_largestId(0),
m_suffix(tsc.hasEnabled(TextSearchConfig::QueryType::SUBSTRING))
{
	std::ifstream file;
	auto keyStringTable = store.keyStringTable();

	auto keyFun = [&file,&keyStringTable](const std::string & fn, std::unordered_set<std::string> & dest) {
		file.open(fn);
		if (file.is_open()) {
			while (!file.eof()) {
				std::string key;
				std::getline(file, key);
				if (key.size()) {
					dest.insert(key);
				}
			}
			file.close();
		}
		else {
			std::cerr << "OsmKeyValueObjectStoreDerfer: Failed to open " << fn << std::endl;
		}
	};
	{
		std::unordered_set<std::string> keysToStore;
		if (!tsc.keyFile.empty()) {
			std::regex keysToStoreRegex;
			{
				keyFun(tsc.keyFile, keysToStore);
				std::string regexString("(");
				for(const std::string & x : keysToStore) {
					regexString += x + "|";
				}
				regexString.back() = ')';
				keysToStore.clear();
				
				keysToStoreRegex = std::regex(regexString);
			}
			
			for(uint32_t i = 0, s = keyStringTable.size(); i < s; ++i) {
				std::string t = keyStringTable.at(i);
				if (std::regex_match(t, keysToStoreRegex)) {
					m_filter->insert(i);
					m_largestId = std::max(m_largestId, i);
				}
			}
		}
		if (!tsc.storeTagsPrefixFile.empty()) {
			if (tsc.storeTagsPrefixFile == "all") {
				m_tagPrefixSearchFilter->reserve(keyStringTable.size());
				m_largestId = std::max(m_largestId, keyStringTable.size());
				for(uint32_t i = 0, s = keyStringTable.size(); i < s; ++i) {
					(*m_tagPrefixSearchFilter)[i] = keyStringTable.at(i);
				}
			}
			else {
				std::unordered_set<std::string> tagsToStore;
				keyFun(tsc.storeTagsPrefixFile, tagsToStore);
				for(uint32_t i = 0, s = keyStringTable.size(); i < s; ++i) {
					std::string t = keyStringTable.at(i);
					if (tagsToStore.count(t)) {
						(*m_tagPrefixSearchFilter)[i] = t;
						m_largestId = std::max(m_largestId, i);
					}
				}
			}
		}
		if (!tsc.storeTagsSuffixFile.empty()) {
			if (tsc.storeTagsSuffixFile == "all") {
				m_tagSuffixSearchFilter->reserve(keyStringTable.size());
				m_largestId = std::max(m_largestId, keyStringTable.size());
				for(uint32_t i = 0, s = keyStringTable.size(); i < s; ++i) {
					(*m_tagSuffixSearchFilter)[i] = keyStringTable.at(i);
				}
			}
			else {
				std::unordered_set<std::string> tagsToStore;
				keyFun(tsc.storeTagsSuffixFile, tagsToStore);
				for(uint32_t i = 0, s = keyStringTable.size(); i < s; ++i) {
					std::string t = keyStringTable.at(i);
					if (tagsToStore.count(t)) {
						(*m_tagSuffixSearchFilter)[i] = t;
						m_largestId = std::max(m_largestId, i);
					}
				}
			}
		}
	}
}

void OsmKeyValueObjectStoreDerfer::operator()(
	uint32_t itemId,
	sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer & prefixStrings,
	sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer & suffixStrings
) const
{
	throw sserialize::UnimplementedFunctionException("correct derefing not possible since inhertied strings are not correctly handled since addBoundaryInfo to region is gone");
	auto item = m_store.at(itemId);
	for(uint32_t i = 0, s = item.size(); i < s; ++i) {
		uint32_t keyId = item.keyId(i);
		if (m_filter->count(keyId) > 0) {
			std::string valueString = item.value(i);
			if (m_suffix) {
				suffixStrings.insert(valueString);
			}
			prefixStrings.insert(valueString);
		}
		if (m_tagPrefixSearchFilter->count(keyId) > 0) {
			std::string tmp = "@" + m_tagPrefixSearchFilter->at(keyId) + ":" + item.value(i);
			prefixStrings.insert(tmp);
		}
		if (m_tagSuffixSearchFilter->count(keyId) > 0) {
			std::string tmp = "@" + m_tagSuffixSearchFilter->at(keyId) + ":" + item.value(i);
			suffixStrings.insert(tmp);
		}
		if (m_largestId <= keyId) {
			break;
		}
	}
}

CellTextCompleterDerfer::CellTextCompleterDerfer(const GeoCellConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store) :
OsmKeyValueObjectStoreDerfer(tsc, store),
m_cfg(tsc)
{
	m_dr.init();
}

void CellTextCompleterDerfer::operator()(
	const liboscar::Static::OsmKeyValueObjectStore::Item & item,
	StringsContainer & itemStrings,
	bool insertsAsItem
) const
{
	TextSearchConfig::ItemType itemType = (insertsAsItem ? TextSearchConfig::ItemType::ITEM : TextSearchConfig::ItemType::REGION);
	for(uint32_t i = 0, s = item.size(); i < s; ++i) {
		uint32_t keyId = item.keyId(i);
		const TextSearchConfig::SearchCapabilities & caps = m_cfg.searchCapabilites[(int)itemType][(int)TextSearchConfig::TagType::VALUES][(int)TextSearchConfig::QueryType::PREFIX];
		if (m_filter->count(keyId) > 0) {
			
			std::string valueString = item.value(i);
			
			if (!caps.caseSensitive) {
				valueString = sserialize::unicode_to_lower(valueString);
			}
			if (m_suffix) {
				itemStrings.subString.push_back(valueString, m_cfg.suffixDelimeters);
			}
			else {
				itemStrings.prefixOnly.push_back(valueString);
			}
			if (caps.diacritcInSensitive) {
				m_dr.transliterate(valueString);
				if (m_suffix) {
					itemStrings.subString.push_back(valueString, m_cfg.suffixDelimeters);
				}
				else {
					itemStrings.prefixOnly.push_back(valueString);
				}
			}
		}
		if (insertsAsItem && m_tagPrefixSearchFilter->count(keyId) > 0) {
			std::string tmp = "@" + m_tagPrefixSearchFilter->at(keyId) + ":" + item.value(i);
			if (!caps.caseSensitive) {
				tmp = sserialize::unicode_to_lower(tmp);
			}
			itemStrings.prefixOnly.emplace_back(std::move(tmp));
		}
		else if (insertsAsItem && m_tagSuffixSearchFilter->count(keyId) > 0) {
			std::string tmp = "@" + m_tagSuffixSearchFilter->at(keyId) + ":" + item.value(i);
			if (!caps.caseSensitive) {
				tmp = sserialize::unicode_to_lower(tmp);
			}
			itemStrings.subString.push_back(tmp, m_cfg.suffixDelimeters);
		}
		else if (m_largestId <= keyId) {
			break;
		}
	}
}


}//end namespace oscar_create