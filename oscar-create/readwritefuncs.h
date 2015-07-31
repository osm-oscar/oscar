#ifndef OSCAR_CREATE_READ_WRITE_FUNCS_H
#define OSCAR_CREATE_READ_WRITE_FUNCS_H
#include <sserialize/containers/GeneralizedTrie.h>
#include <sserialize/Static/TrieNode.h>
#include <liboscar/OsmKeyValueObjectStore.h>
#include <sserialize/spatial/ItemGeoGrid.h>
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/spatial/GridRTree.h>
#include <string>
#include "Config.h"
#include "TagStore.h"
#include "OsmKeyValueObjectStore.h"

namespace oscar_create {

bool findNodeIdBounds(const std::string & fileName, uint64_t & smallestId, uint64_t & largestId);
uint64_t getNumBlocks(const std::string & fileName);

liboscar::Static::OsmKeyValueObjectStore mangleAndWriteKv(oscar_create::OsmKeyValueObjectStore & store, const oscar_create::Config & opts);
void handleKv(liboscar::Static::OsmKeyValueObjectStore & kvStore, oscar_create::Config & opts);
bool writeItemIndexFactory(sserialize::ItemIndexFactory& indexFactory);

///Rewerites data if it should be compressed @param src source data to be processed (should already be on disk)
// void handleData(sserialize::UByteArrayAdapter & src, const oscar_create::Config & opts, liboscar::FileConfig fc);

template<typename T_DATABASE_TYPE>
bool createAndWriteGrid(const oscar_create::Config& opts, T_DATABASE_TYPE & db, sserialize::ItemIndexFactory& indexFactory, sserialize::UByteArrayAdapter & dest) {
	if (opts.gridLatCount == 0 || opts.gridLonCount == 0)
		return false;
	std::cout << "Finding grid dimensions..." << std::flush;
	sserialize::spatial::GeoRect rect( db.boundary() );
// 	std::cout << rect;
// 	std::cout << std::endl;
	sserialize::spatial::ItemGeoGrid grid(rect, opts.gridLatCount, opts.gridLonCount);
	std::cout << "Creating Grid" << std::endl;
	sserialize::ProgressInfo info;
	info.begin(db.size());
	bool allOk = true;
	for(size_t i = 0; i < db.size(); i++) {
		allOk = grid.addItem(i, db.geoShape(i)) && allOk;
		info(i);
	}
	info.end("Done Creating grid");
	std::cout <<  "Serializing Grid" << std::endl;
	std::vector<uint8_t> dataStorage;
	dataStorage.resize(opts.gridLatCount*opts.gridLonCount*4);
	sserialize::UByteArrayAdapter dataAdap(&dataStorage);
	
	grid.serialize(dataAdap, indexFactory);
	std::cout << "Grid serialized" << std::endl;
	dest.putData(dataStorage);
	return allOk;
}

template<typename TContainer>
struct ItemBoundaryGenerator {
	TContainer * c;
	uint32_t p;
	ItemBoundaryGenerator(TContainer * c) : c(c), p(0) {}
	inline bool valid() const { return p < c->size(); }
	inline uint32_t id() const { return p; }
	inline sserialize::Static::spatial::GeoShape shape() {
		return c->geoShape(p);
	}
	inline void next() { ++p;}
};

template<typename T_DATABASE_TYPE>
bool createAndWriteGridRTree(const oscar_create::Config& opts, T_DATABASE_TYPE & db, sserialize::ItemIndexFactory& indexFactory, sserialize::UByteArrayAdapter & dest) {
	if (opts.gridRTreeLatCount == 0 || opts.gridRTreeLonCount == 0)
		return false;
	std::cout << "Finding grid dimensions..." << std::flush;
	sserialize::spatial::GeoRect rect( db.boundary() );
	sserialize::spatial::GridRTree grid(rect, opts.gridRTreeLatCount, opts.gridRTreeLonCount);
	ItemBoundaryGenerator<T_DATABASE_TYPE> generator(&db);
	grid.bulkLoad(generator);
	std::cout << "Creating GridRTree" << std::endl;
	std::cout <<  "Serializing GridRTree" << std::endl;
	std::vector<uint8_t> dataStorage;
	sserialize::UByteArrayAdapter dataAdap(&dataStorage);
	
	grid.serialize(dataAdap, indexFactory);
	std::cout << "GridRTree serialized" << std::endl;
	dest.putData(dataStorage);
	return true;
}

}

#endif