#ifndef LIBOSCAR_STATIC_OSM_ITEMSET_H
#define LIBOSCAR_STATIC_OSM_ITEMSET_H
#include <ostream>
#include <sserialize/search/ItemSet.h>
#include <sserialize/search/FilteredItemSet.h>
#include "OsmKeyValueObjectStore.h"

namespace liboscar {
namespace Static {

typedef sserialize::ItemSet<liboscar::Static::OsmKeyValueObjectStoreItem, liboscar::Static::OsmKeyValueObjectStore> OsmItemSet;
typedef sserialize::FilteredItemSet<liboscar::Static::OsmKeyValueObjectStoreItem, liboscar::Static::OsmKeyValueObjectStore> OsmItemSetIterator;


inline std::ostream & operator<<(std::ostream & out, const Static::OsmItemSet & s) {
	for(size_t i = 0; i < s.size(); i++) {
		s.at(i).print(out, true);
	}
	return out;
}

inline std::ostream & operator<<(std::ostream & out, Static::OsmItemSetIterator & s) {
	for(size_t i = 0; i < s.cacheSize(); i++) {
		s.at(i).print(out, true);
	}
	return out;
}

}}//end namespace

#endif