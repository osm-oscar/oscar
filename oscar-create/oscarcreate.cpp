#include <iostream>
#include <memory>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/stats/memusage.h>
#include "OsmKeyValueObjectStore.h"
#include "types.h"
#include "helpers.h"
#include "readwritefuncs.h"
#include "AreaExtractor.h"

void printHelp() {
	std::cout << "oscar-create creates all data necessary for oscar-(web|cmd|gui) out of .osm.bf files." << std::endl;
	std::cout << oscar_create::Options::help() << std::endl;
}

int main(int argc, char ** argv) {
	{
	timeval t1;
	gettimeofday(&t1, NULL);
	srand(t1.tv_usec * t1.tv_sec);
	}
	std::cout.precision(10);
	oscar_create::Options opts;
	int ret = opts.fromCmdLineArgs(argc, argv);
	if (ret != oscar_create::Options::RV_OK) {
		printHelp();
		return 1;
	}
	std::cout << "Selected Options:" << std::endl;
	std::cout << opts << std::endl;

	if (!sserialize::MmappedFile::isDirectory( opts.getOutFileDir() ) && ! sserialize::MmappedFile::createDirectory(opts.getOutFileDir())) {
		std::cout << "Failed to create destination directory" << std::endl;
		return -1;
	}
	
	if (opts.createKVStore) {
		sserialize::TimeMeasurer dbTime; dbTime.begin();
		oscar_create::OsmKeyValueObjectStore store;

		oscar_create::OsmKeyValueObjectStore::SaveDirector * itemSaveDirectorPtr = 0;
		if (opts.kvStoreConfig.saveEverything) {
			itemSaveDirectorPtr = new oscar_create::OsmKeyValueObjectStore::SaveAllSaveDirector();
		}
		else if (opts.kvStoreConfig.saveEveryTag) {
			itemSaveDirectorPtr = new oscar_create::OsmKeyValueObjectStore::SaveEveryTagSaveDirector(opts.kvStoreConfig.itemsSavedByKeyFn, opts.kvStoreConfig.itemsSavedByKeyValueFn);
		}
		else if (!opts.kvStoreConfig.itemsIgnoredByKeyFn.empty()) {
			itemSaveDirectorPtr = new oscar_create::OsmKeyValueObjectStore::SaveAllItemsIgnoring(opts.kvStoreConfig.itemsIgnoredByKeyFn);
		}
		else {
			itemSaveDirectorPtr = new oscar_create::OsmKeyValueObjectStore::SingleTagSaveDirector(opts.kvStoreConfig.keyToStoreFn, opts.kvStoreConfig.keyValuesToStoreFn, opts.kvStoreConfig.itemsSavedByKeyFn, opts.kvStoreConfig.itemsSavedByKeyValueFn);
		}
		std::shared_ptr<oscar_create::OsmKeyValueObjectStore::SaveDirector> itemSaveDirector(itemSaveDirectorPtr);

		for (const std::string & inFileName : opts.inFileNames) {
			if (opts.kvStoreConfig.autoMaxMinNodeId) {
				if (!oscar_create::findNodeIdBounds(inFileName, opts.kvStoreConfig.minNodeId, opts.kvStoreConfig.maxNodeId)) {
					std::cout << "Finding min/max node id failed for " << inFileName << "! skipping input file..." << std::endl;
					continue;
				}
				std::cout << "min=" << opts.kvStoreConfig.minNodeId << ", max=" << opts.kvStoreConfig.maxNodeId << std::endl;
			}
			
			if (opts.memUsage) {
				sserialize::MemUsage().print("Polygonstore", "Poylgonstore");
			}
			
			oscar_create::OsmKeyValueObjectStore::CreationConfig cc;
			cc.fileName = inFileName;
			cc.itemSaveDirector = itemSaveDirector;
			cc.maxNodeCoordTableSize = opts.kvStoreConfig.maxNodeHashTableSize;
			cc.maxNodeId = opts.kvStoreConfig.maxNodeId;
			cc.minNodeId = opts.kvStoreConfig.minNodeId;
			cc.numThreads = opts.kvStoreConfig.numThreads;
			cc.rc.regionFilter = oscar_create::AreaExtractor::nameFilter(opts.kvStoreConfig.keysDefiningRegions, opts.kvStoreConfig.keyValuesDefiningRegions);
			cc.rc.polyStoreLatCount = opts.kvStoreConfig.polyStoreLatCount;
			cc.rc.polyStoreLonCount = opts.kvStoreConfig.polyStoreLonCount;
			cc.rc.polyStoreMaxTriangPerCell = opts.kvStoreConfig.polyStoreMaxTriangPerCell;
			cc.rc.triangMaxCentroidDist = opts.kvStoreConfig.triangMaxCentroidDist;
			cc.sortOrder = (oscar_create::OsmKeyValueObjectStore::ItemSortOrder) opts.kvStoreConfig.itemSortOrder;
			cc.prioStringsFn = opts.kvStoreConfig.prioStringsFileName;
			cc.scoreConfigFileName = opts.kvStoreConfig.scoreConfigFileName;
			
			if (!opts.kvStoreConfig.keysValuesToInflate.empty()) {
				oscar_create::readKeysFromFile(opts.kvStoreConfig.keysValuesToInflate, std::inserter(cc.inflateValues, cc.inflateValues.end()));
			}

			bool parseOK = store.addFromPBF(cc);
			if (!parseOK) {
				std::cout << "Failed to parse: " << inFileName << std::endl; 
				return 1;
			}
			else {
				std::cout << "Read data" << std::endl;
			}
		}

		dbTime.end();
		store.stats(std::cout);
		
		oscar_create::mangleAndWriteKv(store, opts);
		std::cout << "Time to create KeyValueStore: " << dbTime << std::endl;
	}
	else {
		std::cout << "Trying to create index file...";
		opts.indexFile = sserialize::UByteArrayAdapter::createFile(1024*1024, opts.getOutFileName(liboscar::FC_INDEX));
		if (opts.indexFile.size() < 1024*1024) {
			std::cout << "Failed to create index file" << std::endl;
			return -1;
		}
		else {
			std::cout << "OK" << std::endl;
		}

		sserialize::TimeMeasurer totalTime; totalTime.begin();
		liboscar::Static::OsmKeyValueObjectStore staticStore;
		
		try {
			staticStore = liboscar::Static::OsmKeyValueObjectStore(sserialize::UByteArrayAdapter::openRo(opts.inFileNames.front(), false));
		}
		catch (sserialize::Exception & e) {
			std::cerr << "Failed to open store file at " << opts.inFileNames.front() << std::endl;
			return -1;
		}
		oscar_create::handleKv(staticStore, opts);
		totalTime.end();
		sserialize::MmappedFile::createSymlink( sserialize::MmappedFile::realPath(opts.inFileNames.front()), opts.getOutFileDir() + "/kvstore");
		std::cout << "Time to handle KeyValueStore: " << totalTime << std::endl;
	}
	
	sserialize::MemUsage().print();
	
	return 0;
}
