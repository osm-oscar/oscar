#ifndef OSCAR_CMD_CONSISTENCY_CHECKERS_H
#define OSCAR_CMD_CONSISTENCY_CHECKERS_H
#include <liboscar/OsmKeyValueObjectStore.h>

namespace oscarcmd {

struct ConsistencyChecker {
	static bool checkIndex(const sserialize::Static::ItemIndexStore& indexStore);
	static bool checkGh(const liboscar::Static::OsmKeyValueObjectStore& store, const sserialize::Static::ItemIndexStore& indexStore);
	static bool checkStore(const liboscar::Static::OsmKeyValueObjectStore & store);
};


}

#endif