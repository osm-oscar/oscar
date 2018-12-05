#ifndef OSCAR_CMD_CONSISTENCY_CHECKERS_H
#define OSCAR_CMD_CONSISTENCY_CHECKERS_H
#include <liboscar/StaticOsmCompleter.h>
#include <sserialize/Static/CellTextCompleter.h>

namespace oscarcmd {

struct ConsistencyChecker {
	ConsistencyChecker(liboscar::Static::OsmCompleter & completer) : cmp(completer) {}
	liboscar::Static::OsmCompleter & cmp;
	bool debug{false};
	
	bool checkIndex();
	bool checkGh();
	bool checkTriangulation();
	bool checkStore();
	bool checkCTC();
};


}

#endif
