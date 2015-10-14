#ifndef LIBOSCAR_STATIC_CELL_TEXT_COMPLETER_H
#define LIBOSCAR_STATIC_CELL_TEXT_COMPLETER_H
#include <sserialize/Static/UnicodeTrie/FlatTrie.h>
#include <sserialize/Static/UnicodeTrie/Trie.h>
#include <sserialize/containers/UnicodeStringMap.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/spatial/TreedCQR.h>
#include <sserialize/containers/ItemIndexFactory.h>
#include <sserialize/search/StringCompleter.h>
#include <sserialize/containers/RLEStream.h>
#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/Static/TriangulationGeoHierarchyArrangement.h>

#define LIBOSCAR_STATIC_CELL_TEXT_COMPLETER_VERSION 2

namespace liboscar {
namespace Static {

/** FileFormat v2
  *
  * --------------------------------------------------------------------------------------------------------
  * VERSION|SupportedQuerries|TrieTypeMarker|TrieType<CellTextCompleter::Payload>
  * --------------------------------------------------------------------------------------------------------
  *  u8    |u8               |uint8         |
  * 
  * Payload format:
  *
  * -------------------------------------------------------------------------
  * Types|Offsets|(fmPtr|pPtr|pItemsPtr)
  * -------------------------------------------------------------------------
  *  u8  |vu32   |RLEStream
  * 
  * The first offset is always 0 and will not be stored
  * 
  */
namespace detail {

class CellTextCompleter: public sserialize::RefCountObject {
public:
	class Payload {
	public:
		class Type {
		public:
			typedef sserialize::RLEStream const_iterator;
			typedef sserialize::RLEStream BaseContainer;
		private:
			uint32_t m_fmPtr;
			uint32_t m_pPtr;
			BaseContainer m_data;
		public:
			Type() {}
			Type(const BaseContainer & d) : m_data(d) {
				m_fmPtr = *m_data;
				++m_data;
				m_pPtr = *m_data;
				++m_data;
			}
			inline bool valid() const { return m_data.hasNext(); }
			inline const BaseContainer & data() const { return m_data; }
			inline uint32_t fmPtr() const {
				return m_fmPtr;
			}
			inline uint32_t pPtr() const {
				return m_pPtr;
			}
			inline const_iterator pItemsPtrBegin() const {
				return m_data;
			}
		};
	private:
		uint32_t m_types;
		uint32_t m_offsets[3];
		sserialize::UByteArrayAdapter m_data;
	public:
		Payload() {}
		Payload(const sserialize::UByteArrayAdapter& d);
		~Payload() {}
		inline sserialize::StringCompleter::QuerryType types() const { return (sserialize::StringCompleter::QuerryType) (m_types & 0xF); }
		Type type(sserialize::StringCompleter::QuerryType qt) const;
		sserialize::UByteArrayAdapter typeData(sserialize::StringCompleter::QuerryType qt) const;
		
	};
	typedef enum {TT_TRIE=0, TT_FLAT_TRIE=1} TrieTypeMarker;
	typedef sserialize::UnicodeStringMap<Payload> Trie;
	typedef sserialize::Static::UnicodeTrie::UnicodeStringMapFlatTrie<Payload> FlatTrieType;
	typedef sserialize::Static::UnicodeTrie::UnicodeStringMapTrie<Payload> TrieType;
// 	typedef sserialize::Static::UnicodeTrie::Trie<Payload> Trie;
// 	typedef Trie::Node Node;
private:
	sserialize::StringCompleter::SupportedQuerries m_sq;
	TrieTypeMarker m_tt;
	Trie m_trie;
	sserialize::Static::ItemIndexStore m_idxStore;
	sserialize::Static::spatial::GeoHierarchy m_gh;
	sserialize::Static::spatial::TriangulationGeoHierarchyArrangement m_ra;
public:
	CellTextCompleter();
	CellTextCompleter(const sserialize::UByteArrayAdapter & d, const sserialize::Static::ItemIndexStore & idxStore, const sserialize::Static::spatial::GeoHierarchy & gh, const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement & ra);
	virtual ~CellTextCompleter();
	const Trie & trie() const { return m_trie;}
	Trie & trie() { return m_trie;}
	bool count(const std::string::const_iterator& begin, const std::string::const_iterator& end) const;
	Payload::Type typeFromCompletion(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const;
	sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;
	inline const sserialize::Static::ItemIndexStore & idxStore() const { return m_idxStore; }
	std::ostream & printStats(std::ostream & out) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE complete(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE regions(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE items(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE fromRegionStoreId(uint32_t id) const;
	
	template<typename T_CQR_TYPE>
	T_CQR_TYPE fromCellId(uint32_t id) const;
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	T_CQR_TYPE cqrFromRect(const sserialize::spatial::GeoRect & rect) const;

	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	T_CQR_TYPE cqrBetween(const sserialize::spatial::GeoPoint & start, const sserialize::spatial::GeoPoint & end, double radius) const;
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult, typename T_GEOPOINT_ITERATOR>
	T_CQR_TYPE cqrAlongPath(double radius, const T_GEOPOINT_ITERATOR & begin, const T_GEOPOINT_ITERATOR & end) const;
};

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::complete(const std::string& qstr, const sserialize::StringCompleter::QuerryType qt) const {
	try {
		Payload::Type t(typeFromCompletion(qstr, qt));
		return T_CQR_TYPE(m_idxStore.at( t.fmPtr() ), m_idxStore.at( t.pPtr() ), t.pItemsPtrBegin(), m_gh, m_idxStore);
	}
	catch (const sserialize::OutOfBoundsException & e) {
		return T_CQR_TYPE();
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::regions(const std::string& qstr, const sserialize::StringCompleter::QuerryType qt) const {
	try {
		Payload::Type t(typeFromCompletion(qstr, qt));
		return T_CQR_TYPE(m_idxStore.at( t.fmPtr() ), m_gh, m_idxStore);
	}
	catch (const sserialize::OutOfBoundsException & e) {
		return T_CQR_TYPE();
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::items(const std::string& qstr, const sserialize::StringCompleter::QuerryType qt) const {
	try {
		Payload::Type t(typeFromCompletion(qstr, qt));
		return T_CQR_TYPE(sserialize::ItemIndex(), m_idxStore.at( t.pPtr() ), t.pItemsPtrBegin(), m_gh, m_idxStore);
	}
	catch (const sserialize::OutOfBoundsException & e) {
		return T_CQR_TYPE();
	}
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::fromRegionStoreId(uint32_t storeId) const {
	if (storeId < m_gh.regionSize()) {
		uint32_t regionCellPtr = m_gh.regionCellIdxPtr(m_gh.storeIdToGhId(storeId));
		return T_CQR_TYPE(m_idxStore.at(regionCellPtr), m_gh, m_idxStore);
	}
	return T_CQR_TYPE();
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::fromCellId(uint32_t cellId) const {
	if (cellId < m_gh.cellSize()) {
		return T_CQR_TYPE(true, cellId, m_gh, m_idxStore, 0);
	}
	return T_CQR_TYPE();
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::cqrFromRect(const sserialize::spatial::GeoRect & rect) const {
	T_CQR_TYPE retCQR;
	m_gh.cqr(idxStore(), rect, retCQR);
	return retCQR;
}

template<typename T_CQR_TYPE>
T_CQR_TYPE CellTextCompleter::cqrBetween(const sserialize::spatial::GeoPoint& start, const sserialize::spatial::GeoPoint& end, double radius) const {
	sserialize::ItemIndex idx( m_ra.cellsBetween(start, end, radius) );
	if (idx.size()) {
		return T_CQR_TYPE(idx, m_gh, m_idxStore);
	}
	return T_CQR_TYPE();
}

template<typename T_CQR_TYPE, typename T_GEOPOINT_ITERATOR>
T_CQR_TYPE CellTextCompleter::cqrAlongPath(double radius, const T_GEOPOINT_ITERATOR & begin, const T_GEOPOINT_ITERATOR & end) const {
	sserialize::ItemIndex idx( m_ra.cellsAlongPath(radius, begin, end) );
	if (idx.size()) {
		return T_CQR_TYPE(idx, m_gh, m_idxStore);
	}
	return T_CQR_TYPE();
}

}//end namespace detail

class CellTextCompleter {
public:
// 	typedef detail::CellTextCompleter Node;
	typedef detail::CellTextCompleter::Payload Payload;
	typedef detail::CellTextCompleter::Trie Trie;
	typedef detail::CellTextCompleter::FlatTrieType FlatTrieType;
	typedef detail::CellTextCompleter::TrieType TrieType;
	typedef detail::CellTextCompleter::TrieTypeMarker TrieTypeMarker;
private:
	sserialize::RCPtrWrapper<detail::CellTextCompleter> m_priv;
private:
	inline detail::CellTextCompleter* priv() { return m_priv.priv(); }
	inline const detail::CellTextCompleter* priv() const { return m_priv.priv(); }
public:
	CellTextCompleter() : m_priv(new detail::CellTextCompleter()) {}
	CellTextCompleter(detail::CellTextCompleter * priv) : m_priv(priv) {}
	CellTextCompleter(const sserialize::RCPtrWrapper<detail::CellTextCompleter> & priv) : m_priv(priv) {}
	CellTextCompleter(const sserialize::UByteArrayAdapter & d, const sserialize::Static::ItemIndexStore & idxStore, const sserialize::Static::spatial::GeoHierarchy & gh, const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement & ra) :
	m_priv(new detail::CellTextCompleter(d, idxStore, gh, ra))
	{}
	virtual ~CellTextCompleter() {}
	const Trie & trie() const {
		return priv()->trie();
	}
	Trie & trie() {
		return priv()->trie();
	}
	
	inline Payload::Type typeFromCompletion(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const {
		return priv()->typeFromCompletion(qstr, qt);
	}
	
	inline bool count(const std::string::const_iterator & begin, const std::string::const_iterator & end) const {
		return priv()->count(begin, end);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE complete(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const {
		return priv()->complete<T_CQR_TYPE>(qstr, qt);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE regions(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const {
		return priv()->regions<T_CQR_TYPE>(qstr, qt);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE items(const std::string & qstr, const sserialize::StringCompleter::QuerryType qt) const {
		return priv()->items<T_CQR_TYPE>(qstr, qt);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE cqrFromRegionStoreId(uint32_t id) const {
		return priv()->fromRegionStoreId<T_CQR_TYPE>(id);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE cqrFromCellId(uint32_t id) const {
		return priv()->fromCellId<T_CQR_TYPE>(id);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE cqrFromRect(const sserialize::spatial::GeoRect & rect) const {
		return priv()->cqrFromRect<T_CQR_TYPE>(rect);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult>
	inline T_CQR_TYPE cqrBetween(const sserialize::spatial::GeoPoint & start, const sserialize::spatial::GeoPoint & end, double radius) const {
		return priv()->cqrBetween<T_CQR_TYPE>(start, end, radius);
	}
	
	template<typename T_CQR_TYPE = sserialize::CellQueryResult, typename T_GEOPOINT_ITERATOR>
	inline T_CQR_TYPE cqrAlongPath(double radius, const T_GEOPOINT_ITERATOR & begin, const T_GEOPOINT_ITERATOR & end) const {
		return priv()->cqrAlongPath<T_CQR_TYPE>(radius, begin, end);
	}
	
	inline sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const {
		return priv()->getSupportedQuerries();
	}
	
	inline const sserialize::Static::ItemIndexStore & idxStore() const {
		return priv()->idxStore();
	}
	
	inline std::ostream & printStats(std::ostream & out) const {
		return priv()->printStats(out);
	}
};


class CellTextCompleterUnclustered: public sserialize::StringCompleterPrivate {
private:
	CellTextCompleter m_cellTextCompleter;
	sserialize::Static::spatial::GeoHierarchy m_gh;
public:
	CellTextCompleterUnclustered() {}
	CellTextCompleterUnclustered(const CellTextCompleter & c, const sserialize::Static::spatial::GeoHierarchy & gh) : m_cellTextCompleter(c), m_gh(gh) {}
	virtual ~CellTextCompleterUnclustered() {}
	virtual sserialize::ItemIndex complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	
	virtual sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;

	virtual sserialize::ItemIndex indexFromId(uint32_t idxId) const;

	virtual std::ostream& printStats(std::ostream& out) const;
	
	virtual std::string getName() const;
	
	const CellTextCompleter & cellTextCompleter() const { return m_cellTextCompleter; }
};

}}

#endif