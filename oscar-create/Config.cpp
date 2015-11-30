#include "Config.h"
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/utility/printers.h>
#include <sserialize/strings/stringfunctions.h>
#include <liboscar/constants.h>
#include "OsmKeyValueObjectStore.h"
#include <json/json.h>

namespace oscar_create {

//Print functions----------------

inline std::string toString(bool v) {
	return (v ? "yes" : "no");
}

inline std::string toString(sserialize::MmappedMemoryType mmt) {
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

inline std::string toString(sserialize::ItemIndex::Types v) {
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

inline sserialize::MmappedMemoryType parseMMT(const std::string str) {
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

std::ostream& StatsConfig::operator<<(std::ostream& out) const {
	out << "print memusage: " << toString(memUsage);
	return out;
}

std::ostream& IndexStoreConfig::operator<<(std::ostream& out) const {
	out << "input store: " << inputStore << "\n";
	out << "type: " << toString(type) << "\n";
	out << "check: " << toString(check) << "\n";
	out << "deduplicate: " << toString(deduplicate);
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
	out << "tagFile: " << keysFn << "\n";
	out << "valueFile: " << keyValuesFn;
	return out;
}

std::ostream& TextSearchConfig::operator<<(std::ostream& out) const {
	std::array<std::string, 2> itemTypeNames{{"items", "regions"}};
	std::array<std::string, 2> tagTypeNames{{"value's keys", "key-values"}};
	std::array<std::string, 2> queryTypeNames{{"prefix", "suffix"}};
	
	for(uint32_t itemType(0); itemType < 2; ++itemType) {
		for(uint32_t tagType(0); tagType < 2; ++tagType) {
			for(uint32_t queryType(0); queryType < 2; ++queryType) {
				if (searchCapabilites[itemType][tagType][queryType].enabled) {
					out << itemTypeNames[itemType] << "::" << tagTypeNames[tagType] << "::" << queryTypeNames[queryType] << ":\n";
					searchCapabilites[itemType][tagType][queryType].operator<<(out) << "\n";
				}
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
}

void GeoHierarchySearchConfig::print(std::ostream& out) const {
	ItemSearchConfig::print(out);
}

void GeoCellConfig::print(std::ostream& out) const {
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
}

void OOMGeoCellConfig::print(std::ostream& out) const {
	out << "Thread count: " << threadCount;
}

std::ostream& Config::operator<<(std::ostream& out) const {
	statsConfig.operator<<(out);
	if (indexStoreConfig) {
		indexStoreConfig->operator<<(out);
	}
	if (kvStoreConfig) {
		out << "KVStoreConfig:\n"; 
		kvStoreConfig->operator<<(out);
	}
	if (gridConfig) {
		out << "GridConfig:\n";
		gridConfig->operator<<(out);
	}
	if (rTreeConfig) {
		out << "RTreeConfig:\n";
		rTreeConfig->operator<<(out);
	}
	if (tagStoreConfig) {
		out << "TagStoreConfig:\n";
		tagStoreConfig->operator<<(out);
	}
	for(TextSearchConfig * x : textSearchConfig) {
		if (!x) {
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
		case liboscar::TextSearch::GEOHIERARCHY:
			out << "geohierarchy";
			break;
		default:
			out << "invalid";
			break;
		}
		out << "]:\n";
		x->operator<<(out);
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
				if (cap.enabled & (!sserialize::MmappedFile::fileExists(cap.keysFn) || !sserialize::MmappedFile::fileExists(cap.keyValuesFn))) {
					return false;
				}
			}
		}
	}
	return false;
}

bool ItemSearchConfig::valid() const {
	return mmType != sserialize::MM_INVALID &&
		nodeType != sserialize::Static::TrieNode::T_EMPTY;
}

bool GeoCellConfig::valid() const {
	return mmType != sserialize::MM_INVALID;
}

//parse functions
StatsConfig::StatsConfig(const Json::Value & cfg) : StatsConfig() {
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
		tmp = v["keyValues"];
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
		tmp = v["keyValues"];
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
		tmp = v["keyValues"];
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

TextSearchConfig::TextSearchConfig(const Json::Value& cfg) {
	Json::Value v = cfg["enabled"];
	if (v.isBool()) {
		enabled = v.asBool();
	}
	
	v = cfg["items"];
	if (v.isObject()) {
		parseTagTypeObject(v, ItemType::ITEM);
	}
	
	v = cfg["regions"];
	if (v.isObject()) {
		parseTagTypeObject(v, ItemType::REGION);
	}
}

void TextSearchConfig::parseTagTypeObject(const Json::Value& cfg, TextSearchConfig::ItemType itemType) {
	Json::Value v = cfg["values"];
	if (v.isObject()) {
		parseQueryTypeObject(v, itemType, TagType::VALUES);
	}
	
	v = cfg["tags"];
	if (v.isObject()) {
		parseQueryTypeObject(v, itemType, TagType::TAGS);
	}
}

void TextSearchConfig::parseQueryTypeObject(const Json::Value& cfg, TextSearchConfig::ItemType itemType, TextSearchConfig::TagType tagType) {
	Json::Value v = cfg["prefix"];
	if (v.isObject()) {
		parseKvConfig(v, itemType, tagType, QueryType::PREFIX);
	}
	
	v = cfg["substring"];
	if (v.isObject()) {
		parseKvConfig(v, itemType, tagType, QueryType::SUBSTRING);
	}
}

void TextSearchConfig::parseKvConfig(const Json::Value& cfg, TextSearchConfig::ItemType itemType, TextSearchConfig::TagType tagType, TextSearchConfig::QueryType qt) {
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
	
	v = cfg["keys"];
	if (v.isString()) {
		cap.keysFn = v.asString();
	}
	
	v = cfg["keyValues"];
	if (v.isString()) {
		cap.keyValuesFn = v.asString();
	}
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
	return 0;
}

ItemSearchConfig::ItemSearchConfig(const Json::Value& cfg) :
TextSearchConfig(cfg)
{
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
	
	v = cfg["aggressivMemory"];
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

GeoHierarchySearchConfig::GeoHierarchySearchConfig(const Json::Value& cfg) :
ItemSearchConfig(cfg)
{}

GeoCellConfig::GeoCellConfig(const Json::Value & cfg) :
TextSearchConfig(cfg)
{
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

OOMGeoCellConfig::OOMGeoCellConfig(const Json::Value& cfg) {
	Json::Value v = cfg["threadCount"];
	if (v.isNumeric()) {
		threadCount = v.asUInt();
	}
}

std::string Config::help() {
	return std::string("-i <input.osm.pbf|input dir> -o <output dir> -c <config.json>");
}

Config::Config() :
indexStoreConfig(0),
kvStoreConfig(0),
gridConfig(0),
rTreeConfig(0),
tagStoreConfig(0)
{}

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
		indexStoreConfig = new IndexStoreConfig(tmp);
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
				textSearchConfig.push_back(TextSearchConfig::parseTyped(x));
			}
		}
		else if (tmp.isObject()) {
			textSearchConfig.push_back(TextSearchConfig::parseTyped(tmp));
		}
		else {
			std::cerr << "Invalid entry for textsearch" << std::endl;
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


}//end namespace oscar_create