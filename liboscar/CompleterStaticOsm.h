#ifndef LIBOSCAR_COMPLETER_PRIVATE_STATIC_OSM_H
#define LIBOSCAR_COMPLETER_PRIVATE_STATIC_OSM_H
#include <sserialize/search/Completer.h>
#include "OsmKeyValueObjectStore.h"

namespace liboscar {

/** This is the refcount wrapper for the Static::OsmCompleter */
typedef sserialize::Completer<liboscar::Static::OsmKeyValueObjectStore::Item, liboscar::Static::OsmKeyValueObjectStore> CompleterStaticOsm;

}

#endif