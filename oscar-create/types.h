#ifndef OSCAR_CREATE_TYPES_H
#define OSCAR_CREATE_TYPES_H
#include <liboscar/OsmIdType.h>
#include <ostream>

namespace oscar_create {

struct BoundariesInfo {
	BoundariesInfo() : adminLevel(0xFF) {}
	BoundariesInfo(uint8_t adminLevel) : adminLevel(adminLevel) {}
	liboscar::OsmIdType osmIdType;
	uint8_t adminLevel; //0xFF is reserved for everything that is not a postal code, this way we can sort by adminlevel
	inline bool operator<(const BoundariesInfo & other) const {
		return (adminLevel < other.adminLevel);
	}
	inline bool operator<=(const BoundariesInfo & other) const {
		return (adminLevel <= other.adminLevel);
	}
	inline bool operator>(const BoundariesInfo & other) const {
		return (adminLevel > other.adminLevel);
	}
	inline bool operator>=(const BoundariesInfo & other) const {
		return (adminLevel >= other.adminLevel);
	}
};

inline std::ostream& operator<<(std::ostream& out, const BoundariesInfo & binfo) {
	out << (int)binfo.adminLevel;
	return out;
}

} //end namespace


#endif