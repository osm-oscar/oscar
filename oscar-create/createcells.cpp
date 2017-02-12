#include "AreaExtractor.h"
#include "CellCreator.h"
#include "Config.h"
#include "common.h"
#include <osmtools/AreaExtractor.h>

#include <liboscar/OsmIdType.h>

#include <osmpbf/pbistream.h>

using namespace oscar_create;

struct State {
	osmpbf::PbiStream input;
};

struct RegionInfo {
	RegionInfo() {}
	RegionInfo(const liboscar::OsmIdType & osmIdType) : osmIdType(osmIdType) {}
	liboscar::OsmIdType osmIdType;
};

int main(int argc, char ** argv) {
	Config cfg;
	
	auto ret = cfg.fromCmdLineArgs(argc, argv);
	
	if (ret == Config::RV_FAILED) {
		cfg.help();
		return -1;
	}
	else if (ret == Config::RV_HELP) {
		cfg.help();
		return 0;
	}
	if (!cfg.kvStoreConfig) {
		std::cerr << "Need a valid store config" << std::endl;
		return -1;
	}
	
	State state;
	try {
		state.input = std::move( osmpbf::PbiStream(cfg.inFileNames) );
	}
	catch (const std::exception & e) {
		std::cerr << "Error occured while opening files: " << e.what() << std::endl;
		return -1;
	}
	auto myFilter = AreaExtractor::nameFilter(cfg.kvStoreConfig->keysDefiningRegions, cfg.kvStoreConfig->keyValuesDefiningRegions);
	osmtools::AreaExtractor ae;
	osmtools::OsmGridRegionTree<RegionInfo> polyStore;
	ae.extract(state.input, [&polyStore](const std::shared_ptr<sserialize::spatial::GeoRegion> & region, osmpbf::IPrimitive & primitive) {
		liboscar::OsmIdType osmIdType(primitive.id(), oscar_create::toOsmItemType(primitive.type()));
		polyStore.push_back(*region, RegionInfo(osmIdType));
	}, osmtools::AreaExtractor::ET_ALL_SPECIAL_BUT_BUILDINGS, myFilter, cfg.kvStoreConfig->numThreads, "Extracting regions");
	
	polyStore.addPolygonsToRaster(cfg.kvStoreConfig->latCount, cfg.kvStoreConfig->lonCount);
	osmtools::OsmTriangulationRegionStore trs;
	
	return -1;
}