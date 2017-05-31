#include "readwritefuncs.h"
#include <liboscar/GeoSearch.h>
#include <osmpbf/iway.h>
#include <osmpbf/parsehelpers.h>
#include <sserialize/utility/printers.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/stats/memusage.h>
#include <sserialize/Static/TrieNodePrivates/TrieNodePrivates.h>
#include <sserialize/spatial/ItemGeoGrid.h>
#include <sserialize/spatial/GridRTree.h>
#include <sserialize/search/OOMSACTCCreator.h>
#include <iostream>
#include <regex>
#include <liboscar/constants.h>

#include "CellTextCompleter.h"
#include "OsmKeyValueObjectStore.h"
#include "helpers.h"
#include "AreaExtractor.h"
#include "TextSearchTraits.h"

namespace oscar_create {

struct ItemBoundaryGenerator {
	liboscar::Static::OsmKeyValueObjectStore * c;
	uint32_t p;
	ItemBoundaryGenerator(liboscar::Static::OsmKeyValueObjectStore * c) : c(c), p(0) {}
	inline bool valid() const { return p < c->size(); }
	inline uint32_t id() const { return p; }
	inline sserialize::Static::spatial::GeoShape shape() {
		return c->geoShape(p);
	}
	inline void next() { ++p;}
};

bool createAndWriteGrid(const oscar_create::Config& opts, State & state, sserialize::UByteArrayAdapter & dest) {
	if (!opts.gridConfig || opts.gridConfig->latCount == 0 || opts.gridConfig->lonCount == 0) {
		return false;
	}
	std::cout << "Finding grid dimensions..." << std::flush;
	sserialize::spatial::GeoRect rect( state.store.boundary() );
	sserialize::spatial::ItemGeoGrid grid(rect, opts.gridConfig->latCount, opts.gridConfig->lonCount);
	std::cout << "Creating Grid" << std::endl;
	sserialize::ProgressInfo info;
	info.begin(state.store.size());
	bool allOk = true;
	for(uint32_t i = 0; i < state.store.size(); i++) {
		allOk = grid.addItem(i, state.store.geoShape(i)) && allOk;
		info(i);
	}
	info.end("Done Creating grid");
	std::cout <<  "Serializing Grid" << std::endl;
	std::vector<uint8_t> dataStorage;
	dataStorage.resize(opts.gridConfig->latCount*opts.gridConfig->lonCount*4);
	sserialize::UByteArrayAdapter dataAdap(&dataStorage);
	
	grid.serialize(dataAdap, state.indexFactory);
	std::cout << "Grid serialized" << std::endl;
	dest.putData(dataStorage);
	return allOk;
}

bool createAndWriteGridRTree(const oscar_create::Config& opts, State & state, sserialize::UByteArrayAdapter & dest) {
	if (opts.rTreeConfig || opts.rTreeConfig->latCount == 0 || opts.rTreeConfig->lonCount == 0) {
		return false;
	}
	std::cout << "Finding grid dimensions..." << std::flush;
	sserialize::spatial::GeoRect rect( state.store.boundary() );
	sserialize::spatial::GridRTree grid(rect, opts.rTreeConfig->latCount, opts.rTreeConfig->lonCount);
	ItemBoundaryGenerator generator(&state.store);
	grid.bulkLoad(generator);
	std::cout << "Creating GridRTree" << std::endl;
	std::cout <<  "Serializing GridRTree" << std::endl;
	std::vector<uint8_t> dataStorage;
	sserialize::UByteArrayAdapter dataAdap(&dataStorage);
	
	grid.serialize(dataAdap, state.indexFactory);
	std::cout << "GridRTree serialized" << std::endl;
	dest.putData(dataStorage);
	return true;
}

void handleGeoSearch(Config & opts, State & state) {
	if (!(
		(opts.gridConfig && opts.gridConfig->enabled) ||
		(opts.rTreeConfig && opts.rTreeConfig->enabled)
	))
	{
		return;
	}
	
	std::string fn = liboscar::fileNameFromFileConfig(opts.getOutFileDir(), liboscar::FC_GEO_SEARCH, false);
	sserialize::UByteArrayAdapter dest( sserialize::UByteArrayAdapter::createFile(1, fn) );
	if (dest.size() != 1) {
		std::cout << "Failed to create file for text search" << std::endl;
		return;
	}
	liboscar::GeoSearchCreator gsCreator(dest);
	if (opts.gridConfig && opts.gridConfig->enabled) {
		gsCreator.beginRawPut(liboscar::GeoSearch::ITEMS);
		if (createAndWriteGrid(opts, state, gsCreator.rawPut())) {
			std::cout << "Created grid" << std::endl;
		}
		else {
			std::cout << "Failed to create grid" << std::endl;
		}
		gsCreator.endRawPut();
	}
	
	if (opts.rTreeConfig && opts.rTreeConfig->enabled) {
		gsCreator.beginRawPut(liboscar::GeoSearch::ITEMS);
		if (createAndWriteGridRTree(opts, state, gsCreator.rawPut())) {
			std::cout << "Created rtree" << std::endl;
		}
		else {
			std::cout << "Failed to create grid" << std::endl;
		}
		gsCreator.endRawPut();
	}
	gsCreator.flush();
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
			
			for(uint32_t i = 0, s = (uint32_t) keyStringTable.size(); i < s; ++i) {
				std::string key = keyStringTable.at(i);
				if (keysToId.count(key))
					keysToId[key] = i;
			}
			
			for(uint32_t i = 0, s = (uint32_t) valueStringTable.size(); i < s; ++i) {
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


void createTagStore(Config & opts, State & state) {
	if (!opts.tagStoreConfig) {
		return;
	}

	sserialize::ProgressInfo info;
	std::cout << "Caching string tables to memory..." << std::flush;
	std::vector<std::string> keyStringTable(state.store.keyStringTable().cbegin(), state.store.keyStringTable().cend());
	std::vector<std::string> valueStringTable(state.store.valueStringTable().cbegin(), state.store.valueStringTable().cend());
	std::cout << "done" << std::endl;
	
	oscar_create::TagStore tagStore;
	TagStoreFilter tagStoreFilter(opts.tagStoreConfig->tagKeys, opts.tagStoreConfig->tagKeyValues, state.store.keyStringTable(), state.store.valueStringTable());
	info.begin(state.store.size(), "Creating TagStore");
	for(uint32_t i = 0, s = state.store.size(); i < s; ++i) {
		liboscar::Static::OsmKeyValueObjectStore::Item item = state.store.at(i);
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
	
	sserialize::UByteArrayAdapter tagStoreAdapter(sserialize::UByteArrayAdapter::createFile(1, opts.getOutFileName(liboscar::FC_TAGSTORE)));

	std::cout << "Serializing TagStore" << std::endl;	
	tagStore.serialize(tagStoreAdapter, state.indexFactory);
	std::cout << "TagStore serialized" << std::endl;
}

void handleSimpleTextSearch(ItemSearchConfig & cfg, State & state, sserialize::UByteArrayAdapter & dest) {
	SimpleSearchTraits itemsDerefer(cfg, state.store);
	uint32_t itemsBegin = 0;
	uint32_t itemsEnd = 0;
	if (cfg.type == liboscar::TextSearch::GEOHIERARCHY) {
		itemsBegin = 0;
		itemsEnd = state.store.geoHierarchy().regionSize();
	}
	else {
		itemsBegin = 0;
		itemsEnd = state.store.size();
	}
	std::deque<uint8_t> treeList;

	if (cfg.trieType == ItemSearchConfig::TrieType::FULL_INDEX_TRIE ) {
		sserialize::GeneralizedTrie::SinglePassTrie * tree = new sserialize::GeneralizedTrie::SinglePassTrie();
		sserialize::GeneralizedTrie::GeneralizedTrieCreatorConfig trieCfg;
		
		trieCfg.trieList = &treeList;
		trieCfg.indexFactory = &(state.indexFactory);
		trieCfg.mergeIndex = cfg.mergeIndex;
		trieCfg.levelsWithoutFullIndex = cfg.levelsWithoutIndex;
		trieCfg.maxPrefixMergeCount = cfg.maxPrefixIndexMergeCount;
		trieCfg.maxSuffixMergeCount = cfg.maxSuffixIndexMergeCount;
		trieCfg.nodeType = cfg.nodeType;
		
		//handle case-sensitivity and translitation in derefer
		tree->setCaseSensitivity(false);
		tree->setAddTransliteratedDiacritics(false);

		tree->fromStringsFactory<decltype(itemsDerefer)>(itemsDerefer, itemsBegin, itemsEnd, cfg.mmType);
		tree->createStaticTrie(trieCfg);

		delete tree;
	}
	else {
		std::cerr << "Other trie types are not supported" << std::endl;
	}
	dest.putData(treeList);
}

template<typename TCT>
void
handleCellTextSearchBase(GeoCellConfig & cfg, State & state, sserialize::UByteArrayAdapter & dest) {
	TCT ct(cfg.mmType);
	sserialize::Static::ItemIndexStore idxStore( state.indexFactory.asItemIndexStore() );
	ct.create(state.store, idxStore, InMemoryCTCSearchTraits(cfg, state.store));
	sserialize::UByteArrayAdapter::OffsetType bO = dest.tellPutPtr();
	uint32_t sq = (cfg.hasEnabled(TextSearchConfig::QueryType::SUBSTRING) ? sserialize::StringCompleter::SQ_EPSP : sserialize::StringCompleter::SQ_EP);
	sq |= (cfg.hasCaseSensitive() ? sserialize::StringCompleter::SQ_CASE_SENSITIVE: sserialize::StringCompleter::SQ_CASE_INSENSITIVE);
	ct.append((sserialize::StringCompleter::SupportedQuerries) sq, dest, state.indexFactory, cfg.threadCount);
	if (cfg.check) {
		std::cout << "Checking CellTextCompleter for equality" << std::endl;
		sserialize::UByteArrayAdapter tmp = dest;
		tmp.setPutPtr(bO);
		tmp.shrinkToPutPtr();
		tmp.resetPtrs();
		sserialize::Static::CellTextCompleter sct( tmp, idxStore, state.store.geoHierarchy());
		if (ct.equal(sct, [&state](uint32_t id){ return state.indexFactory.indexById(id);})) {
			std::cout << "CellTextCompleter is equal" << std::endl;
		}
		else {
			std::cerr << "CellTextCompleter is NOT equal!" << std::endl;
		}
	}
}

void
handleCellTextSearch(GeoCellConfig & cfg, State & state, sserialize::UByteArrayAdapter & dest) {
	if (cfg.trieType == GeoCellConfig::TrieType::FLAT_TRIE) {
		handleCellTextSearchBase< oscar_create::CellTextCompleterFlatTrie>(cfg, state, dest);
	}
	else if (cfg.trieType == GeoCellConfig::TrieType::TRIE) {
		handleCellTextSearchBase< oscar_create::CellTextCompleterUnicodeTrie>(cfg, state, dest);
	}
	else {
		std::cout << "CellTextSearch does not support the requested trie type" << std::endl;
	}
}

void handleOOMCellTextSearch(OOMGeoCellConfig & cfg, State & state, sserialize::UByteArrayAdapter & dest) {
	std::shared_ptr<BaseSearchTraitsState> searchState(new BaseSearchTraitsState(state.store.kvStore(), cfg));
	OOM_SA_CTC_Traits<TextSearchConfig::ItemType::ITEM> itemTraits(cfg, state.store, state.indexFactory.asItemIndexStore());
	OOM_SA_CTC_Traits<TextSearchConfig::ItemType::REGION> regionTraits(cfg, state.store, state.indexFactory.asItemIndexStore());
	
	int sq = sserialize::StringCompleter::SQ_NONE;
	if (cfg.hasEnabled(TextSearchConfig::QueryType::PREFIX)) {
		sq |= sserialize::StringCompleter::SQ_EP;
	}
	if (cfg.hasEnabled(TextSearchConfig::QueryType::SUBSTRING)) {
		sq |= sserialize::StringCompleter::SQ_SSP;
	}
	if(cfg.hasCaseSensitive()) {
		sq |= sserialize::StringCompleter::SQ_CASE_SENSITIVE;
	}
	else {
		sq |= sserialize::StringCompleter::SQ_CASE_INSENSITIVE;
	}
	
	
	sserialize::appendSACTC(
		state.store.begin(), state.store.end(),
		state.store.begin(), state.store.begin()+state.store.geoHierarchy().regionSize(),
		itemTraits, regionTraits,
		cfg.maxMemoryUsage, cfg.threadCount, cfg.sortConcurrency,
		(sserialize::StringCompleter::SupportedQuerries)sq,
		state.indexFactory,
		dest
	);
}

void handleTextSearch(Config & opts, State & state) {
	if (!opts.textSearchConfig.size()) {
		return;
	}
	else {
		bool enabled = false;
		for(TextSearchConfig * x  : opts.textSearchConfig) {
			if (x && x->enabled) {
				enabled = true;
				break;
			}
		}
		if (!enabled) {
			return;
		}
	}

	std::string fn = liboscar::fileNameFromFileConfig(opts.getOutFileDir(), liboscar::FC_TEXT_SEARCH, false);
	sserialize::UByteArrayAdapter dest( sserialize::UByteArrayAdapter::createFile(1, fn) );

	if (dest.size() != 1) {
		std::cout << "Failed to create file for text search" << std::endl;
		return;
	}
	
#ifdef WITH_OSCAR_CREATE_NO_DATA_REFCOUNTING
	dest.disableRefCounting();
#endif

	liboscar::TextSearchCreator tsCreator(dest);
	
	for(TextSearchConfig* x : opts.textSearchConfig) {
		if (!x || !(x->enabled)) { //skip empty/disabled
			continue;
		}
		tsCreator.beginRawPut(x->type);
		if (x->type == liboscar::TextSearch::GEOCELL) {
			handleCellTextSearch(dynamic_cast<GeoCellConfig&>(*x), state, tsCreator.rawPut());
		}
		else if (x->type == liboscar::TextSearch::OOMGEOCELL) {
			handleOOMCellTextSearch(dynamic_cast<OOMGeoCellConfig&>(*x), state, tsCreator.rawPut());
		}
		else if (x->type == liboscar::TextSearch::GEOHIERARCHY_AND_ITEMS) {
			ItemSearchConfig & scfg = dynamic_cast<ItemSearchConfig&>(*x);
		
			tsCreator.rawPut().putUint8(1); //Version
			std::cout << "Creating the GeoHierarchy completer for the GeoHierarchyWithItems TextSearch" << std::endl;
			
			scfg.type = liboscar::TextSearch::GEOHIERARCHY;
			handleSimpleTextSearch(scfg, state, tsCreator.rawPut());
			std::cout << "Creating the Items completer for the GeoHierarchyWithItems TextSearch" << std::endl;
			
			scfg.type = liboscar::TextSearch::ITEMS;
			handleSimpleTextSearch(scfg, state, tsCreator.rawPut());
			tsCreator.endRawPut();
		}
		else if (x->type == liboscar::TextSearch::GEOHIERARCHY) {
			handleSimpleTextSearch(dynamic_cast<GeoHierarchySearchConfig&>(*x), state, tsCreator.rawPut());
		}
		else if (x->type == liboscar::TextSearch::ITEMS) {
			handleSimpleTextSearch(dynamic_cast<ItemSearchConfig&>(*x), state, tsCreator.rawPut());
		}
		
		tsCreator.endRawPut();
	}
	tsCreator.flush();
	
#ifdef WITH_OSCAR_CREATE_NO_DATA_REFCOUNTING
	dest.enableRefCounting();
#endif
}

void handleSearchCreation(Config & opts, State & state) {
	if (opts.statsConfig.memUsage) {
		sserialize::MemUsage().print();
	}
	
	createTagStore(opts, state);

	if (opts.statsConfig.memUsage) {
		sserialize::MemUsage().print();
	}

	handleGeoSearch(opts, state);

	if (opts.statsConfig.memUsage) {
		sserialize::MemUsage().print();
	}
	
	handleTextSearch(opts, state);
	
	if (opts.statsConfig.memUsage) {
		sserialize::MemUsage().print();
	}
}

void handleKVCreation(oscar_create::Config & opts, State & state) {
	if (!opts.kvStoreConfig || !opts.kvStoreConfig->enabled) {
		throw sserialize::ConfigurationException("KvStoreConfig", "not enabled");
		return;
	}
	
	sserialize::TimeMeasurer dbTime; dbTime.begin();
	oscar_create::OsmKeyValueObjectStore store;

	oscar_create::OsmKeyValueObjectStore::SaveDirector * itemSaveDirectorPtr = 0;
	if (opts.kvStoreConfig->saveEverything) {
		itemSaveDirectorPtr = new oscar_create::OsmKeyValueObjectStore::SaveAllSaveDirector();
	}
	else if (opts.kvStoreConfig->saveEveryTag) {
		itemSaveDirectorPtr = new oscar_create::OsmKeyValueObjectStore::SaveEveryTagSaveDirector(opts.kvStoreConfig->itemsSavedByKeyFn, opts.kvStoreConfig->itemsSavedByKeyValueFn);
	}
	else if (!opts.kvStoreConfig->itemsIgnoredByKeyFn.empty()) {
		itemSaveDirectorPtr = new oscar_create::OsmKeyValueObjectStore::SaveAllItemsIgnoring(opts.kvStoreConfig->itemsIgnoredByKeyFn);
	}
	else {
		itemSaveDirectorPtr = new oscar_create::OsmKeyValueObjectStore::SingleTagSaveDirector(opts.kvStoreConfig->keyToStoreFn, opts.kvStoreConfig->keyValuesToStoreFn, opts.kvStoreConfig->itemsSavedByKeyFn, opts.kvStoreConfig->itemsSavedByKeyValueFn);
	}
	std::shared_ptr<oscar_create::OsmKeyValueObjectStore::SaveDirector> itemSaveDirector(itemSaveDirectorPtr);

	if (opts.inFileNames.size()) {
		if (opts.kvStoreConfig->autoMaxMinNodeId) {
			for(const std::string & inFileName : opts.inFileNames) {
				uint64_t minNodeId, maxNodeId;
				opts.kvStoreConfig->minNodeId = std::numeric_limits<uint64_t>::max();
				opts.kvStoreConfig->maxNodeId = 0;
				if (!oscar_create::findNodeIdBounds(inFileName, minNodeId, maxNodeId)) {
					throw sserialize::CreationException("Finding min/max node id failed for " + inFileName + "! skipping input file...");
				}
				opts.kvStoreConfig->minNodeId = std::min(opts.kvStoreConfig->minNodeId, minNodeId);
				opts.kvStoreConfig->maxNodeId = std::min(opts.kvStoreConfig->maxNodeId, maxNodeId);
			}
			std::cout << "min=" << opts.kvStoreConfig->minNodeId << ", max=" << opts.kvStoreConfig->maxNodeId << std::endl;
		}
		
		if (opts.statsConfig.memUsage) {
			sserialize::MemUsage().print("Polygonstore", "Poylgonstore");
		}
		
		oscar_create::OsmKeyValueObjectStore::CreationConfig cc;
		cc.fileNames = opts.inFileNames;
		cc.itemSaveDirector = itemSaveDirector;
		sserialize::narrow_check_assign(cc.maxNodeCoordTableSize) = opts.kvStoreConfig->maxNodeHashTableSize;
		cc.maxNodeId = opts.kvStoreConfig->maxNodeId;
		cc.minNodeId = opts.kvStoreConfig->minNodeId;
		cc.numThreads = opts.kvStoreConfig->numThreads;
		cc.blobFetchCount = opts.kvStoreConfig->blobFetchCount;
		cc.addRegionsToCells = opts.kvStoreConfig->addRegionsToCells;
		cc.rc.regionFilter = oscar_create::AreaExtractor::nameFilter(opts.kvStoreConfig->keysDefiningRegions, opts.kvStoreConfig->keyValuesDefiningRegions);
		cc.rc.grtLatCount = opts.kvStoreConfig->grtLatCount;
		cc.rc.grtLonCount = opts.kvStoreConfig->grtLonCount;
		cc.rc.grtMinDiag = opts.kvStoreConfig->grtMinDiag;
		cc.rc.polyStoreLatCount = opts.kvStoreConfig->latCount;
		cc.rc.polyStoreLonCount = opts.kvStoreConfig->lonCount;
		cc.rc.polyStoreMaxTriangPerCell = opts.kvStoreConfig->maxTriangPerCell;
		cc.rc.triangMaxCentroidDist = opts.kvStoreConfig->maxTriangCentroidDist;
		cc.sortOrder = (oscar_create::OsmKeyValueObjectStore::ItemSortOrder) opts.kvStoreConfig->itemSortOrder;
		cc.prioStringsFn = opts.kvStoreConfig->prioStringsFileName;
		cc.scoreConfigFileName = opts.kvStoreConfig->scoreConfigFileName;
		cc.geometryCleanType = opts.kvStoreConfig->gct;
		
		if (!opts.kvStoreConfig->keysValuesToInflate.empty()) {
			oscar_create::readKeysFromFile(opts.kvStoreConfig->keysValuesToInflate, std::inserter(cc.inflateValues, cc.inflateValues.end()));
		}

		bool parseOK = store.populate(cc);
		if (!parseOK) {
			throw std::runtime_error("Failed to parse files");
		}
	}

	dbTime.end();
	store.stats(std::cout);
	std::cout << "Time to create KeyValueStore: " << dbTime << std::endl;

	std::cout << "Serializing KeyValueStore" << std::endl;
	sserialize::UByteArrayAdapter dataBaseListAdapter(sserialize::UByteArrayAdapter::createFile(1024*1024, opts.getOutFileName(liboscar::FC_KV_STORE)) );
	sserialize::UByteArrayAdapter::OffsetType outSize = store.serialize(dataBaseListAdapter, state.indexFactory, opts.kvStoreConfig->fullRegionIndex);
	if (outSize < dataBaseListAdapter.size()) {
		dataBaseListAdapter.shrinkStorage(dataBaseListAdapter.size() - outSize);
	}
	std::cout << "KeyValueStore serialized  with a length of " << sserialize::prettyFormatSize(outSize) << std::endl;
}

} //end namespace