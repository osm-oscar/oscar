#include <iostream>
#include <memory>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/stats/memusage.h>
#include <sserialize/utility/printers.h>
#include "OsmKeyValueObjectStore.h"
#include "helpers.h"
#include "readwritefuncs.h"
#include "AreaExtractor.h"
#include "State.h"

void printHelp() {
	std::cout << "oscar-create creates all data necessary for oscar-(web|cmd|gui) out of .osm.bf files." << std::endl;
	std::cout << oscar_create::Config::help() << std::endl;
}

int main(int argc, char ** argv) {
	sserialize::TimeMeasurer totalTime, kvTime, searchTime;
	totalTime.begin();

	{ //init rand
		timeval t1;
		gettimeofday(&t1, NULL);
		srand((unsigned int) (t1.tv_usec * t1.tv_sec));
	}
	std::cout.precision(10);
	oscar_create::Config opts;
	oscar_create::Data data;
	oscar_create::State state(data);
	bool writeSymlink = false;
	
	int ret = opts.fromCmdLineArgs(argc, argv);
	if (ret != oscar_create::Config::RV_OK) {
		printHelp();
		return -1;
	}
	if (opts.validate() != oscar_create::Config::VRV_OK) {
		auto errCode = opts.validate();
		std::cout << "Config is invalid with errorcode: " << errCode << std::endl;
		std::cout << "Error description: " << oscar_create::Config::toString(errCode) << std::endl;
		return -1;
	}
	std::cout << "Selected Options:" << std::endl;
	std::cout << opts << std::endl;
	if (opts.ask) {
		std::string tmp;
		std::cout << "Everything ok? (yes/NO)" << std::endl;
		std::cin >> tmp;
		if (!tmp.size() || !(tmp.at(0) == 'y' || tmp.at(0) == 'Y')) {
			std::cout << "Exiting" << std::endl;
			return 0;
		}
	}

	if (!sserialize::MmappedFile::isDirectory( opts.getOutFileDir() ) && ! sserialize::MmappedFile::createDirectory(opts.getOutFileDir())) {
		std::cout << "Failed to create destination directory" << std::endl;
		return -1;
	}
	
	state.indexFile = sserialize::UByteArrayAdapter::createFile(1024*1024, opts.getOutFileName(liboscar::FC_INDEX));
	if (state.indexFile.size() < 1024*1024) {
		std::cerr << "Failed to create index file" << std::endl;
		return -1;
	}
	#ifdef WITH_OSCAR_CREATE_NO_DATA_REFCOUNTING
	state.indexFile.disableRefCounting();
	#endif
	
	//init the index factory
	{
		state.indexFactory.setCheckIndex(opts.indexStoreConfig->check);
		state.indexFactory.setType(opts.indexStoreConfig->type);
		state.indexFactory.setIndexFile( state.indexFile );
		state.indexFactory.setDeduplication(opts.indexStoreConfig->deduplicate);
	}

	std::string storeFileName = opts.inFileNames.front() + "/" + liboscar::toString(liboscar::FC_KV_STORE);

	
	//from pbf, first create the kvstore
	kvTime.begin();
	if (opts.inFileNames.size() > 1 || (sserialize::MmappedFile::fileExists(opts.inFileNames.front()) && !sserialize::MmappedFile::isDirectory(opts.inFileNames.front()))) {
		oscar_create::handleKVCreation(opts, state);
		storeFileName = opts.getOutFileName(liboscar::FC_KV_STORE);
	}
	else {//get the input index and insert it into our index and open the store
		writeSymlink = true;
		std::string idxStoreFileName = opts.inFileNames.front() + "/" + liboscar::toString(liboscar::FC_INDEX);
		storeFileName = opts.inFileNames.front() + "/" + liboscar::toString(liboscar::FC_KV_STORE);
		
		sserialize::Static::ItemIndexStore idxStore;;
		try {
			idxStore = sserialize::Static::ItemIndexStore( sserialize::UByteArrayAdapter::openRo(idxStoreFileName, false) );
		}
		catch (sserialize::Exception & e) {
			std::cerr << "Failed to initialize index at " << idxStoreFileName << " with error message: " << e.what();
			return -1;
		}
		//switch deduplication off in case the initial store was build without it
		state.indexFactory.setDeduplication(false);
		std::vector<uint32_t> tmp = state.indexFactory.insert(idxStore);
		for(std::size_t i = 0, s = tmp.size(); i < s; ++i) {
			if (i != tmp[i])  {
				std::cout << "ItemIndexFactory::insert is broken" << std::endl;
				std::cout << tmp << std::endl;
				return -1;
			}
		}
		if (opts.indexStoreConfig->deduplicate) {
			state.indexFactory.setDeduplication(true);
			std::cout << "Re-calculating ItemIndexFactory deduplication database..." << std::flush;
			state.indexFactory.recalculateDeduplicationData();
			std::cout << "done" << std::endl;
		}
		SSERIALIZE_CHEAP_ASSERT_EQUAL(idxStore.size(), state.indexFactory.size());
	}
	kvTime.end();

	//open the store
	try {
		state.storeData = sserialize::UByteArrayAdapter::openRo(storeFileName, false);
		#ifdef WITH_OSCAR_CREATE_NO_DATA_REFCOUNTING
		state.storeData.disableRefCounting();
		#endif
		state.store = liboscar::Static::OsmKeyValueObjectStore(state.storeData);
		#ifdef WITH_OSCAR_CREATE_NO_DATA_REFCOUNTING
		state.store.disableRefCounting();
		#endif
	}
	catch (sserialize::Exception & e) {
		std::cerr << "Failed to open store file at " << storeFileName << " with error message: "<< e.what();
		return -1;
	}
	SSERIALIZE_CHEAP_ASSERT(state.store.size());
	
	//create searches if need be
	searchTime.begin();
	oscar_create::handleSearchCreation(opts, state);
	searchTime.end();
	
	//finalize the index factory
	std::cout << "Serializing index" << std::endl;
	sserialize::OffsetType indexFlushedLength = state.indexFactory.flush();
	std::cout << "Index size: " << indexFlushedLength << std::endl;
	if (indexFlushedLength) {
		if (indexFlushedLength < 1024*1024 || indexFlushedLength < state.indexFactory.getFlushedData().size()) {
			sserialize::UByteArrayAdapter indexStorage = state.indexFactory.getFlushedData();
			state.indexFactory = sserialize::ItemIndexFactory();
			if (!indexStorage.shrinkStorage(indexStorage.size() - indexFlushedLength) || indexStorage.size() != indexFlushedLength) {
				std::cerr << "Failed to shrink index to correct size. Should=" << indexFlushedLength << " is " << indexStorage.size() << std::endl;
			}
			else {
				std::cout << "Shrinking index to correct size succeeded" << std::endl;
			}
		}
	}
	else {
		std::cerr <<  "Failed to serialize index" << std::endl;
		return -1;
	}
	if (writeSymlink) {
		std::string sourceStoreFileName = sserialize::MmappedFile::realPath(opts.inFileNames.front() + "/" + liboscar::toString(liboscar::FC_KV_STORE));
		std::string targetStoreFileName = opts.getOutFileDir() + "/" + liboscar::toString(liboscar::FC_KV_STORE);
		if (!sserialize::MmappedFile::createSymlink(sourceStoreFileName, targetStoreFileName)) {
			std::cerr << "Could not create symlink" << std::endl;
		}
	}
	
	if (opts.statsConfig.memUsage) {
		sserialize::MemUsage().print();
	}
	
	totalTime.end();
	std::cout << "KV-Store creation took " << kvTime << "\n";
	std::cout << "Search data structure creation took " << searchTime << "\n";
	std::cout << "Total time: " << totalTime << std::endl;
	
	#ifdef WITH_OSCAR_CREATE_NO_DATA_REFCOUNTING
	state.store.enableRefCounting();
	#endif
	
	return 0;
}
