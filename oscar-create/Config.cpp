#include "Config.h"
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/printers.h>
#include <sserialize/strings/stringfunctions.h>
#include <liboscar/constants.h>
#include "OsmKeyValueObjectStore.h"
#include <json/json.h>

namespace oscar_create {

//Print functions----------------

inline std::string repr(bool v) {
	return (v ? "yes" : "no");
}

inline std::string repr(sserialize::ItemIndex::Types v) {
	switch (v) {
	case (sserialize::ItemIndex::T_WAH):
		return "rle word aligned bit vector";
	case (sserialize::ItemIndex::T_DE):
		return "delta encoding";
	case (sserialize::ItemIndex::T_REGLINE):
		return "regression line";
	case (sserialize::ItemIndex::T_RLE_DE):
		return "delta+run-length encoding";
	case (sserialize::ItemIndex::T_SIMPLE):
		return "simple";
	case (sserialize::ItemIndex::T_NATIVE):
		return "native";
	default:
		return "invalid";
	}
}

std::ostream& StatsConfig::operator<<(std::ostream& out) const {
	out << "print memusage: " << repr(memUsage);
	return out;
}

std::ostream& IndexStoreConfig::operator<<(std::ostream& out) const {
	out << "input store: " << inputStore << "\n";
	out << "type: " << repr(type) << "\n";
	out << "check: " << repr(check) << "\n";
	out << "deduplicate: " << repr(deduplicate);
	return out;
}

std::ostream& GridConfig::operator<<(std::ostream& out) const {
	out << "latCount: " << latCount << "\nlonCount: " << lonCount;
	return out;
}

std::ostream& RTreeConfig::operator<<(std::ostream& out) const {
	out << "latCount: " << latCount << "\nlonCount: " << lonCount;
	return out;
}

std::ostream& TagStoreConfig::operator<<(std::ostream& out) const {
	out << "key file: " << tagKeys << "\n";
	out << "key-values file: " << tagKeyValues;
	return out;
}

std::ostream& KVStoreConfig::operator<<(std::ostream& out) const {
	out << "Number of threads: " << numThreads << "\n";
	out << "Max node table entries: " << sserialize::toString(maxNodeHashTableSize) << "\n";
	out << "Keys whose values are infalted: " << keysValuesToInflate << "\n";
	out << "Node HashTable size: ";
	if (autoMaxMinNodeId) {
		out << "auto";
	}
	else {
		out << minNodeId << "->" << maxNodeId;
	}
	out << "\n";
	out << "ItemSaveDirector rule: ";
	if (saveEverything) {
		out << "everything" << "\n";
	}
	else if (!itemsIgnoredByKeyFn.empty()) {
		out << "Ignore items with tags in" << itemsIgnoredByKeyFn << "\n";
	}
	else {
		if (saveEveryTag) {
			out << "every tag";
		}
		else {
			out << "according to keys and key, values";
		}
		out << "\n";
		out << "Key to store: " << keyToStoreFn << "\n";
		out << "Key=Value to store: " << keyValuesToStoreFn << "\n";
		out << "Items with key= to store: " << itemsSavedByKeyFn << "\n";
		out << "Items with key=value to store: " << itemsSavedByKeyValueFn << "\n";
	}
	out << "Score config: " << scoreConfigFileName << "\n";
	out << "Item sort order: ";
	switch(itemSortOrder) {
	case oscar_create::OsmKeyValueObjectStore::ISO_NONE:
		out << "none";
		break;
	case oscar_create::OsmKeyValueObjectStore::ISO_SCORE:
		out << "score";
		break;
	case oscar_create::OsmKeyValueObjectStore::ISO_SCORE_NAME:
		out << "score, name";
		break;
	case oscar_create::OsmKeyValueObjectStore::ISO_SCORE_PRIO_STRINGS:
		out << prioStringsFileName;
		break;
	default:
		out << "invalid";
		break;
	}
	out << "\n";
	out << "Read boundaries: ";
	if (readBoundaries) {
		out << "initial=" << polyStoreLatCount << "x" << polyStoreLonCount;
		out << ", max triangle per cell="<< polyStoreMaxTriangPerCell << ", max triangle centroid dist=" << triangMaxCentroidDist << "\n";
	}
	else {
		out << "no" << std::endl;
	}
	out << "Keys defining regions=" << keysDefiningRegions << "\n";
	out << "Key:Values defining regions=" << keyValuesDefiningRegions << "\n";
	out << " FullRegionIndex: " << (fullRegionIndex ? "yes" : "no" );
	return out;
}

std::ostream& TextSearchConfig::SearchCapabilities::operator<<(std::ostream& out) const {
	out << "caseSensitive: " << (caseSensitive ? "yes" : "no") << "\n";
	out << "diacritcInSensitive: " << (diacritcInSensitive ? "yes" : "no") << "\n";
	out << "tagFile: " << tagFile << "\n";
	out << "valueFile: " << valueFile;
	return out;
}

std::ostream& TextSearchConfig::operator<<(std::ostream& out) const {
	std::array<std::string, 2> itemTypeNames{"items", "regions"};
	std::array<std::string, 2> queryTypeNames{"prefix", "suffix"};
	
	for(uint32_t itemType(0); itemType < 2; ++itemType) {
		for(uint32_t queryType(0); queryType < 2; ++queryType) {
			if (searchCapabilites[itemType][queryType].enabled) {
				out << itemTypeNames[itemType] << "::" << queryTypeNames[queryType] << ":\n";
				out << searchCapabilites[itemType][queryType] << "\n";
			}
		}
	}
	this->print(out);
	return out;
}

void ItemSearchConfig::print(std::ostream& out) const {
	if (suffixDelimeters.size()) {
		out << "\tSuffix delimeters: " << sserialize::stringFromUnicodePoints(suffixDelimeters.cbegin(), suffixDelimeters.cend()) << "\n";
	}
	if (levelsWithoutIndex.size()) {
		out << "\tLevels without a full index: ";
		for(std::set<uint8_t>::iterator it = levelsWithoutIndex.cbegin(); it != levelsWithoutIndex.cend(); it++) {
			out << static_cast<int>(*it) << ", ";
		}
		out << "\n";
	}
	out << "\tMaximum merge count for no full prefix index:" << maxPrefixIndexMergeCount << std::endl;
	out << "\tMaximum merge count for no full suffix index:" << maxSuffixIndexMergeCount << std::endl;
	out << "\tNodeType: " << oscar_create::Config::toString(nodeType) << std::endl;
	out << "\tAggressive memory usage: " <<  sserialize::toString(aggressiveMem) << std::endl;
	out << "\tMemory storage type: ";
	switch (mmType) {
	case sserialize::MM_FILEBASED:
		out << "file-based";
		break;
	case sserialize::MM_PROGRAM_MEMORY:
		out << "program memory";
		break;
	case sserialize::MM_SHARED_MEMORY:
		out << "shared memory";
		break;
	default:
		out << "invalid";
		break;
	}
	out << "\n";
	out << "\tMerge Index: " << sserialize::toString(mergeIndex) <<  std::endl;
	out << "\tTrie-Type: ";
	if (trieType == TrieType::FULL_INDEX_TRIE) {
		out << "full-index-trie";
	}
	else if (trieType == TrieType::TRIE) {
		out << "trie";
	}
	else if (trieType == TrieType::FLAT_GST) {
		out << "fgst";
	}
	else if (trieType == TrieType::FLAT_TRIE) {
		out << "flattrie";
	}
	out << "\n";
	out << "\tExtensive Checking: " << sserialize::toString(extensiveChecking) <<  "\n";
	out << "\tThread count: " << threadCount;
}

void GeoHierarchySearchConfig::print(std::ostream& out) const {}

void GeoCellConfig::print(std::ostream& out) const {
	if (suffixDelimeters.size()) {
		out << "\tSuffix delimeters: " << sserialize::stringFromUnicodePoints(suffixDelimeters.cbegin(), suffixDelimeters.cend()) << "\n";
	}
}

void OOMGeoCellConfig::print(std::ostream& out) const {
	out << "Thread count: " << threadCount;
}

std::ostream& Config::operator<<(std::ostream& out) const {
	out << indexStoreConfig;
	out << statsConfig;
	if (kvStoreConfig) {
		out << "KVStoreConfig:\n"; 
		out << *kvStoreConfig;
	}
	if (gridConfig) {
		out << "GridConfig:\n";
		out << *gridConfig;
	}
	if (rTreeConfig) {
		out << "RTreeConfig:\n";
		out << *rTreeConfig;
	}
	if (tagStoreConfig) {
		out << "TagStoreConfig:\n";
		out << *tagStoreConfig;
	}
	for(auto x : textSearchConfig) {
		if (!x.second) {
			continue;
		}
		out << "TextSearchConfig[";
		switch (x.first) {
		case liboscar::TextSearch::ITEMS:
			out << "items";
			break;
		case liboscar::TextSearch::GEOCELL:
			out << "geocell";
			break;
		case liboscar::TextSearch::GEOHIERARCHY:
			out << "geohierarchy";
			break;
		default:
			out << "invalid";
			break;
		}
		out << "]:\n" << *x.second;
	}
}

std::string Config::help() {
	return std::string("-i <input.osm.pbf|input dir> -o <output dir> -c <config.json>");
}

Config::Config() :
kvStoreConfig(0),
gridConfig(0),
rTreeConfig(0),
tagStoreConfig(0)
{}

//parse functions
StatsConfig::StatsConfig(const Json::Value & cfg) : StatsConfig() {
	Json::Value v = cfg["print-memory-usage"];
	if (v.isBool()) {
		memUsage = v.asBool();
	}
}

IndexStoreConfig::IndexStoreConfig(const Json::Value & cfg) :
type(sserialize::ItemIndex::sserialize::ItemIndex::T_NULL),
check(false),
deduplicate(true)
{
	Json::Value v = cfg["type"];
	if (v.isString()) {
		std::string t = v.asString();
		if (t == "rline") {
			type = sserialize::ItemIndex::T_REGLINE;
		}
		else if (t == "simple") {
			type = sserialize::ItemIndex::T_SIMPLE;
		}
		else if (t == "wah") {
			type = sserialize::ItemIndex::T_WAH;
		}
		else if (t == "de") {
			type = sserialize::ItemIndex::T_DE;
		}
		else if (t == "rlede") {
			type = sserialize::ItemIndex::T_RLE_DE;
		}
		else if (t == "native") {
			type = sserialize::ItemIndex::T_NATIVE;
		}
		else {
			throw sserialize::ConfigurationException("IndexStoreConfig: invalid index type");
		}
	}
	
	v = cfg["check"];
	if (v.isBool()) {
		check = v.asBool();
	}
	
	v = cfg["deduplicate"];
	if (v.isBool()) {
		deduplicate = v.asBool();
	}
}

GridConfig::GridConfig(const Json::Value& cfg) :
latCount(0),
lonCount(0)
{
	Json::Value v = cfg["latcount"];
	if (v.isNumeric()) {
		latCount = v.asUInt();
	}
	
	v = cfg["loncount"];
	if (v.isNumeric()) {
		lonCount = v.asUInt();
	}
}

RTreeConfig::RTreeConfig(const Json::Value& cfg) :
latCount(0),
lonCount(0)
{
	Json::Value v = cfg["latcount"];
	if (v.isNumeric()) {
		latCount = v.asUInt();
	}
	
	v = cfg["loncount"];
	if (v.isNumeric()) {
		lonCount = v.asUInt();
	}
}

TagStoreConfig::TagStoreConfig(const Json::Value& cfg) :
enabled(false)
{
	Json::Value v = cfg["enabled"];
	if (v.isBool()) {
		enabled = v.asBool();
	}
	
	v = cfg["keys"];
	if (v.isString()) {
		tagKeys = v.asString();
	}
	
	v = cfg["keyvalues"];
	if (v.isString()) {
		tagKeyValues = v.asString();
	}
}

KVStoreConfig::KVStoreConfig(const Json::Value& cfg) :
maxNodeHashTableSize(std::numeric_limits<uint32_t>::max()),
saveEverything(false),
saveEveryTag(false),
minNodeId(0),
maxNodeId(0),
autoMaxMinNodeId(false),
readBoundaries(false),
fullRegionIndex(false),
polyStoreLatCount(0),
polyStoreLonCount(0),
polyStoreMaxTriangPerCell(0),
numThreads(0),
itemSortOrder(OsmKeyValueObjectStore::ISO_NONE)
{
	Json::Value v = cfg["threadCount"];
	if (v.isNumeric()) {
		numThreads = v.asUInt();
	}
	
	v = cfg["fullRegionIndex"];
	if (v.isBool()) {
		fullRegionIndex = v.asBool();
	}
	
	v = cfg["addParentInfo"];
	if (v.isBool()) {
		addParentInfo = v.asBool();
	}
	
	v = cfg["hashConfig"];
	if (v.isObject()) {
		Json::Value tmp = v["begin"];
		if (tmp.isNumeric()) {
			minNodeId = tmp.asUInt64();
		}
		tmp = v["end"];
		if (tmp.isNumeric()) {
			maxNodeId = tmp.asUInt64();
		}
	}
	else if (v.isString() && v.asString() == "auto") {
		autoMaxMinNodeId = true;
	}
	
	v = cfg["nodeTableSize"];
	if (v.isNumeric()) {
		maxNodeHashTableSize = v.asUInt64();
	}
	
	v = cfg["tagFilter"];
	if (v.isObject()) {
		Json::Value tmp = v["keys"];
		if (tmp.isString()) {
			keyToStoreFn = tmp.asString();
		}
		Json::Value tmp = v["keyValues"];
		if (tmp.isString()) {
			keyValuesToStoreFn = tmp.asString();
		}
	}
	else if (v.isString() && v.asString() == "all") {
		saveEveryTag = true;
	}
	
	v = cfg["itemFilter"];
	if (v.isObject()) {
		Json::Value tmp = v["keys"];
		if (tmp.isString()) {
			itemsSavedByKeyFn = tmp.asString();
		}
		Json::Value tmp = v["keyValues"];
		if (tmp.isString()) {
			itemsSavedByKeyValueFn = tmp.asString();
		}
	}
	else if (v.isString() && v.asString() == "all") {
		saveEverything = true;
	}
	
	v = cfg["regionFilter"];
	if (v.isObject()) {
		Json::Value tmp = v["keys"];
		if (tmp.isString()) {
			keysDefiningRegions = tmp.asString();
		}
		Json::Value tmp = v["keyValues"];
		if (tmp.isString()) {
			keyValuesDefiningRegions = tmp.asString();
		}
	}
	
	v = cfg["scoring"];
	if (v.isObject()) {
		Json::Value tmp = v["config"];
		if (tmp.isString()) {
			scoreConfigFileName = tmp.asString();
		}
	}
	
	v = cfg["sorting"];
	if (v.isObject()) {
		Json::Value tmp = v["order"];
		if (tmp.isString()) {
			std::string order = tmp.asString();
			if (order == "score") {
				itemSortOrder = OsmKeyValueObjectStore::ISO_SCORE;
			}
			else if (order == "name") {
				itemSortOrder = OsmKeyValueObjectStore::ISO_SCORE_NAME;
			}
			if (order == "priority" && tmp["priority"].isString() &&
				sserialize::MmappedFile::fileExists(tmp["priority"].asString()))
			{
				prioStringsFileName = tmp["priority"].asString();
				itemSortOrder = OsmKeyValueObjectStore::ISO_SCORE_PRIO_STRINGS;
			}
		}
	}
}

TextSearchConfig::TextSearchConfig(const Json::Value& v) {
	
}

TextSearchConfig* TextSearchConfig::parseTyped(const Json::Value& cfg) {
	Json::Value tv = cfg["type"];
	Json::Value cv = cfg["config"];
	if (tv.isString() && cv.isObject()) {
		std::string t = tv.asString();
		if (t == "items") {
			return new ItemSearchConfig(cv);
		}
		else if (t == "geoh") {
			return new GeoHierarchySearchConfig(cv);
		}
		else if (t == "geocell") {
			return new GeoCellConfig(cv);
		}
		else if (t == "oomgeocell") {
			return new OOMGeoCellConfig(cv);
		}
	}
	std::cerr << "Invalid text search type" << std::endl;
	return new TextSearchConfig();
}


Config::ReturnValues Config::fromCmdLineArgs(int argc, char** argv) {
	std::string inputString, outputString, configString;

	for(int i=1; i < argc; i++) {
		std::string token(argv[i]);
		if (token == "-i" && i+1 < argc) {
			inputString = std::string(argv[i+1]);
			++i;
		}
		else if (token == "-o" && i+1 < argc) {
			outputString = std::string(argv[i+1]);
			++i;
		}
		else if (token == "-c" && i+1 < argc) {
			configString = std::string(argv[i+1]);
			++i;
		}
	}
	
	std::ifstream inFileStream;
	inFileStream.open(configString);
	
	if (!inFileStream.is_open()) {
		std::cerr << "could not open config file " << configString << std::endl;
		return RV_FAILED;
	}
	
	Json::Value root;
	inFileStream >> root;
	
	Json::Value tmp;
	
	tmp = root["stats"];
	if (!tmp.isNull()) {
		statsConfig = StatsConfig(tmp);
	}
	
	tmp = root["tempdir"];
	if (!root["tempdir"].isNull()) {
		Json::Value tmp2 = tmp["fast"];
		if (!tmp2.isNull() && tmp2.isString()) {
			sserialize::UByteArrayAdapter::setFastTempFilePrefix(tmp2.asString());
		}
		
		tmp2 = tmp["slow"];
		if (!tmp2.isNull() && tmp2.isString()) {
			sserialize::UByteArrayAdapter::setTempFilePrefix(tmp2.asString());
		}
	}
	
	tmp = root["index"];
	if (!tmp.isNull()) {
		indexStoreConfig = IndexStoreConfig(tmp);
	}
	
	tmp = root["store"];
	if (!tmp.isNull()) {
		kvStoreConfig = new KVStoreConfig(tmp);
	}
	
	tmp = root["grid"];
	if (!tmp.isNull()) {
		gridConfig = new GridConfig(tmp);
	}
	
	tmp = root["rtree"];
	if (!tmp.isNull()) {
		rTreeConfig = new RTreeConfig(tmp);
	}
	
	tmp = root["tagstore"];
	if (!tmp.isNull()) {
		tagStoreConfig = new TagStoreConfig(tmp);
	}
	
	tmp = root["textsearch"];
	if (!tmp.isNull()) {
		if (tmp.isArray()) {
			for(const Json::Value & x : tmp) {
				textSearchConfig.push_back(TextSearchConfig::parseTextSearchConfig(x));
			}
		}
		else if (tmp.isObject()) {
			textSearchConfig.push_back(TextSearchConfig::parseTextSearchConfig(tmp));
		}
		else {
			std::cerr << "Invalid entry for textsearch" << std::endl;
		}
	}
	
	return RV_OK;
}


/*
TextSearchConfig::TextSearchConfig(const std::string& str) :
aggressiveMem(false), mmType(sserialize::MM_FILEBASED), mergeIndex(false), extensiveChecking(false),
nodeType(sserialize::Static::TrieNode::T_LARGE_COMPACT),
maxPrefixIndexMergeCount(-1), maxSuffixIndexMergeCount(-1),
type(Type::TRIE), threadCount(1)
{
	std::vector<std::string> ks = sserialize::split< std::vector<std::string> >(str, ',', '\\');
	std::vector< std::pair<std::string, std::string> > kvs;
	for(std::vector<std::string>::const_iterator it(ks.cbegin()), end(ks.cend()); it != end; ++it) {
		if (*it == "c") {
			caseSensitive = true;
		}
		else if (*it == "d") {
			diacritcInSensitive = true;
		}
		else if (*it == "s") {
			suffixes = true;
		}
		else if (*it == "a") {
			aggressiveMem = true;
		}
		else if (*it == "mi") {
			mergeIndex = true;
		}
		else if (*it == "ec") {
			extensiveChecking=true;
		}
		else {
			std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(*it, '=', '\\');
			if (tmp.size() != 2) {
				std::cout << "Unrecoginzed option: " << tmp << std::endl;
			}
			else {
				kvs.push_back( std::pair<std::string, std::string>(tmp.front(), tmp.back()) );
			}
		}
	}
	for(std::vector< std::pair<std::string, std::string> >::const_iterator it(kvs.cbegin()), end(kvs.cend()); it != end; ++it) {
		bool checkFileExistence = false;
		if (it->first == "sd") {
			for(std::string::const_iterator jt(it->second.cbegin()), jend(it->second.cend()); jt != jend; ) {
				suffixDelimeters.insert( utf8::next<std::string::const_iterator>(jt, jend) );
			}
		}
		else if (it->first == "lwi") {
			int tmp = atoi(it->second.c_str());
			if (tmp >= 0)
				levelsWithoutIndex.insert(tmp);
		}
		else if (it->first == "lwir") {
			std::vector<std::string> range = sserialize::split< std::vector<std::string> >(it->second, '-', '\\');
			if (range.size() != 2) {
				std::cout << "Given levels-without-index range is invalid" << it->second << std::endl;
			}
			else {
				int tmp0 = atoi(range.front().c_str());
				int tmp1 = atoi(range.back().c_str());
				if (tmp0 >= 0 && tmp1 >= 0 && tmp0 < tmp1) {
					for(; tmp0 <= tmp1; ++tmp0) {
						levelsWithoutIndex.insert(tmp0);
					}
				}
			}
		}
		else if (it->first == "n") {
			if (it->second == "simple") {
				nodeType = sserialize::Static::TrieNode::T_SIMPLE;
			}
			else if (it->second == "compact") {
				nodeType = sserialize::Static::TrieNode::T_COMPACT;
			}
			else if (it->second == "large") {
				nodeType = sserialize::Static::TrieNode::T_LARGE_COMPACT;
			}
			else {
				std::cout << "Unrecognized node type: " << it->second << std::endl;
			}
		}
		else if (it->first == "mpm") {
			maxPrefixIndexMergeCount = atoi(it->second.c_str());
		}
		else if (it->first == "msm") {
			maxSuffixIndexMergeCount = atoi(it->second.c_str());
		}
		else if (it->first == "t") {
			if (it->second == "trie") {
				type = Type::TRIE;
			}
			else if (it->second == "fgst") {
				type = Type::FLAT_GST;
			}
			else if (it->second == "flattrie") {
				type = Type::FLAT_TRIE;
			}
			else if (it->second == "fitrie") {
				type = Type::FULL_INDEX_TRIE;
			}
			else {
				std::cout << "Unrecognized trie type: " << it->second << std::endl;
			}
		}
		else if (it->first == "tp") {
			storeTagsPrefixFile = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "ts") {
			storeTagsSuffixFile = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "kf") {
			keyFile = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "rf") {
			storeRegionTagsFile = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "mmt") {
			if (it->second == "prg") {
				mmType = sserialize::MM_PROGRAM_MEMORY;
			}
			else if (it->second == "shm") {
				mmType = sserialize::MM_SHARED_MEMORY;
			}
			else if (it->second == "file") {
				mmType = sserialize::MM_FILEBASED;
			}
			else {
				std::cout << "Unrecognized memory storage type: " << it->second << std::endl;
			}
		}
		else if (it->first == "tc") {
			int tmp = atoi(it->second.c_str());
			if (tmp <= 0) {
				tmp = std::thread::hardware_concurrency();
			}
			threadCount = tmp;
		}
		else {
			std::cout << "Unrecognized key: " << it->first << std::endl;
		}
		if (checkFileExistence) {
			if (!sserialize::MmappedFile::fileExists(it->second)) {
				sserialize::ConfigurationException("TextSearchConfig", "Key=" + it->first + ": File " + it->second + " does not exist");
			}
		}
	}
}

TagStoreConfig::TagStoreConfig(const std::string& str) : create(true) {
	std::vector<std::string> ks = sserialize::split< std::vector<std::string> >(str, ',', '\\');
	std::vector< std::pair<std::string, std::string> > kvs;
	for(std::vector<std::string>::const_iterator it(ks.cbegin()), end(ks.cend()); it != end; ++it) {
		std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(*it, '=', '\\');
		if (tmp.size() != 2) {
			std::cout << "Unrecoginzed option: " << tmp << std::endl;
		}
		else {
			kvs.push_back( std::pair<std::string, std::string>(tmp.front(), tmp.back()) );
		}
	}
	for(std::vector< std::pair<std::string, std::string> >::const_iterator it(kvs.cbegin()), end(kvs.cend()); it != end; ++it) {
		bool checkFileExistence = false;
		if (it->first == "k") {
			tagKeys = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "kv") {
			tagKeyValues = it->second;
			checkFileExistence = true;
		}
		else {
			std::cout << "Unrecoginzed option: " << it->first << "=" << it->second << std::endl;
		}
		if (checkFileExistence) {
			if (!sserialize::MmappedFile::fileExists(it->second)) {
				sserialize::ConfigurationException("TagStoreConfig", "Key=" + it->first + ": File " + it->second + " does not exist");
			}
		}
	}
}

KVStoreConfig::KVStoreConfig() :
maxNodeHashTableSize(std::numeric_limits<uint32_t>::max()),
saveEverything(false),
saveEveryTag(false),
minNodeId(0),
maxNodeId(0),
autoMaxMinNodeId(false),
readBoundaries(false),
polyStoreLatCount(0),
polyStoreLonCount(0),
polyStoreMaxTriangPerCell(0),
numThreads(0)
{}


KVStoreConfig::KVStoreConfig(const std::string& str) :
maxNodeHashTableSize(std::numeric_limits<uint32_t>::max()),
saveEverything(false),
saveEveryTag(false),
minNodeId(0),
maxNodeId(0),
autoMaxMinNodeId(false),
readBoundaries(false),
fullRegionIndex(false),
polyStoreLatCount(0),
polyStoreLonCount(0),
polyStoreMaxTriangPerCell(0),
numThreads(0),
itemSortOrder(OsmKeyValueObjectStore::ISO_NONE)
{
	std::vector<std::string> ks = sserialize::split< std::vector<std::string> >(str, ',', '\\');
	std::vector< std::pair<std::string, std::string> > kvs;
	for(std::vector<std::string>::const_iterator it(ks.cbegin()), end(ks.cend()); it != end; ++it) {
		if (*it == "st") {
			saveEveryTag = true;
		}
		else if (*it == "se") {
			saveEverything = true;
		}
		else if (*it == "fri") {
			fullRegionIndex = true;
		}
		else {
			std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(*it, '=', '\\');
			if (tmp.size() != 2) {
				std::cout << "Unrecoginzed option: " << tmp << std::endl;
				continue;
			}
			else {
				kvs.push_back( std::pair<std::string, std::string>(tmp.front(), tmp.back()) );
			}
		}
	}
	for(std::vector< std::pair<std::string, std::string> >::const_iterator it(kvs.cbegin()), end(kvs.cend()); it != end; ++it) {
		bool checkFileExistence = false;
		if (it->first == "nts") {
			maxNodeHashTableSize = atol(it->second.c_str());
		}
		else if (it->first == "p") {
			std::vector<std::string> popts = sserialize::split< std::vector<std::string> >(it->second, 'x', '\\');
			if (popts.size() != 4) {
				throw sserialize::ConfigurationException("KVStoreConfig", "Region store configuration has invalid options string: " + it->second);
			}
			readBoundaries = true;
			polyStoreLatCount = atoi(popts[0].c_str());
			polyStoreLonCount = atoi(popts[1].c_str());
			polyStoreMaxTriangPerCell = atoi(popts[2].c_str());
			triangMaxCentroidDist = atof(popts[3].c_str());
		}
		else if (it->first == "hs") {
			if (it->second == "auto") {
				autoMaxMinNodeId = true;
			}
			else {
				std::vector<std::string> hsopts = sserialize::split< std::vector<std::string> >(it->second, ':', '\\');
				if (hsopts.size() != 2) {
					std::cout << "Unrecoginzed option: " << it->second << std::endl;
				}
				else {
					minNodeId = atoi(hsopts[0].c_str());
					maxNodeId = atoi(hsopts[1].c_str());
				}
			}
		}
		else if (it->first == "kvi") {
			keysValuesToInflate = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "sk") {
			keyToStoreFn = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "skv") {
			keyValuesToStoreFn = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "sik") {
			itemsSavedByKeyFn = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "sikv") {
			itemsSavedByKeyValueFn = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "kdr") {
			keysDefiningRegions = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "kvdr") {
			keyValuesDefiningRegions = it->second;
			checkFileExistence = true;
		}
		else if (it->first == "tc") {
			numThreads = atoi(it->second.c_str());
		}
		else if (it->first == "sc") {
			scoreConfigFileName = it->second;
		}
		else if(it->first == "iso") {
			if (it->second == "score") {
				itemSortOrder = OsmKeyValueObjectStore::ISO_SCORE;
			}
			else if (it->second == "name") {
				itemSortOrder = OsmKeyValueObjectStore::ISO_SCORE_NAME;
			}
			else if (sserialize::MmappedFile::fileExists(it->second)) {
				prioStringsFileName = it->second;
				itemSortOrder = OsmKeyValueObjectStore::ISO_SCORE_PRIO_STRINGS;
			}
		}
		else {
			std::cout << "Unrecognized key: " << it->first << std::endl;
		}
		if (checkFileExistence) {
			if (!sserialize::MmappedFile::fileExists(it->second)) {
				throw sserialize::ConfigurationException("KVStoreConfig", "Key=" + it->first + ": File " + it->second + " does not exist");
			}
		}
	}
}



Config::Config() :
m_appendConfigToOutFileName(false),
createKVStore(false),
indexType(sserialize::ItemIndex::T_RLE_DE),
indexDedup(true),
gridLatCount(0),
gridLonCount(0),
gridRTreeLatCount(0),
gridRTreeLonCount(0),
memUsage(false)
{}

Config::ReturnValues Config::fromCmdLineArgs(int argc, char** argv) {
	for(int i=1; i < argc; i++) {
		std::string str(argv[i]);
		if (str == "-o" && i+1 < argc) {
			m_outFileName = std::string(argv[i+1]);
			i++;
		}
		else if (str == "-oa") {
			m_appendConfigToOutFileName = true;
		}
		else if (str == "-tempdir" && i+1 < argc) {
			std::string tempDir(argv[i+1]);
			sserialize::UByteArrayAdapter::setTempFilePrefix(tempDir);
			i++;
		}
		else if (str == "-fasttempdir" && i+1 < argc) {
			std::string tempDir(argv[i+1]);
			sserialize::UByteArrayAdapter::setFastTempFilePrefix(tempDir);
			i++;
		}
		else if (str == "--create-grid") {
			if (i+2 < argc) {
				gridLatCount = atoi(argv[i+1]);
				gridLonCount = atoi(argv[i+2]);
				i += 2;
			}
			else {
				return RV_FAILED;
			}
		}
		else if (str == "--create-rtree-grid") {
			if (i+2 < argc) {
				gridRTreeLatCount = atoi(argv[i+1]);
				gridRTreeLonCount = atoi(argv[i+2]);
				i += 2;
			}
			else {
				return RV_FAILED;
			}
		}
		else if (str == "-i" && i+1 < argc) {
			inputIndexStore = std::string(argv[i+1]);
			++i;
		}
		else if (str == "-it" && i+1 < argc) {
			std::string idType(argv[i+1]);
			if (idType == "rline")
				indexType = sserialize::ItemIndex::T_REGLINE;
			else if (idType == "simple")
				indexType = sserialize::ItemIndex::T_SIMPLE;
			else if (idType == "wah")
				indexType = sserialize::ItemIndex::T_WAH;
			else if (idType == "de")
				indexType = sserialize::ItemIndex::T_DE;
			else if (idType == "rlede")
				indexType = sserialize::ItemIndex::T_RLE_DE;
			else if (idType == "native")
				indexType = sserialize::ItemIndex::T_NATIVE;
			++i;
		}
		else if (str == "-ci") {
			checkIndex = true;
		}
		else if (str == "--no-index-dedup") {
			indexDedup = false;
		}
		else if (str == "--create-kv" && i+1 < argc) {
			std::string ckvString(argv[i+1]);
			createKVStore = true;
			kvStoreConfig = KVStoreConfig(ckvString);
			i += 1;
		}
		else if (str == "--create-textsearch" && i+2 < argc) {
			std::string tsTypeStr(argv[i+1]), tsValues(argv[i+2]);
			TextSearchConfig tsCfg(tsValues);
			std::vector<liboscar::TextSearch::Type> tsTypes;
			if (tsTypeStr == "items") {
				tsTypes.push_back(liboscar::TextSearch::ITEMS);
			}
			else if (tsTypeStr == "geoh") {
				tsTypes.push_back(liboscar::TextSearch::GEOHIERARCHY);
			}
			else if (tsTypeStr == "geocell") {
				tsTypes.push_back(liboscar::TextSearch::GEOCELL);
			}
			else if (tsTypeStr == "geohitems") {
				tsTypes.push_back(liboscar::TextSearch::GEOHIERARCHY_AND_ITEMS);
			}
			else if (tsTypeStr == "all") {
				tsTypes.push_back(liboscar::TextSearch::ITEMS);
				tsTypes.push_back(liboscar::TextSearch::GEOHIERARCHY_AND_ITEMS);
				tsTypes.push_back(liboscar::TextSearch::GEOCELL);
			}
			else {
				std::cout << "Error: unknown text search type: " << tsTypeStr << std::endl;
				return RV_FAILED;
			}
			for(liboscar::TextSearch::Type tsType : tsTypes) {
				textSearchConfig.push_back(std::pair<liboscar::TextSearch::Type, TextSearchConfig>(tsType, tsCfg) );
			}
			i += 2;
		}
		else if (str == "--create-tagstore" && i+1 < argc) {
			tagStoreConfig = TagStoreConfig(argv[i+1]);
			++i;
		}
		else if (str == "--print-memory-usage") {
			memUsage = true;
		}
		else if (str == "--help" || str == "-h") {
			return RV_HELP;
		}
		else {
			if (str.size() && str.front() == '-') {
				std::cout << "unknown option: " << str << std::endl;
				return RV_FAILED;
			}
			else if (!sserialize::MmappedFile::fileExists(str)) {
				std::cout << "File " << str << " does not exist" << std::endl;
				return RV_FAILED;
			}
			else {
				inFileNames.push_back(std::string(argv[i]));
			}
		}
	}
	
	if (m_outFileName.size() == 0) {
		std::cout << "Arguments given: " << std::endl;
		for (int i=0; i < argc; i++) {
			std::cout << argv[i];
		}
		std::cout << std::endl << "Need out filename" << std::endl;
		return RV_FAILED;
	}

	if (inFileNames.size() == 0) {
		std::cout << std::endl << "Need in file name" << std::endl;
	}

	if (!createKVStore && inputIndexStore.empty()) {
		std::cout << "kvstore has missing associated inputIndexStore. Give it with -i path_to_index_store" << std::endl;
		return RV_FAILED;
	}

	if (kvStoreConfig.readBoundaries && !(kvStoreConfig.polyStoreLatCount && kvStoreConfig.polyStoreLonCount && kvStoreConfig.polyStoreMaxTriangPerCell)) {
		std::cout << "Boundary reading spec is invalid" << std::endl;
		return RV_FAILED;
	}
	
	return RV_OK;
}

std::string Config::help() {
	return std::string("-i <input.osm.pbf|input dir> -o <output dir> -c <config.json>");
}


sserialize::GeneralizedTrie::GeneralizedTrieCreatorConfig Config::toTrieConfig(const TextSearchConfig & cfg) {
	sserialize::GeneralizedTrie::GeneralizedTrieCreatorConfig config;
	config.deleteRootTrie = cfg.aggressiveMem;
	config.levelsWithoutFullIndex = cfg.levelsWithoutIndex;
	config.maxPrefixMergeCount = cfg.maxPrefixIndexMergeCount;
	config.maxSuffixMergeCount = cfg.maxSuffixIndexMergeCount;
	config.mergeIndex = cfg.mergeIndex;
	config.nodeType = cfg.nodeType;
	return config;
}

std::string Config::getOutFileDir() const {
	if (m_appendConfigToOutFileName) {
		std::stringstream ss;
		ss << m_outFileName;
		if (indexType == sserialize::ItemIndex::T_REGLINE)
			ss << "_rline";
		else if (indexType == sserialize::ItemIndex::T_WAH)
			ss << "_wah";
		else if (indexType == sserialize::ItemIndex::T_DE)
			ss << "_de";
		else if (indexType == sserialize::ItemIndex::T_RLE_DE)
			ss << "_rlede";
		else if (indexType == sserialize::ItemIndex::T_NATIVE)
			ss << "_native";

		std::string str = ss.str();
		if (str.size() > 245) {
			str.erase(245, 0xFFFFFFFF);
		}
		return str;
	}
	else {
		return m_outFileName;
	}
}

std::string Config::getOutFileName(liboscar::FileConfig fc) const {
	return liboscar::fileNameFromFileConfig(getOutFileDir(), fc, false);
}

sserialize::UByteArrayAdapter Config::ubaFromFC(liboscar::FileConfig fc) const {
	std::string fileName = liboscar::fileNameFromFileConfig(getOutFileDir(), fc, false);
	return sserialize::UByteArrayAdapter::openRo(fileName, false);
}

std::string Config::toString(sserialize::Static::TrieNode::Types nodeType) {
	if (nodeType == sserialize::Static::TrieNode::T_SIMPLE) {
		return "simple";
	}
	else if (nodeType == sserialize::Static::TrieNode::T_COMPACT) {
		return "compact";
	}
	else if (nodeType == sserialize::Static::TrieNode::T_LARGE_COMPACT) {
		return "large_compact";
	}
	else {
		return "unsupported";
	}
}
*/

/*
std::ostream& operator<<(std::ostream& out, const Config & opts) {
	if (opts.createKVStore) {
		std::cout << "Creating KeyValueStore at " << opts.getOutFileName(liboscar::FC_KV_STORE) << std::endl;
		out << "Number of threads: " << opts.kvStoreConfig.numThreads << std::endl;
		out << "Max node table entries: " << sserialize::toString(opts.kvStoreConfig.maxNodeHashTableSize) << std::endl;
		out << "Keys whose values are infalted: " << opts.kvStoreConfig.keysValuesToInflate << std::endl;
		out << "Node HashTable size: ";
		if (opts.kvStoreConfig.autoMaxMinNodeId) {
			out << "auto";
		}
		else {
			out << opts.kvStoreConfig.minNodeId << "->" << opts.kvStoreConfig.maxNodeId;
		}
		out << std::endl;
		out << "ItemSaveDirector rule: ";
		if (opts.kvStoreConfig.saveEverything) {
			out << "everything" << std::endl;
		}
		else if (!opts.kvStoreConfig.itemsIgnoredByKeyFn.empty()) {
			out << "Ignore items with tags in" << opts.kvStoreConfig.itemsIgnoredByKeyFn << std::endl;
		}
		else {
			if (opts.kvStoreConfig.saveEveryTag) {
				out << "every tag";
			}
			else {
				out << "according to keys and key, values";
			}
			out << std::endl;
			out <<  "Key to store: " << opts.kvStoreConfig.keyToStoreFn << std::endl;
			out <<  "Key=Value to store: " << opts.kvStoreConfig.keyValuesToStoreFn << std::endl;
			out <<  "Items with key= to store: " << opts.kvStoreConfig.itemsSavedByKeyFn << std::endl;
			out <<  "Items with key=value to store: " << opts.kvStoreConfig.itemsSavedByKeyValueFn << std::endl;
		}
		out << "Score config: " << opts.kvStoreConfig.scoreConfigFileName << std::endl;
		out << "Item sort order: ";
		switch(opts.kvStoreConfig.itemSortOrder) {
		case oscar_create::OsmKeyValueObjectStore::ISO_NONE:
			out << "none";
			break;
		case oscar_create::OsmKeyValueObjectStore::ISO_SCORE:
			out << "score";
			break;
		case oscar_create::OsmKeyValueObjectStore::ISO_SCORE_NAME:
			out << "score, name";
			break;
		case oscar_create::OsmKeyValueObjectStore::ISO_SCORE_PRIO_STRINGS:
			out << opts.kvStoreConfig.prioStringsFileName;
			break;
		default:
			out << "invalid";
			break;
		}
		out << std::endl;
		out << "Read boundaries: ";
		if (opts.kvStoreConfig.readBoundaries) {
			out << "initial=" << opts.kvStoreConfig.polyStoreLatCount << "x" << opts.kvStoreConfig.polyStoreLonCount;
			out << ", max triangle per cell="<< opts.kvStoreConfig.polyStoreMaxTriangPerCell << ", max triangle centroid dist=" << opts.kvStoreConfig.triangMaxCentroidDist << std::endl;
		}
		else {
			out << "no" << std::endl;
		}
		std::cout << "Keys defining regions=" << opts.kvStoreConfig.keysDefiningRegions << std::endl;
		std::cout << "Key:Values defining regions=" << opts.kvStoreConfig.keyValuesDefiningRegions << std::endl;
		out << " FullRegionIndex: " << (opts.kvStoreConfig.fullRegionIndex ? "yes" : "no" ) << std::endl;
	}
	out << "Selected index type: ";
	switch (opts.indexType) {
	case (sserialize::ItemIndex::T_WAH):
		out << "rle word aligned bit vector";
		break;
	case (sserialize::ItemIndex::T_DE):
		out << "delta encoding";
		break;
	case (sserialize::ItemIndex::T_REGLINE):
		out << "regression line";
		break;
	case (sserialize::ItemIndex::T_RLE_DE):
		out << "delta+run-length encoding";
		break;
	case (sserialize::ItemIndex::T_SIMPLE):
		out << "simple";
		break;
	case (sserialize::ItemIndex::T_NATIVE):
		out << "native";
		break;
	default:
		out << "I'm going to eat your kitten!";
		break;
	}
	out << std::endl;
	out << "Index deduplication: " << sserialize::toString(opts.indexDedup) << std::endl;
		
	if (!opts.createKVStore) {
		if (opts.tagStoreConfig.create) {
			out << "TagStore:" << std::endl;
			out << "\tConsidered keys: " << opts.tagStoreConfig.tagKeys << std::endl;
			out << "\tConsidered Key=Values" << opts.tagStoreConfig.tagKeyValues << std::endl;
		}
		
		for(const std::pair<liboscar::TextSearch::Type, oscar_create::TextSearchConfig> & x : opts.textSearchConfig) {
			std::cout << "Text-Search-Type:";
			if (x.first == liboscar::TextSearch::GEOHIERARCHY) {
				std::cout << "GeoHierarchy";
			}
			else if (x.first == liboscar::TextSearch::GEOHIERARCHY_AND_ITEMS) {
				std::cout << "GeoHierarchy with Items";
			}
			else if (x.first == liboscar::TextSearch::GEOCELL) {
				std::cout << "GeoCell";
			}
			else if (x.first == liboscar::TextSearch::ITEMS) {
				std::cout << "Items";
			}
			out << "\n";
			out << "\tcase-sensitive=" << sserialize::toString(x.second.caseSensitive) << std::endl;
			out << "\tdiacritic-insensitive=" << sserialize::toString(x.second.diacritcInSensitive) << std::endl;
			out << "\tsuffixes=" << sserialize::toString(x.second.suffixes) << std::endl;
			if (x.second.suffixDelimeters.size()) {
				out << "\tSuffix delimeters: " << sserialize::stringFromUnicodePoints(x.second.suffixDelimeters.cbegin(), x.second.suffixDelimeters.cend()) << std::endl;
			}
			if (x.second.levelsWithoutIndex.size()) {
				out << "\tLevels without a full index: ";
				for(std::set<uint8_t>::iterator it = x.second.levelsWithoutIndex.cbegin(); it != x.second.levelsWithoutIndex.cend(); it++) {
					out << static_cast<int>(*it) << ", ";
				}
				out << std::endl;
			}
			out << "\tMaximum merge count for no full prefix index:" << x.second.maxPrefixIndexMergeCount << std::endl;
			out << "\tMaximum merge count for no full suffix index:" << x.second.maxSuffixIndexMergeCount << std::endl;
			out << "\tKeys to use for the text search: " << x.second.keyFile << std::endl;
			out << "\tTags to store  for prefix search: " << x.second.storeTagsPrefixFile << std::endl;
			out << "\tTags to store  for suffix search: " << x.second.storeTagsSuffixFile << std::endl;
			out << "\tNodeType: " << oscar_create::Config::toString(x.second.nodeType) << std::endl;
			out << "\tAggressive memory usage: " <<  sserialize::toString(x.second.aggressiveMem) << std::endl;
			out << "\tMemory storage type: ";
			switch (x.second.mmType) {
			case sserialize::MM_FILEBASED:
				out << "file-based";
				break;
			case sserialize::MM_PROGRAM_MEMORY:
				out << "program memory";
				break;
			case sserialize::MM_SHARED_MEMORY:
				out << "shared memory";
				break;
			default:
				out << "invalid";
				break;
			}
			out << std::endl;
			out << "\tMerge Index: " << sserialize::toString(x.second.mergeIndex) <<  std::endl;
			out << "\tTrie-Type: ";
			if (x.second.type == oscar_create::TextSearchConfig::Type::FULL_INDEX_TRIE) {
				out << "full-index-trie";
			}
			else if (x.second.type == oscar_create::TextSearchConfig::Type::TRIE) {
				out << "trie";
			}
			else if (x.second.type == oscar_create::TextSearchConfig::Type::FLAT_GST) {
				out << "fgst";
			}
			else if (x.second.type == oscar_create::TextSearchConfig::Type::FLAT_TRIE) {
				out << "flattrie";
			}
			out << std::endl;
			out << "\tExtensive Checking: " << sserialize::toString(x.second.extensiveChecking) <<  std::endl;
			out << "\tThread count: " << x.second.threadCount << std::endl;
		}
		if ( (opts.gridLatCount != 0 && opts.gridLonCount != 0) || (opts.gridRTreeLatCount != 0 && opts.gridRTreeLonCount != 0)) {
			out << "GeoSearch:" << std::endl;
			if (opts.gridLatCount != 0 && opts.gridLonCount != 0) {
				out << "\tGeoGrid:" << opts.gridLatCount << "x" << opts.gridLonCount << std::endl;
			}
			if (opts.gridRTreeLatCount != 0 && opts.gridRTreeLonCount != 0) {
				out << "Grided RTree:" << opts.gridRTreeLatCount << "x" << opts.gridRTreeLonCount;
			}
		}
	}

	out << "out-file-name: " << opts.getOutFileDir() << std::endl;
	return out;
}
*/
}//end namespace oscar_create