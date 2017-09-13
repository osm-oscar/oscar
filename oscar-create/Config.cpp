#include "Config.h"
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/printers.h>
#include <sserialize/strings/stringfunctions.h>
#include <liboscar/constants.h>
#include "OsmKeyValueObjectStore.h"
#include <json/json.h>

namespace oscar_create {

//Print functions----------------

std::string toString(bool v) {
	return (v ? "yes" : "no");
}

std::string toString(sserialize::MmappedMemoryType mmt) {
	switch (mmt) {
	case sserialize::MM_FILEBASED:
		return "file-based";
	case sserialize::MM_PROGRAM_MEMORY:
		return "program memory";
	case sserialize::MM_SHARED_MEMORY:
		return "shared memory";
	default:
		return "invalid";
	}
}

std::string toString(sserialize::ItemIndex::Types v) {
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
	case (sserialize::ItemIndex::T_ELIAS_FANO):
		return "elias-fano";
	default:
		return "invalid";
	}
}

std::string toString(sserialize::Static::TrieNode::Types nodeType) {
	switch (nodeType) {
	case sserialize::Static::TrieNode::T_COMPACT:
		return "compact";
	case sserialize::Static::TrieNode::T_LARGE_COMPACT:
		return "large_compact";
	case sserialize::Static::TrieNode::T_SIMPLE:
		return "simple";
	default:
		return "invalid";
	};
}

sserialize::MmappedMemoryType parseMMT(const std::string str) {
	sserialize::MmappedMemoryType mmType = sserialize::MM_INVALID;
	if (str == "prg") {
		mmType = sserialize::MM_PROGRAM_MEMORY;
	}
	else if (str == "shm") {
		mmType = sserialize::MM_SHARED_MEMORY;
	}
	else if (str == "slowfile") {
		mmType = sserialize::MM_SLOW_FILEBASED;
	}
	else if (str == "fastfile") {
		mmType = sserialize::MM_FAST_FILEBASED;
	}
	return mmType;
}

std::string toAbsolutePath(const std::string & str, const std::string & base) {
	if (!str.size()) {
		return str;
	}
	if (sserialize::MmappedFile::isAbsolute(str)) { //absolute path
		return str;
	}
	else { //relative path
		return sserialize::MmappedFile::realPath(base + "/" + str);
	}
}

std::ostream& StatsConfig::print(std::ostream& out) const {
	out << "print memusage: " << toString(memUsage);
	return out;
}

std::ostream& IndexStoreConfig::print(std::ostream& out) const {
	out << "input store: " << inputStore << "\n";
	out << "type: " << toString(type) << "\n";
	out << "check: " << toString(check) << "\n";
	out << "deduplicate: " << toString(deduplicate);
	return out;
}

std::ostream& GridConfig::print(std::ostream& out) const {
	out << "latCount: " << latCount << "\nlonCount: " << lonCount;
	return out;
}

std::ostream& RTreeConfig::print(std::ostream& out) const {
	out << "latCount: " << latCount << "\nlonCount: " << lonCount;
	return out;
}

std::ostream& TagStoreConfig::print(std::ostream& out) const {
	out << "key file: " << tagKeys << "\n";
	out << "key-values file: " << tagKeyValues;
	return out;
}

std::ostream& KVStoreConfig::print(std::ostream& out) const {
	out << "Number of threads: " << numThreads << "\n";
	out << "Number of blobs to fetch at once: " << blobFetchCount << "\n";
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
		out << "\n";
		out << "\tgrt=" << grtLatCount << "x" << grtLonCount << " split till diag < " << grtMinDiag << '\n';
		out << "\tinitial=" << latCount << "x" << lonCount << "\n";
		out << "\tmax triangle per cell="<< maxTriangPerCell << "\n";
		out << "\tmax triangle centroid dist=" << maxTriangCentroidDist;
	}
	else {
		out << "no";
	}
	out << "\n";
	out << "Keys defining regions=" << keysDefiningRegions << "\n";
	out << "Key:Values defining regions=" << keyValuesDefiningRegions << "\n";
	out << "FullRegionIndex: " << (fullRegionIndex ? "yes" : "no" ) << "\n";
	out << "Add parent info: " << (addParentInfo ? "yes" : "no") << "\n";
	out << "Add regions to cells they enclose: " << (addRegionsToCells ? "yes" : "no") << '\n';
	out << "Geometry cleaning: ";
	switch (gct) {
	case sserialize::Static::spatial::Triangulation::GCT_NONE:
		out << "none";
		break;
	case sserialize::Static::spatial::Triangulation::GCT_REMOVE_DEGENERATE_FACES:
		out << "remove degenerate faces";
		break;
	case sserialize::Static::spatial::Triangulation::GCT_SNAP_VERTICES:
		out << "snapp vertices";
		break;
	default:
		out << "invalid";
		break;
	}
	out << "\n";
	return out;
}

std::ostream& TextSearchConfig::SearchCapabilities::print(std::ostream& out) const {
	out << "\tcaseSensitive: " << (caseSensitive ? "yes" : "no") << "\n";
	out << "\tdiacritcInSensitive: " << (diacritcInSensitive ? "yes" : "no") << "\n";
	out << "\tfile: " << fileName;
	return out;
}

std::ostream& TextSearchConfig::print(std::ostream& out) const {
	std::array<std::string, 2> itemTypeNames{{"items", "regions"}};
	std::array<std::string, 2> tagTypeNames{{"value's keys", "key-values"}};
	std::array<std::string, 2> queryTypeNames{{"prefix", "suffix"}};
	
	for(uint32_t itemType(0); itemType < 2; ++itemType) {
		for(uint32_t tagType(0); tagType < 2; ++tagType) {
			for(uint32_t queryType(0); queryType < 2; ++queryType) {
				if (searchCapabilites[itemType][tagType][queryType].enabled) {
					out << itemTypeNames[itemType] << "::" << tagTypeNames[tagType] << "::" << queryTypeNames[queryType] << ":\n";
					out << searchCapabilites[itemType][tagType][queryType] << "\n";
				}
			}
		}
	}
	return out;
}

std::ostream & ItemSearchConfig::print(std::ostream& out) const {
	TextSearchConfig::print(out);
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
	out << "\tNodeType: " << toString(nodeType) << std::endl;
	out << "\tAggressive memory usage: " <<  sserialize::toString(aggressiveMem) << std::endl;
	out << "\tMemory storage type: " << toString(mmType) << "\n";
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
	out << "\tExtensive Checking: " << sserialize::toString(check) <<  "\n";
	out << "\tThread count: " << threadCount;
	return out;
}

std::ostream& GeoHierarchyItemsSearchConfig::print(std::ostream& out) const {
    return oscar_create::ItemSearchConfig::print(out);
}

std::ostream & GeoHierarchySearchConfig::print(std::ostream& out) const {
	return ItemSearchConfig::print(out);
}


std::ostream & GeoCellConfig::print(std::ostream& out) const {
	TextSearchConfig::print(out);
	if (suffixDelimeters.size()) {
		out << "\tSuffix delimeters: " << sserialize::stringFromUnicodePoints(suffixDelimeters.cbegin(), suffixDelimeters.cend()) << "\n";
	}
	out << "Thread count: " << threadCount << "\n";
	out << "Trie type: ";
	if (trieType == TrieType::TRIE) {
		out << "trie";
	}
	else if (trieType == TrieType::FLAT_TRIE) {
		out << "flattrie";
	}
	out << "\nmmt: " << toString(mmType);
	out << "\ncheck: " << toString(check);
	return out;
}

std::ostream & OOMGeoCellConfig::print(std::ostream& out) const {
	TextSearchConfig::print(out);
	out << "Thread count: " << threadCount << "\n";
	out << "Sort concurrency: " << sortConcurrency << '\n';
	out << "Max memory usage: " << sserialize::prettyFormatSize(maxMemoryUsage);
	return out;
}

std::ostream& Config::print(std::ostream& out) const {
	out << "Stats config:\n";
	out << statsConfig << "\n";
	if (indexStoreConfig) {
		out << "IndexStoreConfig: \n";
		out << *indexStoreConfig << "\n";
	}
	if (kvStoreConfig && kvStoreConfig->enabled) {
		out << "KVStoreConfig:\n"; 
		out << *kvStoreConfig << "\n";
	}
	if (gridConfig && gridConfig->enabled) {
		out << "GridConfig:\n";
		out << *gridConfig << "\n";
	}
	if (rTreeConfig && rTreeConfig->enabled) {
		out << "RTreeConfig:\n";
		out << *rTreeConfig << "\n";
	}
	if (tagStoreConfig && tagStoreConfig->enabled) {
		out << "TagStoreConfig:\n";
		out << *tagStoreConfig << "\n";
	}
	for(TextSearchConfig * x : textSearchConfig) {
		if (!x || !(x->enabled)) {
			continue;
		}
		out << "TextSearchConfig[";
		switch (x->type) {
		case liboscar::TextSearch::ITEMS:
			out << "items";
			break;
		case liboscar::TextSearch::GEOCELL:
			out << "geocell";
			break;
		case liboscar::TextSearch::OOMGEOCELL:
			out << "oomgeocell";
			break;
		case liboscar::TextSearch::GEOHIERARCHY:
			out << "geohierarchy";
			break;
		default:
			out << "invalid";
			break;
		}
		out << "]:\n";
		out << *x << "\n";
	}
	return out;
}

//validation functions

bool StatsConfig::valid() const {
	return true;
}

bool IndexStoreConfig::valid() const {
	return sserialize::ItemIndex::T_NULL != type;
}

bool GridConfig::valid() const {
	return latCount != 0 && lonCount != 0;
}

bool RTreeConfig::valid() const {
	return latCount != 0 && lonCount != 0;
}

bool TagStoreConfig::valid() const {
	if (enabled) {
		return sserialize::MmappedFile::fileExists(tagKeys) &&
				sserialize::MmappedFile::fileExists(tagKeyValues);
	}
	return true;
}

bool KVStoreConfig::valid() const {
	return true;
}

bool TextSearchConfig::valid() const {
	for(uint32_t itemType(0); itemType < 2; ++itemType) {
		for(uint32_t tagType(0); tagType < 2; ++tagType) {
			for(uint32_t queryType(0); queryType < 2; ++queryType) {
				const SearchCapabilities & cap = searchCapabilites[itemType][tagType][queryType];
				if (cap.enabled && !sserialize::MmappedFile::fileExists(cap.fileName)) {
					return false;
				}
			}
		}
	}
	if (!consistentCaseSensitive()) {
		return false;
	}
	return true;
}

bool ItemSearchConfig::valid() const {
	return oscar_create::TextSearchConfig::valid() && mmType != sserialize::MM_INVALID &&
		nodeType != sserialize::Static::TrieNode::T_EMPTY;
}

bool GeoHierarchyItemsSearchConfig::valid() const {
    return oscar_create::ItemSearchConfig::valid();
}

bool GeoHierarchySearchConfig::valid() const {
    return oscar_create::ItemSearchConfig::valid();
}

bool GeoCellConfig::valid() const {
	return oscar_create::TextSearchConfig::valid() && mmType != sserialize::MM_INVALID;
}

bool OOMGeoCellConfig::valid() const {
    return oscar_create::TextSearchConfig::valid();
}


//parse functions
StatsConfig::StatsConfig(const Json::Value & cfg) : StatsConfig() {
	update(cfg);
}

void StatsConfig::update(const Json::Value& cfg) {
	Json::Value v = cfg["print-memory-usage"];
	if (v.isBool()) {
		memUsage = v.asBool();
	}
}


IndexStoreConfig::IndexStoreConfig(const Json::Value & cfg) :
type(sserialize::ItemIndex::T_NULL),
check(false),
deduplicate(true)
{
	update(cfg);
}

void IndexStoreConfig::update(const Json::Value & cfg) {
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
		else if (t == "eliasfano") {
			type = sserialize::ItemIndex::T_ELIAS_FANO;
		}
		else {
			throw sserialize::ConfigurationException("IndexStoreConfig", "invalid index type");
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
enabled(false),
latCount(0),
lonCount(0)
{
	update(cfg);
}

void GridConfig::update(const Json::Value& cfg) {
	Json::Value v = cfg["enabled"];
	if (v.isBool()) {
		enabled = v.asBool();
	}
	
	v = cfg["latcount"];
	if (v.isNumeric()) {
		latCount = v.asUInt();
	}
	
	v = cfg["loncount"];
	if (v.isNumeric()) {
		lonCount = v.asUInt();
	}
}


RTreeConfig::RTreeConfig(const Json::Value& cfg) :
enabled(false),
latCount(0),
lonCount(0)
{
	update(cfg);
}

void RTreeConfig::update(const Json::Value& cfg) {
	Json::Value v = cfg["enabled"];
	if (v.isBool()) {
		enabled = v.asBool();
	}
	
	v = cfg["latcount"];
	if (v.isNumeric()) {
		latCount = v.asUInt();
	}
	
	v = cfg["loncount"];
	if (v.isNumeric()) {
		lonCount = v.asUInt();
	}
}

TagStoreConfig::TagStoreConfig(const Json::Value& cfg, const std::string & basePath) :
enabled(false)
{
	update(cfg, basePath);
}

void TagStoreConfig::update(const Json::Value& cfg, const std::string& basePath) {
	Json::Value v = cfg["enabled"];
	if (v.isBool()) {
		enabled = v.asBool();
	}
	
	v = cfg["keys"];
	if (v.isString()) {
		tagKeys = toAbsolutePath(v.asString(), basePath);
	}
	
	v = cfg["keyvalues"];
	if (v.isString()) {
		tagKeyValues = toAbsolutePath(v.asString(), basePath);
	}
}

KVStoreConfig::KVStoreConfig(const Json::Value& cfg, const std::string & basePath) :
enabled(false),
maxNodeHashTableSize(std::numeric_limits<uint32_t>::max()),
saveEverything(false),
saveEveryTag(false),
minNodeId(0),
maxNodeId(0),
autoMaxMinNodeId(false),
readBoundaries(false),
fullRegionIndex(false),
addParentInfo(false),
addRegionsToCells(false),
latCount(0),
lonCount(0),
grtLatCount(10),
grtLonCount(10),
grtMinDiag(10000),
maxTriangPerCell(std::numeric_limits<uint32_t>::max()),
maxTriangCentroidDist(std::numeric_limits<double>::max()),
numThreads(0),
blobFetchCount(1),
itemSortOrder(OsmKeyValueObjectStore::ISO_NONE),
gct(sserialize::Static::spatial::Triangulation::GCT_NONE)
{
	update(cfg, basePath);
}

void KVStoreConfig::update(const Json::Value& cfg, const std::string & basePath) {
	Json::Value v = cfg["enabled"];
	if (v.isBool()) {
		enabled = v.asBool();
	}
	
	v = cfg["blobFetchCount"];
	if (v.isNumeric()) {
		blobFetchCount = v.asUInt();
	}
	
	v = cfg["threadCount"];
	if (v.isNumeric()) {
		numThreads = v.asUInt();
	}
	
	v = cfg["fullRegionIndex"];
	if (v.isBool()) {
		fullRegionIndex = v.asBool();
	}
	
	v = cfg["addRegionsToCells"];
	if (v.isBool()) {
		addRegionsToCells = v.asBool();
	}
	
	v = cfg["latCount"];
	if (v.isNumeric()) {
		latCount = v.asUInt();
	}
	
	v = cfg["lonCount"];
	if (v.isNumeric()) {
		lonCount = v.asUInt();
	}
	
	v = cfg["grtLatCount"];
	if (v.isNumeric()) {
		grtLatCount = v.asUInt();
	}

	v = cfg["grtLonCount"];
	if (v.isNumeric()) {
		grtLonCount = v.asUInt();
	}
	
	v = cfg["grtMinDiag"];
	if (v.isNumeric()) {
		grtMinDiag = v.asDouble();
	}
	
	v = cfg["readBoundaries"];
	if (latCount && lonCount && v.isBool()) {
		readBoundaries = v.asBool();
	}
	
	v = cfg["maxTriangPerCell"];
	if (v.isNumeric()) {
		int64_t tmp = v.asInt64();
		if (tmp < 0) {
			maxTriangPerCell = std::numeric_limits<uint32_t>::max();
		}
		else {
			maxTriangPerCell = tmp;
		}
	}
	
	v = cfg["maxTriangCentroidDist"];
	if (v.isNumeric()) {
		maxTriangCentroidDist = v.asDouble();
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
			keyToStoreFn = toAbsolutePath(tmp.asString(), basePath);
		}
		tmp = v["keyValues"];
		if (tmp.isString()) {
			keyValuesToStoreFn = toAbsolutePath(tmp.asString(), basePath);
		}
	}
	else if (v.isString() && v.asString() == "all") {
		saveEveryTag = true;
	}
	
	v = cfg["itemFilter"];
	if (v.isObject()) {
		Json::Value tmp = v["keys"];
		if (tmp.isString()) {
			itemsSavedByKeyFn = toAbsolutePath(tmp.asString(), basePath);
		}
		tmp = v["keyValues"];
		if (tmp.isString()) {
			itemsSavedByKeyValueFn = toAbsolutePath(tmp.asString(), basePath);
		}
	}
	else if (v.isString() && v.asString() == "all") {
		saveEverything = true;
	}
	
	v = cfg["regionFilter"];
	if (v.isObject()) {
		Json::Value tmp = v["keys"];
		if (tmp.isString()) {
			keysDefiningRegions = toAbsolutePath(tmp.asString(), basePath);
		}
		tmp = v["keyValues"];
		if (tmp.isString()) {
			keyValuesDefiningRegions = toAbsolutePath(tmp.asString(), basePath);
		}
	}
	
	v = cfg["splitValues"];
	if (v.isString()) {
		keysValuesToInflate = toAbsolutePath(v.asString(), basePath);
	}
	
	v = cfg["scoring"];
	if (v.isObject()) {
		Json::Value tmp = v["config"];
		if (tmp.isString()) {
			scoreConfigFileName = toAbsolutePath(tmp.asString(), basePath);
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
			else if (order == "priority") {
				if (v["priority"].isString()) {
					prioStringsFileName = toAbsolutePath(v["priority"].asString(), basePath);
					itemSortOrder = OsmKeyValueObjectStore::ISO_SCORE_PRIO_STRINGS;
				}
				else {
					throw sserialize::ConfigurationException("KVStoreConfig", "sort order priority expects priority definition file");
				}
			}
		}
	}
	v = cfg["geoclean"];
	if (v.isString()) {
		std::string token = v.asString();
		if (token == "none") {
			gct = sserialize::Static::spatial::Triangulation::GCT_NONE;
		}
		else if (token == "rdf" || token == "remove degenerate faces") {
			gct = sserialize::Static::spatial::Triangulation::GCT_REMOVE_DEGENERATE_FACES;
		}
		else if (token == "sv" || token == "total" || token == "snap vertices" || token == "all") {
			gct = sserialize::Static::spatial::Triangulation::GCT_SNAP_VERTICES;
		}
		else {
			throw sserialize::ConfigurationException("KVStoreConfig", "Unkown GeometryCleaningType: " + token);
		}
	}
}


TextSearchConfig::TextSearchConfig(const Json::Value& cfg, const std::string & basePath) :
enabled(false),
type(liboscar::TextSearch::INVALID)
{
	update(cfg, basePath);
}

void TextSearchConfig::update(const Json::Value& cf, const std::string& basePath) {
	Json::Value v = cf["enabled"];
	if (v.isBool()) {
		enabled = v.asBool();
	}
	
	v = cf["items"];
	if (v.isObject()) {
		parseTagTypeObject(v, basePath, ItemType::ITEM);
	}
	
	v = cf["regions"];
	if (v.isObject()) {
		parseTagTypeObject(v, basePath, ItemType::REGION);
	}
}


void TextSearchConfig::parseTagTypeObject(const Json::Value& cfg, const std::string& basePath, TextSearchConfig::ItemType itemType) {
	Json::Value v = cfg["values"];
	if (v.isObject()) {
		parseQueryTypeObject(v, basePath, itemType, TagType::VALUES);
	}
	
	v = cfg["tags"];
	if (v.isObject()) {
		parseQueryTypeObject(v, basePath, itemType, TagType::TAGS);
	}
}

void TextSearchConfig::parseQueryTypeObject(const Json::Value& cfg, const std::string& basePath, oscar_create::TextSearchConfig::ItemType itemType, oscar_create::TextSearchConfig::TagType tagType) {
	Json::Value v = cfg["prefix"];
	if (v.isObject()) {
		parseKvConfig(v, basePath, itemType, tagType, QueryType::PREFIX);
	}
	
	v = cfg["substring"];
	if (v.isObject()) {
		parseKvConfig(v, basePath, itemType, tagType, QueryType::SUBSTRING);
	}
}

void TextSearchConfig::parseKvConfig(const Json::Value& cfg, const std::string& basePath, oscar_create::TextSearchConfig::ItemType itemType, oscar_create::TextSearchConfig::TagType tagType, oscar_create::TextSearchConfig::QueryType qt) {
	SearchCapabilities & cap = searchCapabilites[(int)itemType][(int)tagType][(int)qt];
	
	Json::Value v = cfg["enabled"];
	if(v.isBool()) {
		cap.enabled = v.asBool();
	}
	
	v = cfg["caseSensitive"];
	if(v.isBool()) {
		cap.caseSensitive = v.asBool();
	}
	
	v = cfg["diacriticSensitive"];
	if(v.isBool()) {
		cap.diacritcInSensitive = !v.asBool();
	}
	
	v = cfg["file"];
	if (v.isString()) {
		cap.fileName = toAbsolutePath(v.asString(), basePath);
	}
}

TextSearchConfig* TextSearchConfig::parseTyped(const Json::Value& cfg, const std::string & basePath, TextSearchConfig * base) {
	Json::Value tv = cfg["type"];
	Json::Value cv = cfg["config"];
	TextSearchConfig * result = 0;
	if (tv.isString() && cv.isObject()) {
		std::string t = tv.asString();
		if (t == "items") {
			if (base && base->type == liboscar::TextSearch::ITEMS) {
				result = base;
				result->update(cv, basePath);
			}
			else {
				result = new ItemSearchConfig(cv, basePath);
				result->type = liboscar::TextSearch::ITEMS;
			}
		}
		else if (t == "regions") {
			if (base && base->type == liboscar::TextSearch::GEOHIERARCHY) {
				result = base;
				result->update(cv, basePath);
			}
			else {
				result = new GeoHierarchySearchConfig(cv, basePath);
				result->type = liboscar::TextSearch::GEOHIERARCHY;
			}
		}
		else if (t == "geoitems") {
			if (base && base->type == liboscar::TextSearch::GEOHIERARCHY) {
				result = base;
				result->update(cv, basePath);
			}
			else {
				result = new GeoHierarchyItemsSearchConfig(cv, basePath);
				result->type = liboscar::TextSearch::GEOHIERARCHY;
			}
		}
		else if (t == "geocell") {
			if (base && base->type == liboscar::TextSearch::GEOCELL) {
				result = base;
				result->update(cv, basePath);
			}
			else {
				result = new GeoCellConfig(cv, basePath);
				result->type = liboscar::TextSearch::GEOCELL;
			}
		}
		else if (t == "oomgeocell") {
			if (base && base->type == liboscar::TextSearch::OOMGEOCELL) {
				result = base;
				result->update(cv, basePath);
			}
			else {
				result = new OOMGeoCellConfig(cv, basePath);
				result->type = liboscar::TextSearch::OOMGEOCELL;
			}
		}
		else {
			std::cerr << "Invalid text search type" << std::endl;
		}
	}
	else {
		std::cerr << "Invalid text search config" << std::endl;
	}
	return result;
}

ItemSearchConfig::ItemSearchConfig(const Json::Value& cfg, const std::string & basePath) :
TextSearchConfig(cfg, basePath),
mergeIndex(false),
aggressiveMem(false),
mmType(sserialize::MM_SHARED_MEMORY),
trieType(TrieType::FULL_INDEX_TRIE),
nodeType(sserialize::Static::TrieNode::T_LARGE_COMPACT),
maxPrefixIndexMergeCount(0),
maxSuffixIndexMergeCount(0),
check(false),
threadCount(0)
{
	updateSelf(cfg);
}

void ItemSearchConfig::update(const Json::Value& cfg, const std::string& basePath) {
	TextSearchConfig::update(cfg, basePath);
	updateSelf(cfg);
}

void ItemSearchConfig::updateSelf(const Json::Value& cfg) {
	Json::Value v = cfg["suffixDelimeters"];
	if (v.isString()) {
		std::string str = v.asString();
		for(std::string::const_iterator it(str.begin()), end(str.end()); it != end; ) {
			suffixDelimeters.insert( utf8::next(it, end) );
		}
	}
	
	v = cfg["mergeIndex"];
	if (v.isBool()) {
		mergeIndex = v.asBool();
	}
	
	v = cfg["aggressiveMemory"];
	if (v.isBool()) {
		aggressiveMem = v.asBool();
	}
	
	v = cfg["mmt"];
	if (v.isString()) {
		mmType = parseMMT(v.asString());
	}
	
	v = cfg["trieType"];
	if (v.isString()) {
		std::string str = v.asString();
		if (str == "trie") {
			trieType = TrieType::TRIE;
		}
		else if (str == "fgst") {
			trieType = TrieType::FLAT_GST;
		}
		else if (str == "flattrie") {
			trieType = TrieType::FLAT_TRIE;
		}
		else if (str == "fitrie") {
			trieType = TrieType::FULL_INDEX_TRIE;
		}
		else {
			std::cout << "Unrecognized trie type: " << str << std::endl;
		}
	}
	
	v = cfg["nodeType"];
	if (v.isString()) {
		std::string str = v.asString();
		if (str == "simple") {
			nodeType = sserialize::Static::TrieNode::T_SIMPLE;
		}
		else if (str == "compact") {
			nodeType = sserialize::Static::TrieNode::T_COMPACT;
		}
		else if (str == "large") {
			nodeType = sserialize::Static::TrieNode::T_LARGE_COMPACT;
		}
		else {
			std::cout << "Unrecognized node type: " << str << std::endl;
		}
	}
	
	v = cfg["levelsWithoutIndex"];
	if (v.isArray()) {
		for(const Json::Value & x : v) {
			if (x.isNumeric()) {
				levelsWithoutIndex.insert(x.asUInt());
			}
		}
	}
	
	v = cfg["maxPrefixIndexMergeCount"];
	if (v.isNumeric()) {
		maxPrefixIndexMergeCount = v.asInt();
	}
	
	v = cfg["maxSuffixIndexMergeCount"];
	if (v.isNumeric()) {
		maxSuffixIndexMergeCount = v.asInt();
	}
	
	v = cfg["check"];
	if (v.isBool()) {
		check = v.asBool();
	}
	
	v = cfg["threadCount"];
	if (v.isNumeric()) {
		threadCount = v.asUInt();
	}
}

GeoHierarchyItemsSearchConfig::GeoHierarchyItemsSearchConfig(const Json::Value& cfg, const std::string & basePath) :
ItemSearchConfig(cfg, basePath)
{}

GeoHierarchySearchConfig::GeoHierarchySearchConfig(const Json::Value& cfg, const std::string& basePath) :
ItemSearchConfig(cfg, basePath)
{}

GeoCellConfig::GeoCellConfig(const Json::Value & cfg, const std::string & basePath) :
TextSearchConfig(cfg, basePath),
threadCount(0),
trieType(TrieType::FLAT_TRIE),
mmType(sserialize::MM_SHARED_MEMORY),
check(false)
{
	updateSelf(cfg);
}

void GeoCellConfig::update(const Json::Value& cfg, const std::string& basePath) {
	oscar_create::TextSearchConfig::update(cfg, basePath);
	updateSelf(cfg);
}

void GeoCellConfig::updateSelf(const Json::Value& cfg) {
	Json::Value v = cfg["threadCount"];
	if (v.isNumeric()) {
		threadCount = v.asUInt();
	}
	
	v = cfg["suffixDelimeters"];
	if (v.isString()) {
		std::string str = v.asString();
		for(std::string::const_iterator it(str.begin()), end(str.end()); it != end; ) {
			suffixDelimeters.insert( utf8::next(it, end) );
		}
	}
	
	v = cfg["trieType"];
	if (v.isString()) {
		std::string str = v.asString();
		if (str == "trie") {
			trieType = TrieType::TRIE;
		}
		else if (str == "flattrie") {
			trieType = TrieType::FLAT_TRIE;
		}
		else {
			std::cout << "Unrecognized trie type: " << str << std::endl;
		}
	}
	
	v = cfg["mmt"];
	if (v.isString()) {
		mmType = parseMMT(v.asString());
	}
	
	v = cfg["check"];
	if (v.isBool()) {
		check = v.asBool();
	}
}

OOMGeoCellConfig::OOMGeoCellConfig(const Json::Value& cfg, const std::string& basePath) :
TextSearchConfig(cfg, basePath),
threadCount(0),
sortConcurrency(0),
maxMemoryUsage(0xFFFFFFFF)
{
	updateSelf(cfg);
}

void OOMGeoCellConfig::update(const Json::Value& cfg, const std::string& basePath) {
	TextSearchConfig::update(cfg, basePath);
	updateSelf(cfg);
}

void OOMGeoCellConfig::updateSelf(const Json::Value& cfg) {
	Json::Value v = cfg["threadCount"];
	if (v.isNumeric()) {
		threadCount = v.asUInt();
	}
	
	v = cfg["sortConcurrency"];
	if (v.isNumeric()) {
		sortConcurrency = v.asUInt();
	}
	
	v = cfg["payloadConcurrency"];
	if (v.isNumeric()) {
		payloadConcurrency = v.asUInt();
	}
	
	v = cfg["maxMemoryUsage"];
	if (v.isNumeric()) {
		maxMemoryUsage = v.asUInt64()*(static_cast<uint32_t>(1) << 20);
	}
}


std::string Config::help() {
	return "[-a --ask] -i <input.osm.pbf|input dir> -o <output dir> -c <config.json>";
}

std::string Config::toString(Config::ValidationReturnValues v) {
	if (v == VRV_OK) {
		return "OK";
	}
	std::string ret;
#define ERRA(__NAME) if (v & __NAME) { ret += #__NAME; ret += ',';}
	ERRA(VRV_BROKEN)
	ERRA(VRV_BROKEN_TAG_STORE)
	ERRA(VRV_BROKEN_INDEX_STORE)
	ERRA(VRV_BROKEN_TEXT_SEARCH)
	ERRA(VRV_BROKEN_GRID)
	ERRA(VRV_BROKEN_RTREE)
	ERRA(VRV_BROKEN_KV_STORE)
#undef ERRA
	return ret;
}

Config::Config() :
ask(false),
indexStoreConfig(0),
kvStoreConfig(0),
gridConfig(0),
rTreeConfig(0),
tagStoreConfig(0)
{}

Config::~Config() {
	delete indexStoreConfig;
	delete kvStoreConfig;
	for(auto x : textSearchConfig) {
		delete x;
	}
	delete gridConfig;
	delete rTreeConfig;
	delete tagStoreConfig;
}

std::string Config::getOutFileDir() const {
	return m_outFileName;
}

std::string Config::getOutFileName(liboscar::FileConfig fc) const {
	return getOutFileDir() + "/" + liboscar::toString(fc);
}

Config::ValidationReturnValues Config::validate() {
	int tmp = VRV_OK;
	if (tagStoreConfig && tagStoreConfig->enabled && !tagStoreConfig->valid()) {
		tmp |= VRV_BROKEN_TAG_STORE;
	}
	if (kvStoreConfig && kvStoreConfig->enabled && !kvStoreConfig->valid()) {
		tmp |= VRV_BROKEN_KV_STORE;
	}
	if (gridConfig && gridConfig->enabled && !gridConfig->valid()) {
		tmp |= VRV_BROKEN_GRID;
	}
	if (!indexStoreConfig || !indexStoreConfig->valid()) {
		tmp |= VRV_BROKEN_INDEX_STORE;
	}
	if (rTreeConfig && rTreeConfig->enabled && !rTreeConfig->valid()) {
		tmp |= VRV_BROKEN_RTREE;
	}
	for(TextSearchConfig * t : textSearchConfig) {
		if (t && t->enabled && !t->valid()) {
			tmp |= VRV_BROKEN_TEXT_SEARCH;
		}
	}
	return (ValidationReturnValues) tmp;
}


Config::ReturnValues Config::fromCmdLineArgs(int argc, char** argv) {
	std::vector<std::string> configFiles;
	std::vector< std::pair<std::string, Json::Value> > configData;
	
	for(int i=1; i < argc; ++i) {
		std::string token(argv[i]);
		if (token == "-i" && i+1 < argc) {
			inFileNames.emplace_back(std::string(argv[i+1]));
			++i;
		}
		else if (token == "-o" && i+1 < argc) {
			m_outFileName = std::string(argv[i+1]);
			++i;
		}
		else if (token == "-c" && i+1 < argc) {
			configFiles.emplace_back(argv[i+1]);
			++i;
		}
		else if (token == "-a" || token == "--ask") {
			ask = true;
		}
	}
	
	if (!inFileNames.size()) {
		std::cerr << "No input files specified" << std::endl;
		return RV_FAILED;
	}

	///@param path must be absolute
	auto handleConfigFile = sserialize::fix([this, &configData](auto && self, const std::string & path) -> Config::ReturnValues {
		SSERIALIZE_CHEAP_ASSERT(sserialize::MmappedFile::isAbsolute(path));
		
		std::ifstream inFileStream;
		std::string basePath( sserialize::MmappedFile::dirName(path) );
		Json::Value root;
		
		inFileStream.open(path);
		if (!inFileStream.is_open()) {
			std::cerr << "could not open config file " << path << std::endl;
			return RV_FAILED;
		}
		inFileStream >> root;
		
		Json::Value incDv = root["include"];
		if (incDv.isString()) { //single include
			self( toAbsolutePath( incDv.asString(), basePath) );
		}
		else if (incDv.isArray()) {
			for(Json::ArrayIndex j(0), s(incDv.size()); j < s; ++j) {
				if (!incDv[j].isString()) {
					std::cerr << "Invalid config option in file " << path << ": include directive takes a string or an array of strings" << std::endl;
					return RV_FAILED;
				}
				if ( RV_OK != self( toAbsolutePath( incDv[j].asString(), basePath) ) ) {
					return RV_FAILED;
				}
				
			}
		}
		else if (!incDv.isNull()) {
			std::cerr << "Invalid config option in file " << path << ": include directive takes a string or an array of strings" << std::endl;
			return RV_FAILED;
		}
		
		//and append our own data changes to the processing queue
		configData.emplace_back( path, std::move(root) );
		return RV_OK;
	});
	
	for(std::size_t i(0); i < configFiles.size(); ++i) {
		auto rt = handleConfigFile( toAbsolutePath(configFiles.at(i), ".") );
		if (rt != oscar_create::Config::RV_OK) {
			return rt;
		}
	}
	
	Json::Value tmp;
	
	std::unordered_map<std::string, std::size_t> tscId2tsc;
	
	//and process our input data in-order
	for(std::size_t i(0), s(configData.size()); i < s; ++i) {
		Json::Value & root = configData.at(i).second;
		std::string basePath = sserialize::MmappedFile::dirName(configData[i].first);
		try {
			tmp = root["stats"];
			if (!tmp.isNull()) {
				statsConfig.update(tmp);
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
				if (!indexStoreConfig) {
					indexStoreConfig = new IndexStoreConfig(tmp);
				}
				else {
					indexStoreConfig->update(tmp);
				}
			}
			
			tmp = root["store"];
			if (!tmp.isNull()) {
				if (!kvStoreConfig) {
					kvStoreConfig = new KVStoreConfig(tmp, basePath);
				}
				else {
					kvStoreConfig->update(tmp, basePath);
				}
			}
			
			tmp = root["grid"];
			if (!tmp.isNull()) {
				if (!gridConfig) {
					gridConfig = new GridConfig(tmp);
				}
				else {
					gridConfig->update(tmp);
				}
			}
			
			tmp = root["rtree"];
			if (!tmp.isNull()) {
				if (!rTreeConfig) {
					rTreeConfig = new RTreeConfig(tmp);
				}
				else {
					rTreeConfig->update(tmp);
				}
			}
			
			tmp = root["tagstore"];
			if (!tmp.isNull()) {
				if (!tagStoreConfig) {
					tagStoreConfig = new TagStoreConfig(tmp, basePath);
				}
				else {
					tagStoreConfig->update(tmp, basePath);
				}
			}
			
			tmp = root["textsearch"];
			if (!tmp.isNull()) {
				auto handleSingleTsc = [this, &tscId2tsc, &basePath](const Json::Value & x) {
					std::string id = x["type"].asString();
					if (!x["id"].isNull()) {
						id = x["id"].asString();
					}
					if (tscId2tsc.count(id)) {
						TextSearchConfig* & tsc = textSearchConfig.at(tscId2tsc.at(id));
						tsc = TextSearchConfig::parseTyped(x, basePath, tsc);
					}
					else {
						tscId2tsc[id] = textSearchConfig.size();
						textSearchConfig.push_back(TextSearchConfig::parseTyped(x, basePath));
					}
				};
				if (tmp.isArray()) {
					for(const Json::Value & x : tmp) {
						handleSingleTsc(x);
					}
				}
				else if (tmp.isObject()) {
					handleSingleTsc(tmp);
				}
				else {
					std::cerr << "Invalid entry for textsearch in file " << configFiles.at(i) << std::endl;
				}
			}
		}
		catch (const sserialize::ConfigurationException & e) {
			std::cerr << "An error occured while parsing file " << configFiles.at(i) << ": " << e.what() << std::endl;
			return oscar_create::Config::RV_FAILED;
		}
	}
	
	return RV_OK;
}

//other stuff

bool TextSearchConfig::hasEnabled(TextSearchConfig::QueryType qt) const {
	for(uint32_t i(0); i < 2; ++i) {
		for(uint32_t j(0); j < 2; ++j) {
			if (searchCapabilites[i][j][(int)qt].enabled) {
				return true;
			}
		}
	}
	return false;
}


bool TextSearchConfig::hasCaseSensitive() const {
	for(uint32_t i(0); i < 2; ++i) {
		for(uint32_t j(0); j < 2; ++j) {
			for(uint32_t k(0); k < 2; ++k) {
				if (searchCapabilites[i][j][k].caseSensitive) {
					return true;
				}
			}
		}
	}
	return false;
}

bool TextSearchConfig::hasDiacriticInSensitive() const {
	for(uint32_t i(0); i < 2; ++i) {
		for(uint32_t j(0); j < 2; ++j) {
			for(uint32_t k(0); k < 2; ++k) {
				if (searchCapabilites[i][j][k].diacritcInSensitive) {
					return true;
				}
			}
		}
	}
	return false;
}

bool TextSearchConfig::consistentCaseSensitive() const {
	bool hasCaseSensitive = false;
	bool hasCaseInsensitive = false;
	for(uint32_t i(0); i < 2; ++i) {
		for(uint32_t j(0); j < 2; ++j) {
			for(uint32_t k(0); k < 2; ++k) {
				const SearchCapabilities & cap = searchCapabilites[i][j][k];
				if (cap.enabled) {
					if (searchCapabilites[i][j][k].caseSensitive) {
						hasCaseSensitive = true;
					}
					else {
						hasCaseInsensitive = true;
					}
				}
			}
		}
	}
	return hasCaseInsensitive != hasCaseSensitive;
}

}//end namespace oscar_create