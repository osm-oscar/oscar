#include "helpers.h"
#include <iostream>
#include <osmpbf/parsehelpers.h>
#include <osmpbf/iway.h>
#include <sserialize/algorithm/utilmath.h>

namespace oscar_create {

bool findNodeIdBounds(const std::string & fileName, uint64_t & smallestId, uint64_t & largestId) {
	osmpbf::OSMFileIn inFile(fileName, false);
	if (!inFile.open()) {
		std::cout << "Failed to open " <<  fileName << std::endl;
		return false;
	}
	
	sserialize::AtomicMinMax<int64_t> minMaxId;
	std::atomic<uint64_t> numNodes(0);
	
	auto workFunc = [&minMaxId, &numNodes](osmpbf::PrimitiveBlockInputAdaptor & pbi) {
		sserialize::MinMax<int64_t> myMinMax;
		for(osmpbf::IWayStream way = pbi.getWayStream(); !way.isNull(); way.next()) {
			for(osmpbf::IWayStream::RefIterator it(way.refBegin()), end(way.refEnd()); it != end; ++it) {
				int64_t nId = std::max<int64_t>(*it, 0); //clip the id in case there are invalid entries (like negative ids)
				myMinMax.update(nId);
			}
		}
		minMaxId.update(myMinMax);
		numNodes += pbi.nodesSize();
	};
	
	osmpbf::parseFileCPPThreads(inFile, workFunc);
	
	largestId = std::max<int64_t>(minMaxId.max(), 0);
	smallestId = std::min<int64_t>(minMaxId.min(), 0);
	return largestId != smallestId;
}

uint64_t getNumBlocks(const std::string & fileName) {
	osmpbf::OSMFileIn inFile(fileName, false);
	osmpbf::PrimitiveBlockInputAdaptor pbi;

	if (!inFile.open()) {
		std::cout << "Failed to open " <<  fileName << std::endl;
		return 0;
	}
	
	uint64_t blockCount = 0;
	while(inFile.skipBlock()) {
		++blockCount;
	}
	return blockCount;
}

}//end namesapce