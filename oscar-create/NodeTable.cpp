#include "NodeTable.h"
#include <osmpbf/osmfilein.h>
#include <osmpbf/parsehelpers.h>
#include <osmpbf/inode.h>
#include <memory>
#include <sserialize/algorithm/OutOfMemoryAlgos.h>

namespace oscar_create {

NodeTable::NodeTable() {}
NodeTable::~NodeTable() {}

void NodeTable::populate(const std::string& filename) {
	osmpbf::OSMFileIn osmfile(filename);
	
	if (!osmfile.open()) {
		throw std::runtime_error("NodeTable: Could not open osm.pbf file");
	}
	sserialize::MMVector< std::pair<int64_t, RawGeoPoint> > tmpDs;
	std::mutex tmpDsLock;
	
	osmpbf::parseFileCPPThreads(osmfile,
		[&tmpDs,&tmpDsLock](osmpbf::PrimitiveBlockInputAdaptor & pbi) {
			if (!pbi.nodesSize())
				return;
			std::vector< std::pair<int64_t, RawGeoPoint> > tmp;
			for(osmpbf::INodeStream node(pbi.getNodeStream()); !node.isNull(); node.next()) {
				tmp.emplace_back(node.id(), RawGeoPoint(node.latd(), node.lond()));
			}
			std::unique_lock<std::mutex> lck(tmpDsLock);
			tmpDs.push_back(tmp.cbegin(), tmp.cend());
		}
	);
	sserialize::OutOfMemorySorter< std::pair<int64_t, RawGeoPoint> > ooms;
	ooms.sort(tmpDs, std::less< std::pair<int64_t, RawGeoPoint> >());
	
}




}