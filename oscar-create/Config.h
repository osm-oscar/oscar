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
	void update(const Json::Value & cfg);
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
};

struct IndexStoreConfig {
	IndexStoreConfig(const Json::Value & cfg);
	std::string inputStore;
	sserialize::ItemIndex::Types type;
	bool check;
	bool deduplicate;
	void update(const Json::Value & cfg);
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
};

struct GridConfig {
	GridConfig(const Json::Value & cfg);
	bool enabled;
	uint32_t latCount;
	uint32_t lonCount;
	void update(const Json::Value & cfg);
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
};

struct RTreeConfig {
	RTreeConfig(const Json::Value & cfg);
	bool enabled;
	uint32_t latCount;
	uint32_t lonCount;
	void update(const Json::Value & cfg);
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
};

struct TagStoreConfig {
	TagStoreConfig(const Json::Value & cfg, const std::string & basePath);
	bool enabled;
	std::string tagKeys;
	std::string tagKeyValues;
	void update(const Json::Value & cfg, const std::string & basePath);
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
};

struct TriangleRefinementConfig {
	typedef enum { T_NONE, T_CONFORMING, T_GABRIEL, T_MAX_CENTROID_DISTANCE, T_LIPSCHITZ, T_MAX_EDGE_LENGTH_RATIO, T_MAX_EDGE_LENGTH} Type;
	TriangleRefinementConfig();
	TriangleRefinementConfig(const Json::Value & cfg, const std::string & basePath);
	void update(const Json::Value & cfg, const std::string & basePath);
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
	bool simplify = false;
	Type type = T_NONE;
	double maxCentroidDistance = std::numeric_limits<double>::max();
	double maxCentroidDistanceRatio = std::numeric_limits<double>::max();
	double maxEdgeLengthRatio = std::numeric_limits<double>::max();
	double maxEdgeLength = std::numeric_limits<double>::max();
};
struct CellRefinementConfig {
	typedef enum {T_NONE, T_CONNECTED, T_TRIANGLE_COUNT, T_CELL_DIAG} Type;
	CellRefinementConfig();
	CellRefinementConfig(const Json::Value & cfg, const std::string & basePath);
	void update(const Json::Value & cfg, const std::string & basePath);
	std::ostream & print(std::ostream & out) const;
	bool valid() const;
	Type type;
	double maxCellDiag;
	uint32_t maxTriangPerCell;
};

struct KVStoreConfig {
	KVStoreConfig(const Json::Value & cfg, const std::string & basePath);
	bool enabled;
	uint64_t maxNodeHashTableSize; 
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
	bool addRegionsToCells;
	uint32_t latCount;
	uint32_t lonCount;
	uint32_t grtLatCount;
	uint32_t grtLonCount;
	double grtMinDiag;
	TriangleRefinementConfig triangRefineCfg;
	CellRefinementConfig cellRefineCfg;
	uint32_t numThreads;
	uint32_t blobFetchCount;
	int itemSortOrder;//as defined by OsmKeyValueObjectStore::ItemSortOrder
	std::string prioStringsFileName;
	sserialize::Static::spatial::Triangulation::GeometryCleanType gct;
	void update(const Json::Value & cfg, const std::string & basePath);
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
	TextSearchConfig(const Json::Value& cfg, const std::string & basePath);
	virtual ~TextSearchConfig() {}
	virtual void update(const Json::Value & cf, const std::string & basePath);
	virtual std::ostream & print(std::ostream & out) const;
	///@param base update base with new info
	static TextSearchConfig * parseTyped(const Json::Value& cfg, const std::string & basePath, TextSearchConfig * base = 0);
	virtual bool valid() const;
	bool hasEnabled(QueryType qt) const;
	bool hasCaseSensitive() const;
	bool hasDiacriticInSensitive() const;
	bool consistentCaseSensitive() const;
private:
	void parseTagTypeObject(const Json::Value & cfg, const std::string & basePath, ItemType itemType);
	void parseQueryTypeObject(const Json::Value & cfg, const std::string & basePath, ItemType itemType, TagType tagType);
	void parseKvConfig(const Json::Value & cfg, const std::string & basePath, ItemType itemType, TagType tagType, QueryType qt);
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
	ItemSearchConfig(const Json::Value& cfg, const std::string& basePath);
	virtual ~ItemSearchConfig() {}
	virtual void update(const Json::Value & cfg, const std::string & basePath) override;
	virtual std::ostream & print(std::ostream& out) const override;
	virtual bool valid() const override;
private:
	void updateSelf(const Json::Value & cfg);
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
	GeoHierarchyItemsSearchConfig(const Json::Value& cfg, const std::string& basePath);
	virtual ~GeoHierarchyItemsSearchConfig() {}
	virtual std::ostream & print(std::ostream & out) const override;
	virtual bool valid() const override;
};

class GeoHierarchySearchConfig: public ItemSearchConfig {
public:
	GeoHierarchySearchConfig(const Json::Value & cfg, const std::string & basePath);
	virtual ~GeoHierarchySearchConfig() {}
	virtual std::ostream & print(std::ostream & out) const override;
	virtual bool valid() const override;
};

class GeoCellConfig: public TextSearchConfig {
public:
	enum class TrieType { TRIE, FLAT_TRIE };
public:
	GeoCellConfig(const Json::Value& cfg, const std::string& basePath);
	virtual ~GeoCellConfig() {}
	virtual void update(const Json::Value & cfg, const std::string & basePath) override;
	virtual std::ostream & print(std::ostream & out) const override;
	virtual bool valid() const override;
private:
	void updateSelf(const Json::Value & cfg);
public:
	uint32_t threadCount;
	std::unordered_set<uint32_t> suffixDelimeters;
	TrieType trieType;
	sserialize::MmappedMemoryType mmType;
	bool check;
};

class OOMGeoCellConfig: public TextSearchConfig {
public:
	OOMGeoCellConfig(const Json::Value & cfg, const std::string & basePath);
	virtual ~OOMGeoCellConfig() {}
	virtual void update(const Json::Value & cfg, const std::string & basePath) override;
	virtual std::ostream & print(std::ostream & out) const override;
	virtual bool valid() const override;
private:
	void updateSelf(const Json::Value & cfg);
public:
	uint32_t threadCount;
	uint32_t sortConcurrency;
	uint32_t payloadConcurrency;
	sserialize::OffsetType maxMemoryUsage;
	bool cellLocalIds;
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
	~Config();
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
	std::vector<std::string> inFileNames;
	
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
