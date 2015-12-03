#ifndef LIBOSCAR_TEXT_SEARCH_H
#define LIBOSCAR_TEXT_SEARCH_H
#include <unordered_map>
#include <sserialize/search/StringCompleter.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/Static/TriangulationGeoHierarchyArrangement.h>
#define LIBOSCAR_TEXT_SEARCH_VERSION 2

namespace liboscar {
/** file layout:
  *
  *
  *-------------------------------------------
  *VERSION|Array<(u8|Static::StringCompleter)>
  *-------------------------------------------
  *
  * VERSION 2:
  * Change TextSearch::Type to begin with 1 and add GEOHIERARCHY_AND_ITEMS completer
  *
  *
  *
  */

class TextSearch {
public:
	enum Type {NONE=0, BEGIN_STRING_COMPLETERS=1, ITEMS=1, GEOHIERARCHY=2, END_STRING_COMPLETERS=3,
						BEGIN_CELL_TEXT_COMPLETERS=3, GEOCELL=3, OOMGEOCELL=4, END_CELL_TEXT_COMPLETERS=5,
						GEOHIERARCHY_AND_ITEMS=6,
						END_ALL_COMPLETERS=7, INVALID=0xFF};

	typedef sserialize::RCPtrWrapper<sserialize::RefCountObject> CompleterBasePtr;
	///Traits class to calculate the result of getAuto()
	template<liboscar::TextSearch::Type TextSearchType>
	struct CompleterType;

private:
	template<typename T>
	struct CompletersInfo {
		typedef T value_type;
		CompletersInfo() : selected(0) {}
		uint32_t selected;
		std::vector<T> completers;
		uint32_t size() const { return completers.size(); }
		inline void push_back(const T & t) { completers.push_back(t); }
		template<typename...Args>
		inline void emplace_back(Args &&...args) { completers.emplace_back(std::forward<Args>(args)...); }
		inline const T & at(uint32_t pos) const { return completers.at(pos);}
		inline T & at(uint32_t pos) { return completers.at(pos); }
		inline const T & get() const { return at(selected);}
		inline T & get() { return at(selected);}
		inline bool select(uint32_t which) { return ((selected = (size() > which ? which : selected)) == which);}
	};
private:
	std::unordered_map<uint32_t, CompletersInfo<CompleterBasePtr> > m_completers;
public:
	TextSearch() {}
	TextSearch(const sserialize::UByteArrayAdapter& d, const sserialize::Static::ItemIndexStore& indexStore, const sserialize::Static::spatial::GeoHierarchy& gh, const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement& ra);
	virtual ~TextSearch() {}
	inline bool hasSearch(Type t) const { return m_completers.count(t);}
	inline uint32_t selectedTextSearcher(Type t) const { return (hasSearch(t) ? m_completers.at(t).selected : std::numeric_limits<uint32_t>::max()); }
	inline bool select(Type t, uint32_t which) { return (hasSearch(t) ? m_completers.at(t).select(which) : false);}
	inline uint32_t size(Type t) const { return (hasSearch(t) ? m_completers.at(t).size() : 0); }
	
	template<TextSearch::Type TType>
	typename CompleterType<TType>::type get(uint32_t pos) const;
	template<TextSearch::Type TType>
	typename CompleterType<TType>::type get() const;
};

class TextSearchCreator: protected sserialize::Static::ArrayCreator<sserialize::UByteArrayAdapter> {
public:
	typedef sserialize::Static::ArrayCreator<sserialize::UByteArrayAdapter> MyBaseClass;
private:
	static inline sserialize::UByteArrayAdapter & addVersion(sserialize::UByteArrayAdapter & destination) {
		destination.putUint8(LIBOSCAR_TEXT_SEARCH_VERSION);
		return destination;
	}
public:
	TextSearchCreator(sserialize::UByteArrayAdapter & destination) : MyBaseClass(addVersion(destination)) {}
	virtual ~TextSearchCreator() {}
	void beginRawPut(TextSearch::Type t) {
		MyBaseClass::beginRawPut();
		MyBaseClass::rawPut().putUint8(t);
	}
	void put(TextSearch::Type t, const sserialize::UByteArrayAdapter & value) {
		beginRawPut(t);
		MyBaseClass::rawPut().putData(value);
		MyBaseClass::endRawPut();
	}
	using MyBaseClass::endRawPut;
	using MyBaseClass::flush;
	using MyBaseClass::reserveOffsets;
	using MyBaseClass::rawPut;
};
}//end namespace liboscar

#include <sserialize/Static/CellTextCompleter.h>

namespace liboscar {

template<>
struct TextSearch::CompleterType<liboscar::TextSearch::ITEMS> {
	typedef sserialize::StringCompleter type;
	static type get(sserialize::RefCountObject * base) {
		return type( dynamic_cast<sserialize::StringCompleterPrivate*>(base) );
	}
};

template<>
struct TextSearch::CompleterType<liboscar::TextSearch::GEOHIERARCHY> {
	typedef sserialize::StringCompleter type;
	static type get(sserialize::RefCountObject * base) {
		return type( dynamic_cast<sserialize::StringCompleterPrivate*>(base) );
	}
};

template<>
struct TextSearch::CompleterType<liboscar::TextSearch::GEOCELL> {
	typedef sserialize::Static::CellTextCompleter type;
	static type get(sserialize::RefCountObject * base) {
		return type( dynamic_cast<sserialize::Static::detail::CellTextCompleter*>(base) );
	}
};

template<liboscar::TextSearch::Type TType>
typename liboscar::TextSearch::CompleterType<TType>::type
TextSearch::get(uint32_t pos) const {
	return liboscar::TextSearch::CompleterType<TType>::get( const_cast<sserialize::RefCountObject*>(m_completers.at(TType).at(pos).get()));
}

template<liboscar::TextSearch::Type TType>
typename liboscar::TextSearch::CompleterType<TType>::type
TextSearch::get() const {
	return get<TType>(selectedTextSearcher(TType));
}

}//end namespace liboscar::detail::TextSearch

#endif