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
	std::ostream & operator<<(std::ostream & out) const;
};

struct IndexStoreConfig {
	IndexStoreConfig(const Json::Value & cfg);
	std::string inputStore;
	sserialize::ItemIndex::Types type;
	bool check;
	bool deduplicate;
	std::ostream & operator<<(std::ostream & out) const;
};

struct GridConfig {
	GridConfig(const Json::Value & cfg);
	uint32_t latCount;
	uint32_t lonCount;
	std::ostream & operator<<(std::ostream & out) const;
};

struct RTreeConfig {
	RTreeConfig(const Json::Value & cfg);
	uint32_t latCount;
	uint32_t lonCount;
	std::ostream & operator<<(std::ostream & out) const;
};

struct TagStoreConfig {
	TagStoreConfig(const Json::Value & cfg);
	bool enabled;
	std::string tagKeys;
	std::string tagKeyValues;
	std::ostream & operator<<(std::ostream & out) const;
};

struct KVStoreConfig {
	KVStoreConfig(const Json::Value & cfg);
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
	uint32_t polyStoreLatCount;
	uint32_t polyStoreLonCount;
	uint32_t polyStoreMaxTriangPerCell;
	double triangMaxCentroidDist;
	uint32_t numThreads;
	int itemSortOrder;//as defined by OsmKeyValueObjectStore::ItemSortOrder
	std::string prioStringsFileName;
	std::ostream & operator<<(std::ostream & out) const;
};

class TextSearchConfig {
public:
	enum class ItemType : int { ITEM=0, REGION=1 };
	enum class QueryType : int { PREFIX=0, SUBSTRING=1 };
	struct SearchCapabilities {
		bool enabled;
		bool caseSensitive;
		bool diacritcInSensitive;
		std::string tagFile;
		std::string valueFile;
		SingleConfig() : enabled(false), caseSensitive(false), diacritcInSensitive(false) {}
		std::ostream & operator<<(std::ostream & out) const;
	};
public:
	TextSearchConfig() : enabled(false), type(liboscar::TextSearch::NONE) {}
	TextSearchConfig(const Json::Value & v);
	std::ostream & operator<<(std::ostream & out) const;
	virtual void print(std::ostream & out) const = 0;
	static TextSearchConfig * parseTyped(const Json::Value& cfg);
public:
	bool enabled;
	liboscar::TextSearch::Type type;
	//[ItemType][QueryType] -> SearchCapabilites
	SearchCapabilities searchCapabilites[2][2];
};

class ItemSearchConfig: public TextSearchConfig {
public:
	enum class TrieType { TRIE, FULL_INDEX_TRIE, FLAT_GST, FLAT_TRIE};
public:
	ItemSearchConfig(const Json::Value & cfg);
	virtual void print(std::ostream & out) const override;
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

	bool extensiveChecking;
	uint32_t threadCount;
};

class GeoHierarchySearchConfig: public ItemSearchConfig {
public:
	GeoHierarchySearchConfig(const Json::Value & cfg);
	virtual void print(std::ostream & out) const override;
};

class GeoCellConfig: public TextSearchConfig {
public:
	GeoCellConfig(const Json::Value & cfg);
	virtual void print(std::ostream & out) const override;
public:
	uint32_t threadCount;
	std::unordered_set<uint32_t> suffixDelimeters;
};

class OOMGeoCellConfig: public TextSearchConfig {
public:
	GeoCellConfig(const Json::Value & cfg);
	virtual void print(std::ostream & out) const override;
public:
	uint32_t threadCount;
};

class Config {
public:
	enum ReturnValues { RV_OK, RV_FAILED, RV_HELP};
private:
	std::string m_outFileName;
	bool m_appendConfigToOutFileName;
public:
	Config();
	~Config() {}
	ReturnValues fromCmdLineArgs(int argc, char** argv);
	sserialize::GeneralizedTrie::GeneralizedTrieCreatorConfig toTrieConfig(const TextSearchConfig & cfg);
	std::string getOutFileDir() const;
	///out file name with full path
	std::string getOutFileName(liboscar::FileConfig fc) const;
	sserialize::UByteArrayAdapter ubaFromFC(liboscar::FileConfig fc) const;

	static std::string help();
	static std::string toString(sserialize::Static::TrieNode::Types nodeType);

	std::ostream & operator<<(std::ostream & out) const;
	
	//Variables
	std::list<std::string> inFileNames;

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
	
	//serialization structures
	sserialize::UByteArrayAdapter indexFile;
};


std::ostream& operator<<(std::ostream & out, const Config & opts);

}//end namespace oscar_create

#endif