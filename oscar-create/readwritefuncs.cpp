#include "readwritefuncs.h"
#include "CellTextCompleter.h"
#include <liboscar/GeoSearch.h>
#include <osmpbf/iway.h>
#include <osmpbf/parsehelpers.h>
#include <sserialize/utility/printers.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/stats/memusage.h>
#include <sserialize/Static/TrieNodePrivates/TrieNodePrivates.h>
#include <iostream>

using namespace std;
using namespace sserialize;

//TODO: stringfactory auf id basiertes umstellen, tagstore filter auf id basiert umstellen, hash in ItemIndexFactory verbessern (evtl. den kompletten index hashen)
//TODO:kvstore needs resizing

namespace oscar_create {

uint64_t getNumBlocks(const std::string & fileName) {
	osmpbf::OSMFileIn inFile(fileName, false);
	osmpbf::PrimitiveBlockInputAdaptor pbi;

	if (!inFile.open()) {
		std::cout << "Failed to open " <<  fileName << std::endl;
		return 0;
	}
	
	uint64_t blockCount = 0;
	while(inFile.skipBlock()) {
		++blockCount;
	}
	return blockCount;
}

bool findNodeIdBounds(const string & fileName, uint64_t & smallestId, uint64_t & largestId) {
	osmpbf::OSMFileIn inFile(fileName, false);
	if (!inFile.open()) {
		std::cout << "Failed to open " <<  fileName << std::endl;
		return false;
	}
	
	sserialize::AtomicMinMax<int64_t> minMaxId;
	std::atomic<uint64_t> numNodes(0);
	
	auto workFunc = [&minMaxId, &numNodes](osmpbf::PrimitiveBlockInputAdaptor & pbi) {
		sserialize::MinMax<int64_t> myMinMax;
		for(osmpbf::IWayStream way = pbi.getWayStream(); !way.isNull(); way.next()) {
			for(osmpbf::IWayStream::RefIterator it(way.refBegin()), end(way.refEnd()); it != end; ++it) {
				int64_t nId = std::max<int64_t>(*it, 0); //clip the id in case there are invalid entries (like negative ids)
				myMinMax.update(nId);
			}
		}
		minMaxId.update(myMinMax);
		numNodes += pbi.nodesSize();
	};
	
	osmpbf::parseFileCPPThreads(inFile, workFunc);
	
	largestId = std::max<int64_t>(minMaxId.max(), 0);
	smallestId = std::min<int64_t>(minMaxId.min(), 0);
	return largestId != smallestId;
}

UByteArrayAdapter ubaFromFC(const oscar_create::Options & opts, liboscar::FileConfig fc) {
	std::string fileName = liboscar::fileNameFromFileConfig(opts.getOutFileDir(), fc, false);
	return UByteArrayAdapter::openRo(fileName, false, MAX_SIZE_FOR_FULL_MMAP, CHUNKED_MMAP_EXPONENT);
}

liboscar::Static::OsmKeyValueObjectStore mangleAndWriteKv(oscar_create::OsmKeyValueObjectStore & store, const oscar_create::Options & opts) {
	std::cout << "Serializing KeyValueStore" << std::endl;
	UByteArrayAdapter dataBaseListAdapter(sserialize::UByteArrayAdapter::createFile(1024*1024, opts.getOutFileName(liboscar::FC_KV_STORE)) );
	UByteArrayAdapter idxFactoryData(sserialize::UByteArrayAdapter::createFile(1024*1024, opts.getOutFileName(liboscar::FC_INDEX)) );
	sserialize::ItemIndexFactory idxFactory;
	idxFactory.setType(opts.indexType);
	idxFactory.setIndexFile(idxFactoryData);
	idxFactory.setDeduplication(opts.indexDedup);
	sserialize::UByteArrayAdapter::OffsetType outSize = store.serialize(dataBaseListAdapter, idxFactory, opts.kvStoreConfig.fullRegionIndex);
	if (outSize < dataBaseListAdapter.size())
		dataBaseListAdapter.shrinkStorage(dataBaseListAdapter.size() - outSize);
	std::cout << "KeyValueStore serialized  with a length of " << outSize << std::endl;
	
	outSize = idxFactory.flush();
	if (outSize < idxFactory.getFlushedData().size())
		idxFactory.getFlushedData().shrinkStorage( idxFactory.getFlushedData().size() - outSize );
	
	std::cout << "Index serialized  with a length of " << outSize << std::endl;
	
	return liboscar::Static::OsmKeyValueObjectStore(dataBaseListAdapter);
}

struct TagStoreFilter {
	std::unordered_set<uint32_t> m_keysToStore;
	std::unordered_map<uint32_t, std::set<uint32_t> > m_keyValuesToStore;

	TagStoreFilter(const std::string & keysToStoreFn, const std::string & keyValsToStoreFn, const sserialize::Static::StringTable & keyStringTable, const sserialize::Static::StringTable & valueStringTable) {
		std::ifstream file;
		
		auto keyFun = [&file](const std::string & fn, std::unordered_set<std::string> & dest) {
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
				std::cerr << "OsmKeyValueObjectStore::TagStoreFilter: Failed to open " << fn << std::endl;
			}
		};
		
		auto keyValFun = [&file](const std::string & fn, std::unordered_map<std::string, std::set<std::string> > & dest) {
			file.open(fn);
			if (file.is_open()) {
				while (!file.eof()) {
					std::string key, value; 
					std::getline(file, key);
					std::getline(file, value);
					if (key.size() && value.size()) {
						dest[key].insert(value);
					}
				}
				file.close();
			}
			else {
				std::cerr << "OsmKeyValueObjectStore::TagStoreFilter: Failed to open " << fn << std::endl;
			}
		};
		std::unordered_set<std::string> keysToStore;
		std::unordered_map<std::string, std::set<std::string> > keyValuesToStore;
		
		std::cout << "TagStoreFilter: populating keys to store table" << std::endl;
		keyFun(keysToStoreFn, keysToStore);
		std::cout << "TagStoreFilter: populating key/values to store table" << std::endl;
		keyValFun(keyValsToStoreFn, keyValuesToStore);
		
		std::cout << "Remapping all tables to ids";
		{
			std::unordered_map<std::string, uint32_t> keysToId, valuesToId;
			for(const auto & key : keysToStore) {
				keysToId[key] = 0;
			}
			for(const auto & keyValues : keyValuesToStore) {
				keysToId[keyValues.first] = 0;
				for(const auto & value : keyValues.second) {
					valuesToId[value] = 0;
				}
			}
			
			for(uint32_t i = 0, s = keyStringTable.size(); i < s; ++i) {
				std::string key = keyStringTable.at(i);
				if (keysToId.count(key))
					keysToId[key] = i;
			}
			
			for(uint32_t i = 0, s = valueStringTable.size(); i < s; ++i) {
				std::string value = valueStringTable.at(i);
				if (valuesToId.count(value))
					valuesToId[value] = i;
			}
			for(const auto & key : keysToStore) {
				m_keysToStore.insert( keysToId.at(key) );
			}
			for(const auto & keyValues : keyValuesToStore) {
				std::set<uint32_t> & values = m_keyValuesToStore[ keysToId.at(keyValues.first) ];
				for(const auto & value : keyValues.second) {
					values.insert( valuesToId.at(value) );
				}
			}
		}
	}
	
	bool operator()(const std::pair<uint32_t, uint32_t> & kv) const {
		return operator()(kv.first, kv.second);
	}
	
	bool operator()(const uint32_t key,  const uint32_t value) const {
		return (m_keysToStore.count(key) > 0 || (m_keyValuesToStore.count(key) > 0 && m_keyValuesToStore.at(key).count(value) > 0 ));
	}

	void printStats(std::ostream & out) {
		out << "TagStoreFilter::stats::begin" << std::endl;
		out << "KeysToStore: " << m_keysToStore.size() << std::endl;
		out << "KeyValuesStore: " << m_keyValuesToStore.size() << std::endl;
		out << "TagStoreFilter::stats::end" << std::endl;
	}
};

struct OsmKeyValueObjectStoreDerfer {
	OsmKeyValueObjectStoreDerfer(const Options::TextSearchConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store) :
	m_store(store),
	m_filter( std::make_shared< std::unordered_set<uint32_t> >() ),
	m_tagPrefixSearchFilter( std::make_shared< std::unordered_map<uint32_t, std::string> >() ),
	m_tagSuffixSearchFilter( std::make_shared< std::unordered_map<uint32_t, std::string> >() ),
	m_largestId(0),
	m_suffix(tsc.suffixes)
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
				keyFun(tsc.keyFile, keysToStore);
				for(uint32_t i = 0, s = keyStringTable.size(); i < s; ++i) {
					std::string t = keyStringTable.at(i);
					if (keysToStore.count(t)) {
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
	typedef std::vector<std::string> value_type;
	liboscar::Static::OsmKeyValueObjectStore m_store;
	std::shared_ptr< std::unordered_set<uint32_t> > m_filter;
	std::shared_ptr< std::unordered_map<uint32_t, std::string> > m_tagPrefixSearchFilter;
	std::shared_ptr< std::unordered_map<uint32_t, std::string> > m_tagSuffixSearchFilter;
	uint32_t m_largestId;
	bool m_suffix;
	
	void operator()(uint32_t itemId, sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer & prefixStrings, sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer & suffixStrings) const {
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
};

struct CellTextCompleterDerfer: public OsmKeyValueObjectStoreDerfer {
	typedef detail::CellTextCompleter::SampleItemStringsContainer StringsContainer;
	CellTextCompleterDerfer(const Options::TextSearchConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store) :
	OsmKeyValueObjectStoreDerfer(tsc, store),
	inSensitive(!tsc.caseSensitive),
	diacriticInSensitive(tsc.diacritcInSensitive),
	seps(tsc.suffixDelimeters)
	{
		dr.init();
	}
	bool inSensitive;
	bool diacriticInSensitive;
	DiacriticRemover dr;
	std::unordered_set<uint32_t> seps;
	
	void operator()(const liboscar::Static::OsmKeyValueObjectStore::Item & item, StringsContainer & itemStrings, bool insertsAsItem) const {
		if (item.osmId() == 2922269) {
			item.print(std::cout, false);
		}
		for(uint32_t i = 0, s = item.size(); i < s; ++i) {
// 			std::string value = item.value(i);
// 			std::string key = item.key(i);
// 			std::cout << "key=" << key << "; value=" << value << std::endl;
			uint32_t keyId = item.keyId(i);
			if (m_filter->count(keyId) > 0) {
				std::string valueString = item.value(i);
				if (inSensitive) {
					valueString = sserialize::unicode_to_lower(valueString);
				}
				if (m_suffix) {
					itemStrings.subString.push_back(valueString, seps);
				}
				else {
					itemStrings.prefixOnly.push_back(valueString);
				}
				if (diacriticInSensitive) {
					dr.transliterate(valueString);
					if (m_suffix) {
						itemStrings.subString.push_back(valueString, seps);
					}
					else {
						itemStrings.prefixOnly.push_back(valueString);
					}
				}
			}
			if (insertsAsItem && m_tagPrefixSearchFilter->count(keyId) > 0) {
				std::string tmp = "@" + m_tagPrefixSearchFilter->at(keyId) + ":" + item.value(i);
				if (inSensitive) {
					tmp = sserialize::unicode_to_lower(tmp);
				}
				itemStrings.prefixOnly.emplace_back(std::move(tmp));
			}
			else if (insertsAsItem && m_tagSuffixSearchFilter->count(keyId) > 0) {
				std::string tmp = "@" + m_tagSuffixSearchFilter->at(keyId) + ":" + item.value(i);
				if (inSensitive) {
					tmp = sserialize::unicode_to_lower(tmp);
				}
				itemStrings.subString.push_back(tmp, seps);
			}
			else if (m_largestId <= keyId) {
				break;
			}
		}
		if (item.osmId() == 2922269) {
			std::cout << std::endl;
			std::cout << "Inserting the following strings as prefix tags" << std::endl;
			std::cout << itemStrings.prefixOnly << std::endl;
		}
	}
};

void createTagStore(liboscar::Static::OsmKeyValueObjectStore & store, Options & opts, ItemIndexFactory & indexFactory) {
	std::cout << "Doing a sanitycheck before processing..." << std::flush;
	if (store.sanityCheck()) {
		std::cout << "OK" << std::endl;
	}
	else {
		std::cout << "FAILED" << std::endl;
		return;
	}

	ProgressInfo info;
	std::cout << "Caching string tables to memory..." << std::flush;
	std::vector<std::string> keyStringTable(store.keyStringTable().cbegin(), store.keyStringTable().cend());
	std::vector<std::string> valueStringTable(store.valueStringTable().cbegin(), store.valueStringTable().cend());
	std::cout << "done" << std::endl;
	
	oscar_create::TagStore tagStore;
	TagStoreFilter tagStoreFilter(opts.tagStoreConfig.tagKeys, opts.tagStoreConfig.tagKeyValues, store.keyStringTable(), store.valueStringTable());
	info.begin(store.size(), "Creating TagStore");
	for(uint32_t i = 0, s = store.size(); i < s; ++i) {
		liboscar::Static::OsmKeyValueObjectStore::Item item = store.at(i);
		for(uint32_t j = 0, js = item.size(); j < js; ++j) {
			uint32_t keyId = item.keyId(j);
			uint32_t valueId = item.valueId(j);
			if ( tagStoreFilter(keyId, valueId) ) {
				tagStore.insert(keyStringTable.at(keyId), valueStringTable.at(valueId), i);
			}
		}
		info(i);
	}
	info.end();
	std::cout << "TagStore node count:" << tagStore.nodeCount() << std::endl;
	
	UByteArrayAdapter tagStoreAdapter(UByteArrayAdapter::createFile(1, opts.getOutFileName(liboscar::FC_TAGSTORE)));

	std::cout << "Serializing TagStore" << std::endl;	
	tagStore.serialize(tagStoreAdapter, indexFactory);
	std::cout << "TagStore serialized" << std::endl;
}

void
handleSimpleTextSearch(
const std::pair<liboscar::TextSearch::Type, Options::TextSearchConfig> & x, Options & opts,
liboscar::Static::OsmKeyValueObjectStore & store, ItemIndexFactory & indexFactory, sserialize::UByteArrayAdapter & dest) {
	OsmKeyValueObjectStoreDerfer itemsDerefer(x.second, store);
	uint32_t itemsBegin = 0;
	uint32_t itemsEnd = 0;
	if (x.first == liboscar::TextSearch::GEOHIERARCHY) {
		itemsBegin = 0;
		itemsEnd = store.geoHierarchy().regionSize();
	}
	else {
		itemsBegin = 0;
		itemsEnd = store.size();
	}
	std::deque<uint8_t> treeList;

	if (x.second.type == Options::TextSearchConfig::Type::FULL_INDEX_TRIE ) {
		sserialize::GeneralizedTrie::SinglePassTrie * tree = new sserialize::GeneralizedTrie::SinglePassTrie();
		GeneralizedTrie::GeneralizedTrieCreatorConfig trieCfg = opts.toTrieConfig(x.second);

		trieCfg.trieList = &treeList;
		trieCfg.indexFactory = &indexFactory;

		tree->setCaseSensitivity(false);
		tree->setAddTransliteratedDiacritics(false);

		tree->fromStringsFactory<OsmKeyValueObjectStoreDerfer>(itemsDerefer, itemsBegin, itemsEnd, x.second.mmType);
		tree->createStaticTrie(trieCfg);

		delete tree;
	}
	//other tries are not supported anymore
	dest.putData(treeList);
}

template<typename TCT>
void
handleCellTextSearchBase(
const std::pair<liboscar::TextSearch::Type, Options::TextSearchConfig> & x,
liboscar::Static::OsmKeyValueObjectStore & store, const sserialize::Static::ItemIndexStore & idxStore,
ItemIndexFactory & indexFactory, sserialize::UByteArrayAdapter & dest) {
	TCT ct(x.second.mmType);
	ct.create(store, idxStore, CellTextCompleterDerfer(x.second, store));
	sserialize::UByteArrayAdapter::OffsetType bO = dest.tellPutPtr();
	uint32_t sq = (x.second.suffixes ? sserialize::StringCompleter::SQ_EPSP : sserialize::StringCompleter::SQ_EP);
	sq |= (x.second.caseSensitive ? sserialize::StringCompleter::SQ_CASE_SENSITIVE: sserialize::StringCompleter::SQ_CASE_INSENSITIVE);
	ct.append((sserialize::StringCompleter::SupportedQuerries) sq, dest, indexFactory, x.second.threadCount);
	if (x.second.extensiveChecking) {
		std::cout << "Checking CellTextCompleter for equality" << std::endl;
		sserialize::UByteArrayAdapter tmp = dest;
		tmp.setPutPtr(bO);
		tmp.shrinkToPutPtr();
		tmp.resetPtrs();
		liboscar::Static::CellTextCompleter sct( tmp, sserialize::Static::ItemIndexStore(), sserialize::Static::spatial::GeoHierarchy(), sserialize::Static::spatial::TriangulationGeoHierarchyArrangement());
		if (ct.equal(sct, [&indexFactory](uint32_t id){ return indexFactory.indexById(id);})) {
			std::cout << "CellTextCompleter is equal" << std::endl;
		}
		else {
			std::cerr << "CellTextCompleter is NOT equal!" << std::endl;
		}
	}
}

void
handleCellTextSearch(
const std::pair<liboscar::TextSearch::Type, Options::TextSearchConfig> & x,
liboscar::Static::OsmKeyValueObjectStore & store, const sserialize::Static::ItemIndexStore & idxStore,
ItemIndexFactory & indexFactory, sserialize::UByteArrayAdapter & dest) {
	if (x.second.type == Options::TextSearchConfig::Type::FLAT_TRIE) {
		handleCellTextSearchBase< oscar_create::CellTextCompleterFlatTrie>(x, store, idxStore, indexFactory, dest);
	}
	else if (x.second.type == Options::TextSearchConfig::Type::TRIE) {
		handleCellTextSearchBase< oscar_create::CellTextCompleterUnicodeTrie>(x, store, idxStore, indexFactory, dest);
	}
	else {
		std::cout << "CellTextSearch does not support the requested trie type" << std::endl;
	}
}

void handleTextSearch(
liboscar::Static::OsmKeyValueObjectStore & store,
Options & opts, ItemIndexFactory & indexFactory, const sserialize::Static::ItemIndexStore & idxStore) {
	std::string fn = liboscar::fileNameFromFileConfig(opts.getOutFileDir(), liboscar::FC_TEXT_SEARCH, false);
	sserialize::UByteArrayAdapter dest( sserialize::UByteArrayAdapter::createFile(1, fn) );
	if (dest.size() != 1) {
		std::cout << "Failed to create file for text search" << std::endl;
		return;
	}
	liboscar::TextSearchCreator tsCreator(dest);
	
	for(const std::pair<liboscar::TextSearch::Type, Options::TextSearchConfig> & x : opts.textSearchConfig) {
		tsCreator.beginRawPut(x.first);
		if (x.first == liboscar::TextSearch::GEOCELL) {
			handleCellTextSearch(x, store, idxStore, indexFactory, tsCreator.rawPut());
		}
		else if (x.first == liboscar::TextSearch::GEOHIERARCHY_AND_ITEMS) {
			tsCreator.rawPut().putUint8(1); //Version
			auto tmp(x);
			tmp.first = liboscar::TextSearch::GEOHIERARCHY;
			std::cout << "Creating the GeoHierarchy completer for the GeoHierarchyWithItems TextSearch" << std::endl;
			handleSimpleTextSearch(tmp, opts, store, indexFactory, tsCreator.rawPut());
			tmp.first = liboscar::TextSearch::ITEMS;
			std::cout << "Creating the Items completer for the GeoHierarchyWithItems TextSearch" << std::endl;
			handleSimpleTextSearch(tmp, opts, store, indexFactory, tsCreator.rawPut());
			tsCreator.endRawPut();
		}
		else {
			handleSimpleTextSearch(x, opts, store, indexFactory, tsCreator.rawPut());
		}
		
		tsCreator.endRawPut();
	}
	tsCreator.flush();
}

void handleGeoSearch(liboscar::Static::OsmKeyValueObjectStore & store, Options & opts, ItemIndexFactory & indexFactory) {
	std::string fn = liboscar::fileNameFromFileConfig(opts.getOutFileDir(), liboscar::FC_GEO_SEARCH, false);
	sserialize::UByteArrayAdapter dest( sserialize::UByteArrayAdapter::createFile(1, fn) );
	if (dest.size() != 1) {
		std::cout << "Failed to create file for text search" << std::endl;
		return;
	}
	liboscar::GeoSearchCreator gsCreator(dest);
	if (opts.gridLatCount != 0 && opts.gridLonCount != 0) {
		gsCreator.beginRawPut(liboscar::GeoSearch::ITEMS);
		if (createAndWriteGrid(opts, store, indexFactory, gsCreator.rawPut()))
			std::cout << "Created grid" << std::endl;
		else
			std::cout << "Failed to create grid" << std::endl;
		gsCreator.endRawPut();
	}
	
	if (opts.gridRTreeLatCount != 0 && opts.gridRTreeLonCount != 0) {
		gsCreator.beginRawPut(liboscar::GeoSearch::ITEMS);
		if (createAndWriteGridRTree(opts, store, indexFactory, gsCreator.rawPut()))
			std::cout << "Created grid" << std::endl;
		else
			std::cout << "Failed to create grid" << std::endl;
		gsCreator.endRawPut();
	}
	gsCreator.flush();

}

void handleKv(liboscar::Static::OsmKeyValueObjectStore & store, Options & opts) {
	sserialize::TimeMeasurer totalTime; totalTime.begin();
	sserialize::ItemIndexFactory indexFactory;
	indexFactory.setCheckIndex(opts.checkIndex);;
	indexFactory.setType(opts.indexType);
	indexFactory.setIndexFile( opts.indexFile );
	if (opts.inputIndexStore.empty()) {
		std::cout << "No ItemIndexStore given containing gh data given" << std::endl;
		return;
	}
	sserialize::Static::ItemIndexStore inputIndexStore;
	{
		std::cout << "Using index at " << opts.inputIndexStore << " as base for ItemIndexFactory" << std::endl;
		inputIndexStore = sserialize::Static::ItemIndexStore(UByteArrayAdapter::openRo(opts.inputIndexStore, false));
		std::vector<uint32_t> tmp = indexFactory.insert(inputIndexStore);
		for(uint32_t i = 0, s = tmp.size(); i < s; ++i) {
			if (i != tmp[i])  {
				std::cout << "ItemIndexFactory::insert is broken" << std::endl;
				std::cout << tmp << std::endl;
				return;
			}
		}
	}
	//disable deduplication after inserting index as otherwise index ids change since the null index is always part of an store
	indexFactory.setDeduplication(opts.indexDedup);

	if (opts.memUsage)
		sserialize::MemUsage().print();

	if (opts.tagStoreConfig.create)  {
		createTagStore(store, opts, indexFactory);
	}
	
	if ((opts.gridLatCount != 0 && opts.gridLonCount != 0) || (opts.gridRTreeLatCount != 0 && opts.gridRTreeLonCount != 0)) {
		handleGeoSearch(store, opts, indexFactory);
	}

	
	if (opts.memUsage)
		sserialize::MemUsage().print();
	
	if (opts.textSearchConfig.size()) {
		handleTextSearch(store, opts, indexFactory, inputIndexStore);
	}
	
	writeItemIndexFactory(indexFactory);
	
	if (opts.memUsage)
		sserialize::MemUsage().print();
	
	totalTime.end();
	std::cout << "Total time spent to handle KVS: " << totalTime << std::endl;
}

bool writeItemIndexFactory(sserialize::ItemIndexFactory & indexFactory) {
	
	std::cout << "Serializing index" << std::endl;
	OffsetType indexFlushedLength = indexFactory.flush();
	std::cout << "Index size: " << indexFlushedLength << std::endl;
	if (indexFlushedLength) {
		if (indexFlushedLength < 1024*1024 || indexFlushedLength < indexFactory.getFlushedData().size()) {
			UByteArrayAdapter indexStorage = indexFactory.getFlushedData();
			indexFactory = sserialize::ItemIndexFactory();
			if (!indexStorage.shrinkStorage(indexStorage.size() - indexFlushedLength) || indexStorage.size() != indexFlushedLength) {
				std::cout << "Failed to shrink index to correct size. Should=" << indexFlushedLength << " is " << indexStorage.size() << std::endl;
			}
			else {
				std::cout << "Shrinking index to correct size succeeded" << std::endl;
			}
		}
	}
	else {
		std::cout <<  "Failed to serialize index" << std::endl;
	}
	return true;
}
} //end namespace