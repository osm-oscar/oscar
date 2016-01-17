#ifndef LIBOSCAR_OSM_ID_TYPE_H
#define LIBOSCAR_OSM_ID_TYPE_H
#include <liboscar/constants.h>
#include <sserialize/storage/UByteArrayAdapter.h>

namespace liboscar {

class OsmIdType final {
private:
	int64_t m_d;
public:
	OsmIdType() : OsmIdType(-1, liboscar::OSMIT_INVALID) {}
	OsmIdType(int64_t id, liboscar::OsmItemTypes type) : m_d((id << 2) | type) {}
	OsmIdType(int64_t raw) : m_d(raw) {}
	OsmIdType(const sserialize::UByteArrayAdapter & d) : OsmIdType(d.getVlPackedInt64(0)) {}
	OsmIdType(const OsmIdType & other) : m_d(other.m_d) {}
	~OsmIdType() {}
	inline const int64_t & raw() const { return m_d; }
	inline int64_t & raw() { return m_d; }
	inline int64_t id() const { return m_d >> 2; }
	inline void id(int64_t v) { m_d = (v << 2) | type(); }
	///type as defined by liboscar::OsmItemTypes
	inline uint8_t type() const { return (m_d & 0x3); }
	inline void type(liboscar::OsmItemTypes v) { m_d = (id() << 2) | v; }
	inline bool operator==(const OsmIdType & other) const { return m_d == other.m_d; }
	inline bool operator!=(const OsmIdType & other) const { return m_d != other.m_d; }
	///serialized as vs64
	inline sserialize::UByteArrayAdapter & append(sserialize::UByteArrayAdapter & dest) const {
		dest.putVlPackedInt64(m_d);
		return dest;
	}
};

inline sserialize::UByteArrayAdapter & operator<<(sserialize::UByteArrayAdapter & dest, const OsmIdType & src) {
	return src.append(dest);
}

inline sserialize::UByteArrayAdapter & operator>>(sserialize::UByteArrayAdapter & dest, OsmIdType & src) {
	dest >> src.raw();
	return dest;
}

}//end namespace liboscar

namespace std {


template<>
struct hash<liboscar::OsmIdType> {
	std::hash<int64_t> hasher;
	inline size_t operator()(const liboscar::OsmIdType & v) const {
		return hasher(v.raw());
	}
};

}//end namespace std

#endif