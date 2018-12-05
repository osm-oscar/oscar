#ifndef OSCAR_CMD_CONSISTENCY_CHECKERS_H
#define OSCAR_CMD_CONSISTENCY_CHECKERS_H
#include <liboscar/OsmKeyValueObjectStore.h>

namespace oscarcmd {

struct ConsistencyChecker {
	bool debug{false};
	bool checkIndex(const sserialize::Static::ItemIndexStore& indexStore);
	bool checkGh(const liboscar::Static::OsmKeyValueObjectStore& store, const sserialize::Static::ItemIndexStore& indexStore);
	bool checkTriangulation(const liboscar::Static::OsmKeyValueObjectStore& store);
	bool checkStore(const liboscar::Static::OsmKeyValueObjectStore & store);
};


}

#endif
