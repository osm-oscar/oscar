#ifndef OSCAR_CREATE_OSM_POLYGON_STORE_H
#define OSCAR_CREATE_OSM_POLYGON_STORE_H
#include <osmtools/OsmGridRegionTree.h>
#include "types.h"

namespace oscar_create {

// typedef sserialize::spatial::GeoRegionStore< BoundariesInfo > OsmPolygonStore;
typedef osmtools::OsmGridRegionTree<BoundariesInfo> OsmPolygonStore;

}//end namespace
#endif