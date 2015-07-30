#ifndef LIBOSCAR_GEO_SEARCH_H
#define LIBOSCAR_GEO_SEARCH_H
#include <unordered_map>
#include <sserialize/search/StringCompleter.h>
#include <sserialize/search/GeoCompleter.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/Array.h>
#include <liboscar/OsmKeyValueObjectStore.h>
#define LIBOSCAR_GEO_SEARCH_VERSION 1

namespace liboscar {

/** file layout:
  *
  *---------------------------------------
  *VERSION|Array<(u8|Static::StringCompleter)>
  *---------------------------------------
  *
  */

class GeoSearch {
public:
	enum Type {ITEMS=0};
private:
	std::unordered_map<uint32_t, std::vector< sserialize::GeoCompleter > > m_completers;
public:
	GeoSearch() {}
	GeoSearch(const sserialize::UByteArrayAdapter& d, const sserialize::Static::ItemIndexStore& indexStore, const Static::OsmKeyValueObjectStore & store);
	virtual ~GeoSearch() {}
	inline const std::vector< sserialize::GeoCompleter >  & get(Type t) const { return m_completers.at(t); }
	inline const sserialize::GeoCompleter & get(Type t, uint32_t pos) const { return m_completers.at(t).at(pos); }
	inline sserialize::GeoCompleter & get(Type t, uint32_t pos) { return m_completers.at(t).at(pos); }
	inline bool hasSearch(Type t) const { return m_completers.count(t);}
	inline uint32_t size(Type t) const { return (hasSearch(t) ? m_completers.at(t).size() : 0); }
};

class GeoSearchCreator: protected sserialize::Static::ArrayCreator<sserialize::UByteArrayAdapter> {
public:
	typedef sserialize::Static::ArrayCreator<sserialize::UByteArrayAdapter> MyBaseClass;
private:
	static inline sserialize::UByteArrayAdapter & addVersion(sserialize::UByteArrayAdapter & destination) {
		destination.putUint8(LIBOSCAR_GEO_SEARCH_VERSION);
		return destination;
	}
public:
	GeoSearchCreator(sserialize::UByteArrayAdapter & destination) : MyBaseClass(addVersion(destination)) {}
	virtual ~GeoSearchCreator() {}
	void beginRawPut(GeoSearch::Type t) {
		MyBaseClass::beginRawPut();
		MyBaseClass::rawPut().putUint8(t);
	}
	void put(GeoSearch::Type t, const sserialize::UByteArrayAdapter & value) {
		beginRawPut(t);
		MyBaseClass::rawPut().putData(value);
		MyBaseClass::endRawPut();
	}
	using MyBaseClass::endRawPut;
	using MyBaseClass::flush;
	using MyBaseClass::reserveOffsets;
	using MyBaseClass::rawPut;
};

}//end namespace

#endif