#include <iostream>
#include <memory>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/stats/memusage.h>
#include "OsmKeyValueObjectStore.h"
#include "helpers.h"
#include "readwritefuncs.h"
#include "AreaExtractor.h"

void printHelp() {
	std::cout << "oscar-create creates all data necessary for oscar-(web|cmd|gui) out of .osm.bf files." << std::endl;
	std::cout << oscar_create::Config::help() << std::endl;
}

int main(int argc, char ** argv) {
	{
	timeval t1;
	gettimeofday(&t1, NULL);
	srand(t1.tv_usec * t1.tv_sec);
	}
	std::cout.precision(10);
	oscar_create::Config opts;
	int ret = opts.fromCmdLineArgs(argc, argv);
	if (ret != oscar_create::Config::RV_OK) {
		printHelp();
		return 1;
	}
	std::cout << "Selected Options:" << std::endl;
	std::cout << opts << std::endl;

	if (!sserialize::MmappedFile::isDirectory( opts.getOutFileDir() ) && ! sserialize::MmappedFile::createDirectory(opts.getOutFileDir())) {
		std::cout << "Failed to create destination directory" << std::endl;
		return -1;
	}
	try {
		if (opts.createKVStore) {
			handleKVCreation(opts);
		}
		else {
			handleSearchCreation(opts);
		}
	}
	catch (std::exception & e) {
		std::cout << "An error occured while trying to process your request:" << e.what() << std::endl;
		return -1;
	}
	sserialize::MemUsage().print();
	
	return 0;
}
