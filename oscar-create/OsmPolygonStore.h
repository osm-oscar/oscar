#ifndef OSCAR_CREATE_OSM_POLYGON_STORE_H
#define OSCAR_CREATE_OSM_POLYGON_STORE_H
#include <osmtools/OsmGridRegionTree.h>
#include "BoundariesInfo.h"
namespace oscar_create {

typedef osmtools::OsmGridRegionTree<BoundariesInfo> OsmPolygonStore;

}//end namespace
#endif