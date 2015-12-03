#include <iostream>
#include <memory>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/stats/memusage.h>
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
	sserialize::TimeMeasurer tm;
	tm.begin();

	{ //init rand
		timeval t1;
		gettimeofday(&t1, NULL);
		srand(t1.tv_usec * t1.tv_sec);
	}
	std::cout.precision(10);
	
	oscar_create::Config opts;
	oscar_create::State state;
	bool writeSymlink = false;
	
	int ret = opts.fromCmdLineArgs(argc, argv);
	if (ret != oscar_create::Config::RV_OK) {
		printHelp();
		return -1;
	}
	if (opts.validate() != oscar_create::Config::VRV_OK) {
		std::cout << "Congig is invalid with errorcode: " << opts.validate() << std::endl;
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
	
	//init the index factory
	{
		state.indexFactory.setCheckIndex(opts.indexStoreConfig->check);;
		state.indexFactory.setType(opts.indexStoreConfig->type);
		state.indexFactory.setIndexFile( state.indexFile );
		state.indexFactory.setDeduplication(opts.indexStoreConfig->deduplicate);
	}

	std::string storeFileName = opts.inFileName + "/" + liboscar::toString(liboscar::FC_KV_STORE);

	
	//from pbf, first create the kvstore
	if (sserialize::MmappedFile::fileExists(opts.inFileName) && !sserialize::MmappedFile::isDirectory(opts.inFileName)) {
		oscar_create::handleKVCreation(opts, state);
		storeFileName = opts.getOutFileName(liboscar::FC_KV_STORE);
	}
	else {//get the input index and insert it into our index and open the store
		std::string idxStoreFileName = opts.inFileName + "/" + liboscar::toString(liboscar::FC_INDEX);
		storeFileName = opts.inFileName + "/" + liboscar::toString(liboscar::FC_KV_STORE);
		
		sserialize::Static::ItemIndexStore idxStore;;
		try {
			idxStore = sserialize::Static::ItemIndexStore( sserialize::UByteArrayAdapter::openRo(idxStoreFileName, false) );
		}
		catch (sserialize::Exception & e) {
			std::cerr << "Failed to initial index at " << idxStoreFileName << " with error message: " << e.what();
			return -1;
		}
		//switch deduplication off in case the initial store was build without it
		state.indexFactory.setDeduplication(false);
		std::vector<uint32_t> tmp = state.indexFactory.insert(idxStore);
		for(uint32_t i = 0, s = tmp.size(); i < s; ++i) {
			if (i != tmp[i])  {
				std::cout << "ItemIndexFactory::insert is broken" << std::endl;
				std::cout << tmp << std::endl;
				return -1;
			}
		}
		state.indexFactory.setDeduplication(opts.indexStoreConfig->deduplicate);
	}

	//open the store
	try {
		state.store = liboscar::Static::OsmKeyValueObjectStore(sserialize::UByteArrayAdapter::openRo(storeFileName, false));
	}
	catch (sserialize::Exception & e) {
		std::cerr << "Failed to open store file at " << storeFileName << " with error message: "<< e.what();
	}
	assert(state.store.size());
	
	//create searches if need be
	oscar_create::handleSearchCreation(opts, state);
	
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
		std::string sourceStoreFileName = sserialize::MmappedFile::realPath(opts.inFileName + "/" + liboscar::toString(liboscar::FC_KV_STORE));
		std::string targetStoreFileName = opts.getOutFileDir() + "/" + liboscar::toString(liboscar::FC_KV_STORE);
		if (!sserialize::MmappedFile::createSymlink(sourceStoreFileName, targetStoreFileName)) {
			std::cerr << "Could not create symlink" << std::endl;
		}
	}
	
	
	if (opts.statsConfig.memUsage) {
		sserialize::MemUsage().print();
	}
	
	tm.end();
	std::cout << "Total time: " << tm << std::endl;
	return 0;
}
