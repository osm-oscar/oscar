#ifndef OSCAR_CREATE_CONFIG_H
#define OSCAR_CREATE_CONFIG_H
#include <sserialize/storage/MmappedMemory.h>
#include <sserialize/containers/GeneralizedTrie.h>
#include <liboscar/TextSearch.h>
#include <liboscar/constants.h>
#include <json/json.h>
#include <unordered_map>
#include <list>
#include <ostream>

namespace oscar_create {

struct StatsConfig {
	StatsConfig() : memUsage(false) {}
	StatsConfig(const Json::Value & cfg);
	bool memUsage;
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
};

struct IndexStoreConfig {
	IndexStoreConfig(const Json::Value & cfg);
	std::string inputStore;
	sserialize::ItemIndex::Types type;
	bool check;
	bool deduplicate;
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
};

struct GridConfig {
	GridConfig(const Json::Value & cfg);
	bool enabled;
	uint32_t latCount;
	uint32_t lonCount;
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
};

struct RTreeConfig {
	RTreeConfig(const Json::Value & cfg);
	bool enabled;
	uint32_t latCount;
	uint32_t lonCount;
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
};

struct TagStoreConfig {
	TagStoreConfig(const Json::Value & cfg);
	bool enabled;
	std::string tagKeys;
	std::string tagKeyValues;
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
};

struct KVStoreConfig {
	KVStoreConfig(const Json::Value & cfg);
	bool enabled;
	uint32_t maxNodeHashTableSize; 
	std::string keyToStoreFn;
	std::string keyValuesToStoreFn;
	std::string itemsSavedByKeyFn;
	std::string itemsSavedByKeyValueFn;
	std::string keysValuesToInflate;
	std::string itemsIgnoredByKeyFn;
	std::string keysDefiningRegions;
	std::string keyValuesDefiningRegions;
	std::string scoreConfigFileName;
	bool saveEverything;
	bool saveEveryTag;
	uint64_t minNodeId;
	uint64_t maxNodeId;
	bool autoMaxMinNodeId;
	bool readBoundaries;
	bool fullRegionIndex;
	bool addParentInfo;
	uint32_t latCount;
	uint32_t lonCount;
	uint32_t maxTriangPerCell;
	double maxTriangCentroidDist;
	uint32_t numThreads;
	uint32_t blobFetchCount;
	int itemSortOrder;//as defined by OsmKeyValueObjectStore::ItemSortOrder
	std::string prioStringsFileName;
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
};

class TextSearchConfig {
public:
	enum class ItemType : int { ITEM=0, REGION=1, ITEM_TYPE_COUNT=2};
	enum class QueryType : int { PREFIX=0, SUBSTRING=1, QUERY_TYPE_COUNT=2};
	enum class TagType : int { VALUES=0, TAGS=1, TAG_TYPE_COUNT=2};
	struct SearchCapabilities {
		bool enabled;
		bool caseSensitive;
		bool diacritcInSensitive;
		std::string fileName;
		SearchCapabilities() : enabled(false), caseSensitive(false), diacritcInSensitive(false) {}
		std::ostream & print(std::ostream & out) const;
	};
public:
	TextSearchConfig() : enabled(false), type(liboscar::TextSearch::NONE) {}
	TextSearchConfig(const Json::Value& cfg);
	virtual std::ostream & print(std::ostream & out) const;
	static TextSearchConfig * parseTyped(const Json::Value& cfg);
	virtual bool valid() const;
	bool hasEnabled(QueryType qt) const;
	bool hasCaseSensitive() const;
	bool hasDiacriticInSensitive() const;
private:
	void parseTagTypeObject(const Json::Value & cfg, ItemType itemType);
	void parseQueryTypeObject(const Json::Value & cfg, ItemType itemType, TagType tagType);
	void parseKvConfig(const Json::Value & cfg, ItemType itemType, TagType tagType, QueryType qt);
public:
	bool enabled;
	liboscar::TextSearch::Type type;
	//[ItemType][TagType][QueryType] -> SearchCapabilites
	SearchCapabilities searchCapabilites[2][2][2];
};

class ItemSearchConfig: public TextSearchConfig {
public:
	enum class TrieType { TRIE, FULL_INDEX_TRIE, FLAT_GST, FLAT_TRIE};
public:
	ItemSearchConfig(const Json::Value & cfg);
	std::ostream & print(std::ostream& out) const override;
	virtual bool valid() const override;
public:
	std::unordered_set<uint32_t> suffixDelimeters;

	bool mergeIndex;

	bool aggressiveMem;
	sserialize::MmappedMemoryType mmType;

	//Trie options
	TrieType trieType;
	sserialize::Static::TrieNode::Types nodeType;
	
	std::set<uint8_t> levelsWithoutIndex;
	int maxPrefixIndexMergeCount;
	int maxSuffixIndexMergeCount;

	bool check;
	uint32_t threadCount;
};

class GeoHierarchyItemsSearchConfig: public ItemSearchConfig {
public:
	GeoHierarchyItemsSearchConfig(const Json::Value & cfg);
	virtual std::ostream & print(std::ostream & out) const override;
	virtual bool valid() const override;
};

class GeoHierarchySearchConfig: public ItemSearchConfig {
public:
	GeoHierarchySearchConfig(const Json::Value & cfg);
	virtual std::ostream & print(std::ostream & out) const override;
	virtual bool valid() const override;
};

class GeoCellConfig: public TextSearchConfig {
public:
	enum class TrieType { TRIE, FLAT_TRIE };
public:
	GeoCellConfig(const Json::Value & cfg);
	virtual std::ostream & print(std::ostream & out) const override;
	virtual bool valid() const override;
public:
	uint32_t threadCount;
	std::unordered_set<uint32_t> suffixDelimeters;
	TrieType trieType;
	sserialize::MmappedMemoryType mmType;
	bool check;
};

class OOMGeoCellConfig: public TextSearchConfig {
public:
	OOMGeoCellConfig(const Json::Value & cfg);
	virtual std::ostream & print(std::ostream & out) const override;
	virtual bool valid() const override;
public:
	uint32_t threadCount;
	sserialize::OffsetType maxMemoryUsage;
};

class Config {
public:
	enum ReturnValues { RV_OK, RV_FAILED, RV_HELP};
	enum ValidationReturnValues {
		VRV_OK = 0,
		VRV_BROKEN=0x1, VRV_BROKEN_TAG_STORE=0x2, VRV_BROKEN_INDEX_STORE=0x4, VRV_BROKEN_TEXT_SEARCH=0x8,
		VRV_BROKEN_GRID=0x10, VRV_BROKEN_RTREE=0x20, VRV_BROKEN_KV_STORE=0x80};
private:
	std::string m_outFileName;
public:
	Config();
	~Config() {}
	ReturnValues fromCmdLineArgs(int argc, char** argv);
	ValidationReturnValues validate();
	std::string getOutFileDir() const;
	///out file name with full path
	std::string getOutFileName(liboscar::FileConfig fc) const;
// 	sserialize::UByteArrayAdapter ubaFromFC(liboscar::FileConfig fc) const;

	std::ostream & print(std::ostream & out) const;
	
	static std::string help();

	static std::string toString(ValidationReturnValues v);
	
public:
	//Variables
	std::string inFileName;
	
	//interaction
	bool ask;

	IndexStoreConfig * indexStoreConfig;
	
	//store creation
	KVStoreConfig * kvStoreConfig;

	//text search
	std::vector<TextSearchConfig*> textSearchConfig;
	
	//spatial search
	GridConfig * gridConfig;
	RTreeConfig * rTreeConfig;
	
	//special search
	TagStoreConfig * tagStoreConfig;
	
	//stats
	StatsConfig statsConfig;
};

#define SOBP(__TYPE) inline std::ostream & operator<<(std::ostream & out, const __TYPE & src) { return src.print(out); }

SOBP(Config)
SOBP(IndexStoreConfig)
SOBP(KVStoreConfig)
SOBP(TextSearchConfig)
SOBP(TextSearchConfig::SearchCapabilities)
SOBP(ItemSearchConfig)
SOBP(GeoCellConfig)
SOBP(GeoHierarchySearchConfig)
SOBP(OOMGeoCellConfig)
SOBP(GridConfig)
SOBP(RTreeConfig)
SOBP(TagStoreConfig)
SOBP(StatsConfig)

#undef SOBP

}//end namespace oscar_create

#endif