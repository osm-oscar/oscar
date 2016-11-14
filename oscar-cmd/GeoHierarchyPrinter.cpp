#include "GeoHierarchyPrinter.h"
#include <sserialize/stats/ProgressInfo.h>

namespace oscarcmd {

std::string escapeString(const std::string & str) {
	std::string ns;
	ns.reserve(str.size());
	for(std::string::const_iterator it(str.cbegin()), end(str.cend()); it != end; ++it) {
		if (*it == '"' || *it == '+') {
// 			ns.push_back('\\');
		}
		else {
			ns.push_back(*it);
		}
	}
	return ns;
}

void printNode(std::ostream & out, uint32_t i, const sserialize::Static::spatial::GeoHierarchy::Region & r, const std::string & label) {
	out << "\tR" << i;
	out << "[ label=\"sid:" << r.storeId() << ";ghId=" << r.ghId() << ";";
	out << "cs:" << r.childrenSize();
	out << "ps:" << r.parentsSize();
	out << label;
	out << "\" ];" << std::endl;
	out << std::endl;
}

void GeoHierarchyPrinter::printNodes(std::ostream& out, const liboscar::Static::OsmKeyValueObjectStore& store) {
	const sserialize::Static::spatial::GeoHierarchy & gh = store.geoHierarchy();
	uint32_t regionSize = gh.regionSize();
	sserialize::Static::spatial::GeoHierarchy::Region r;
	liboscar::Static::OsmKeyValueObjectStore::Item item;
	std::string name;
	auto nameStrId = store.keyStringTable().find("name");
	sserialize::ProgressInfo info;
	info.begin(regionSize, "GeoHierarchyPrinter:: Printing nodes");
	for(uint32_t i = 0; i < regionSize; ++i) {
		r = gh.region(i);
		item = store.at(r.storeId());
		name.clear();
		uint32_t nameId = liboscar::Static::OsmKeyValueObjectStoreItem::npos;
		if (nameStrId != store.keyStringTable().npos) {
			nameId = item.findKey(static_cast<uint32_t>(nameStrId), (uint32_t)0);
		}
		if (nameId != liboscar::Static::OsmKeyValueObjectStoreItem::npos) {
			name = item.value(nameId);
		}
		name = "name:" + escapeString(name);
		printNode(out, i, r, name);
		info(i);
	}
	info.end();
	r = gh.rootRegion();
	out << "\tR" << regionSize << "[ root=true; label=\"ROOT_REGION;cs:" << r.childrenSize() << "\" ];" << std::endl;
	out << "\troot R" << store.geoHierarchy().regionSize() << ";" << std::endl;
}


GeoHierarchyPrinter::GeoHierarchyPrinter() {}
GeoHierarchyPrinter::~GeoHierarchyPrinter() {}

void GeoHierarchyPrinter::printSimple(std::ostream& out, const liboscar::Static::OsmKeyValueObjectStore& store) {
	out << "digraph simpleHierarchy {" << std::endl;
	const sserialize::Static::spatial::GeoHierarchy & gh = store.geoHierarchy();
	uint32_t regionSize = gh.regionSize();
	for(uint32_t i = 0; i < regionSize; ++i) {
		out << "\tR" << i << " -> {";
		sserialize::Static::spatial::GeoHierarchy::Region r = gh.region(i);
		uint32_t childrenSize = r.childrenSize();
		if (childrenSize) {
			for(uint32_t j = 0, s = childrenSize-1; i < s; ++i) {
				out << "R" << r.child(j) << " ; ";
			}
			out << "R" << r.child(childrenSize-1) << ";";
		}
		out << "}" << std::endl;
	}
	out << "}";
}

void GeoHierarchyPrinter::printChildrenWithNames(std::ostream& out, const liboscar::Static::OsmKeyValueObjectStore& store) {
	out << "digraph simpleHierarchy {" << std::endl;
	printNodes(out, store);

	const sserialize::Static::spatial::GeoHierarchy & gh = store.geoHierarchy();
	uint32_t regionSize = gh.regionSize();
	sserialize::Static::spatial::GeoHierarchy::Region r;
	sserialize::ProgressInfo info;
	info.begin(regionSize, "GeoHierarchyPrinter:: Printing edges");
	for(uint32_t i = 0; i < regionSize; ++i) {
		r = gh.region(i);
		uint32_t childrenSize = r.childrenSize();
		if (!childrenSize)
			continue;
		out << "\tR" << i << " -> {";
		for(uint32_t j = 0, s = childrenSize-1; j < s; ++j) {
			out << "R" << r.child(j) << " ; ";
		}
		out << "R" << r.child(childrenSize-1) << ";";
		out << "};" << std::endl;
		info(regionSize);
	}
	info.end();
	r = gh.rootRegion();
	if (r.childrenSize()) {
		uint32_t childrenSize = r.childrenSize();
		out << "\tR" << regionSize << " -> {";
		for(uint32_t j = 0, s = childrenSize-1; j < s; ++j) {
			out << "R" << r.child(j) << " ; ";
		}
		out << "R" << r.child(childrenSize-1) << ";";
		out << "};" << std::endl;
	}
	
	out << "}";
}

void GeoHierarchyPrinter::printParentsWithNames(std::ostream& out, const liboscar::Static::OsmKeyValueObjectStore& store) {
	out << "digraph simpleHierarchy {" << std::endl;
	printNodes(out, store);

	const sserialize::Static::spatial::GeoHierarchy & gh = store.geoHierarchy();
	uint32_t regionSize = gh.regionSize();
	for(uint32_t i = 0; i < regionSize; ++i) {
		sserialize::Static::spatial::GeoHierarchy::Region r = gh.region(i);
		uint32_t parentsSize = r.parentsSize();
		if (!parentsSize)
			continue;
		out << "\tR" << i << " -> {";
		for(uint32_t j = 0, s = parentsSize-1; j < s; ++j) {
			out << "R" << r.parent(j) << " ; ";
		}
		out << "R" << r.parent(parentsSize-1) << ";";
		out << "};" << std::endl;
	}
	out << "}";
}


}//end namespace