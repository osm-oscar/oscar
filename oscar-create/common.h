#ifndef OSCAR_CREATE_COMMON_H
#define OSCAR_CREATE_COMMON_H
#include <osmpbf/common.h>
#include <liboscar/constants.h>

namespace oscar_create {

inline liboscar::OsmItemTypes toOsmItemType(osmpbf::PrimitiveType pt) {
	switch(pt) {
	case osmpbf::NodePrimitive:
		return liboscar::OSMIT_NODE;
	case osmpbf::WayPrimitive:
		return liboscar::OSMIT_WAY;
	case osmpbf::RelationPrimitive:
		return liboscar::OSMIT_RELATION;
	default:
		return liboscar::OSMIT_INVALID;
	};
}

}//end namespace oscar_create

#endif