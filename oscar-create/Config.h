#ifndef OSCAR_CREATE_CONFIG_H
#define OSCAR_CREATE_CONFIG_H
#include <sserialize/storage/MmappedMemory.h>
#include <sserialize/containers/GeneralizedTrie.h>
#include <liboscar/TextSearch.h>
#include <liboscar/constants.h>
#include <unordered_map>
#include <list>
#include <ostream>

namespace oscar_create {

class Options {
public:
	enum ReturnValues { RV_OK, RV_FAILED, RV_HELP};
	struct TextSearchConfig {
		enum class Type { TRIE, FULL_INDEX_TRIE, FLAT_GST, FLAT_TRIE};
		std::string keyFile;
		std::string storeTagsPrefixFile;
		std::string storeTagsSuffixFile;
		std::string storeRegionTagsFile;
		bool caseSensitive;
		bool diacritcInSensitive;
		bool suffixes;
		bool aggressiveMem;
		sserialize::MmappedMemoryType mmType;
		bool mergeIndex;
		bool extensiveChecking;

		std::unordered_set<uint32_t> suffixDelimeters;
		std::set<uint8_t> levelsWithoutIndex;
		sserialize::Static::TrieNode::Types nodeType;
		int maxPrefixIndexMergeCount;
		int maxSuffixIndexMergeCount;
		Type type;
		uint32_t threadCount;
		TextSearchConfig() :
		caseSensitive(false), diacritcInSensitive(false), suffixes(false),
		aggressiveMem(false), mmType(sserialize::MM_FILEBASED), mergeIndex(false), extensiveChecking(false),
		nodeType(sserialize::Static::TrieNode::T_LARGE_COMPACT),
		maxPrefixIndexMergeCount(-1), maxSuffixIndexMergeCount(-1),
		type(Type::TRIE), threadCount(1)
		{}
		/**config text as key=value pairs, or just keys seperated by , and escaped by \ 
		  *
		  * caseSensitive = c, diacritcInSensitive = d, suffixes = s, aggressiveMem = a
		  * inMemoryIndex = imi, mergeIndex = mi, extensiveChecking = ec
		  * suffixDelimeters = sd=<string>, levelsWithoutIndex = lwi=int
		  * nodeType = n=(simple|compact|large)
		  * maxPrefixIndexMergeCount = mpm=int, maxSuffixIndexMergeCount msm=int
		  * type = t=(trie|fitrie|fgst),
		  * keyFile = kf=/path/to/keyfile
		*/
		TextSearchConfig(const std::string & str);
	};
	struct TagStoreConfig {
		TagStoreConfig() : create(false) {}
		TagStoreConfig(const std::string & str);
		bool create;
		std::string tagKeys;
		std::string tagKeyValues;
	};
	/** Config has the following syntax:
	  *
	  * inc=<num>: incrementalKvCreation
	  * pa: add boundary info to items
	  * p=latXlonXlatrfXlonrfXminDiag: polygonstore options
	  * hs=(min:max|auto): nodeid hash table options
	  * kvi=file: keys values to inflate
	  * sk=file: keys to save
	  * skv=file: key values to save
	  * sik=file: keys selecting items to save
	  * sikv=file: key:values selecting items to save
	  * kdr=file: keys defining regions
	  * kvdr=file: key:values defining regions
	  * se: save everything
	  * st: save every tag
	  * 
	  */
	struct KVStoreConfig {
		KVStoreConfig();
		KVStoreConfig(const std::string & str);
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
		uint32_t polyStoreLatCount;
		uint32_t polyStoreLonCount;
		uint32_t polyStoreMaxTriangPerCell;
		double triangMaxCentroidDist;
		uint32_t numThreads;
		int itemSortOrder;//as defined by OsmKeyValueObjectStore::ItemSortOrder
		std::string prioStringsFileName;
	};
// 	enum TextSearchTypes { TS_ITEMS, TS_GEOHIERARCHY};
// 	typedef liboscar::TextSearch::Type TextSearchTypes;
private:
	std::string m_outFileName;
	bool m_appendConfigToOutFileName;
public:
	Options();
	~Options() {}
	ReturnValues fromCmdLineArgs(int argc, char** argv);
	static std::string help();
	sserialize::GeneralizedTrie::GeneralizedTrieCreatorConfig toTrieConfig(const oscar_create::Options::TextSearchConfig & cfg);
	std::string getOutFileDir() const;
	///out file name with full path
	std::string getOutFileName(liboscar::FileConfig fc) const;
	static std::string toString(sserialize::Static::TrieNode::Types nodeType);

	//Variables
	std::list<std::string> inFileNames;

	//Initial processing
	bool createKVStore;

	//index options
	std::string inputIndexStore;
	sserialize::ItemIndex::Types indexType;
	bool checkIndex;
	bool indexDedup;

	//Grid info
	uint32_t gridLatCount;
	uint32_t gridLonCount;
	
	//Grid-RTree info
	uint32_t gridRTreeLatCount;
	uint32_t gridRTreeLonCount;
	
	//text search
	std::vector< std::pair<liboscar::TextSearch::Type, TextSearchConfig> > textSearchConfig;
	TagStoreConfig tagStoreConfig;
	KVStoreConfig kvStoreConfig;
	
	//debug info
	bool memUsage;
	
	//serialization structures
	sserialize::UByteArrayAdapter indexFile;
};


std::ostream& operator<<(std::ostream & out, const Options & opts);

}//end namespace oscar_create

#endif