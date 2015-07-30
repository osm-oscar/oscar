#ifndef OSCAR_CMD_GEO_HIERARCHY_PRINTER_H
#define OSCAR_CMD_GEO_HIERARCHY_PRINTER_H
#include <liboscar/OsmKeyValueObjectStore.h>

namespace oscarcmd {

class GeoHierarchyPrinter {
private:
	void printNodes(std::ostream & out, const liboscar::Static::OsmKeyValueObjectStore & store);
public:
	GeoHierarchyPrinter();
	virtual ~GeoHierarchyPrinter();
	void printSimple(std::ostream & out, const liboscar::Static::OsmKeyValueObjectStore & store);
	void printChildrenWithNames(std::ostream & out, const liboscar::Static::OsmKeyValueObjectStore & store);
	void printParentsWithNames(std::ostream & out, const liboscar::Static::OsmKeyValueObjectStore & store);
};

}//end namespace

#endif