#ifndef LIBOSCAR_OSM_KEY_VALUE_OBJECT_STORE_H
#define LIBOSCAR_OSM_KEY_VALUE_OBJECT_STORE_H
#include <sserialize/Static/KeyValueObjectStore.h>
#include <sserialize/Static/GeoShape.h>
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/search/ItemIndexIteratorGeoDB.h>
#include <sserialize/containers/ItemIndexPrivates/ItemIndexPrivateSimple.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/Static/TriangulationGeoHierarchyArrangement.h>
#include <sserialize/Static/TracGraph.h>
#include <liboscar/constants.h>
#include <liboscar/OsmIdType.h>
#define LIBOSCAR_OSM_KEY_VALUE_OBJECT_STORE_VERSION 6

namespace liboscar {
namespace Static {

/** Storage layout
  *
  * v2: isInCell for Payload, GeoHierarchy
  * v3: add support for reorder map
  * v4: add support for OsmIdType (currently implemented)
  * v5: add support for region triangulation
  * v6: add support for region arrangement cell graph
  * v?: add support for relations by storing their data in GeoShape
  *
  * {
  *   VERSION             u8
  *   PAYLOADS            sserialize::Static::Array<Payload>
  *   REORDER_MAP         sserialize::BoundedCompactUintArray
  *   KeyValues           sserialize::Static::KeyValueObjectStore
  *   GeoHierarchy        sserialize::Static::GeoHierarchy
  *   RegionArrangement   sserialize::Static::spatial::TriangulationGeoHierarchyArrangement
  *   RACellGraph         sserialize::Static::spatial::TRACGraph;
  * }
  *
  *Payload (v3)
  *--------------------------------------------
  *osmId|score|Shape   |cells
  *--------------------------------------------
  *vs64 |vu32 |GeoShape|BoundedCompactUintArray
  *--------------------------------------------
  *
  *Payload (v?)
  *-------------------------------------------------------------------------------------------------
  *OsmIdType                    |score|Shape OR RelationReferences        |cells
  *-------------------------------------------------------------------------------------------------
  *vs64(osmid:61,type:2,shape:1)|vu32 |GeoShape OR BoundedCompactUintArray|BoundedCompactUintArray
  *-------------------------------------------------------------------------------------------------
  */

class OsmKeyValueObjectStorePrivate;
class OsmKeyValueObjectStoreItem;

class OsmKeyValueObjectStorePayload {
	OsmIdType m_osmId;
	uint32_t m_score;
	sserialize::Static::spatial::GeoShape m_shape;
	sserialize::BoundedCompactUintArray m_cells;
public:
	OsmKeyValueObjectStorePayload() : m_score(0) {}
	OsmKeyValueObjectStorePayload(sserialize::UByteArrayAdapter data) :
		m_osmId(data.resetGetPtr().getVlPackedInt64()),
		m_score(data.getVlPackedUint32()),
		m_shape(data.shrinkToGetPtr()),
		m_cells(data + m_shape.getSizeInBytes())
	{}
	virtual ~OsmKeyValueObjectStorePayload() {}
	inline const sserialize::Static::spatial::GeoShape & shape() const { return m_shape; }
	inline int64_t osmId() const { return m_osmId.id(); }
	liboscar::OsmItemTypes type() const { return (liboscar::OsmItemTypes) m_osmId.type(); }
	inline uint32_t score() const { return m_score; }
	inline sserialize::BoundedCompactUintArray cells() const { return m_cells; }
};
  
class OsmKeyValueObjectStore: public sserialize::RCWrapper<OsmKeyValueObjectStorePrivate> {
public:
	typedef sserialize::RCWrapper<OsmKeyValueObjectStorePrivate> MyBaseClass;
	typedef OsmKeyValueObjectStoreItem Item;
	typedef sserialize::Static::KeyValueObjectStore KeyValueObjectStore;
	typedef KeyValueObjectStore::ValueStringTable ValueStringTable;
	typedef KeyValueObjectStore::KeyStringTable KeyStringTable;
	typedef sserialize::ReadOnlyAtStlIterator<const OsmKeyValueObjectStore*, OsmKeyValueObjectStore::Item> const_iterator;
public:
	static constexpr uint32_t npos = 0xFFFFFFFF;
private:
	OsmKeyValueObjectStore(OsmKeyValueObjectStorePrivate * data);
public:
	OsmKeyValueObjectStore();
	OsmKeyValueObjectStore(const OsmKeyValueObjectStore & other);
	OsmKeyValueObjectStore(const sserialize::UByteArrayAdapter & data);
	virtual ~OsmKeyValueObjectStore();
	OsmKeyValueObjectStore & operator=(const OsmKeyValueObjectStore & other);
	uint32_t size() const;
	const KeyValueObjectStore & kvStore() const;
	const KeyStringTable & keyStringTable() const;
	const ValueStringTable & valueStringTable() const;
	const sserialize::Static::spatial::GeoHierarchy & geoHierarchy() const;
	const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement & regionArrangement() const;
	const sserialize::Static::spatial::TracGraph & cellGraph() const;
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	
	const_iterator begin() const;
	const_iterator cbegin() const;
	const_iterator end() const;
	const_iterator cend() const;
	
	uint32_t toInternalId(uint32_t itemId) const;
	
	Item at(uint32_t pos) const;
	
	bool match(uint32_t pos, const std::pair< std::string, sserialize::StringCompleter::QuerryType > & querry) const;
	sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;

	sserialize::ItemIndex complete(const sserialize::spatial::GeoRect & rect, bool approximate) const;
	sserialize::ItemIndex filter(const sserialize::spatial::GeoRect & rect, bool approximate, const sserialize::ItemIndex & partner) const;
	sserialize::ItemIndexIterator filter(const sserialize::spatial::GeoRect & rect, bool approximate, const sserialize::ItemIndexIterator & partner) const;
	
	///expensive operation
	sserialize::spatial::GeoRect boundary() const;
	
	/** checks if any point of the item lies within boundary */
	bool match(uint32_t itemPos, const sserialize::spatial::GeoRect & boundary) const;
	sserialize::spatial::GeoShapeType geoShapeType(uint32_t itemPos) const;
	uint32_t geoPointCount(uint32_t itemPos) const;
	sserialize::Static::spatial::GeoPoint geoPointAt(uint32_t itemPos, uint32_t pos) const;
	sserialize::Static::spatial::GeoShape geoShape(uint32_t itemPos) const;
	
	int64_t osmId(uint32_t itemPos) const;
	uint32_t score(uint32_t itemPos) const;
	bool isRegion(uint32_t itemPos) const;
	sserialize::BoundedCompactUintArray cells(uint32_t itemPos) const;
	OsmKeyValueObjectStorePayload payload(uint32_t itemPos) const;
	
	std::ostream & printStats(std::ostream & out) const;
	
	bool sanityCheck() const;
	
public: //dummy functions
	sserialize::ItemIndex complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	sserialize::ItemIndexIterator partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	sserialize::ItemIndexIterator partialComplete(const sserialize::spatial::GeoRect & rect, bool approximate) const;
	sserialize::ItemIndex select(const std::unordered_set<uint32_t> & strIds) const;
	sserialize::ItemIndex select(const sserialize::ItemIndex & strIds) const;
	///matches the keyIds (Fgst does not use these!
	bool match(uint32_t pos, const sserialize::ItemIndex strIds) const;
	std::string getName() const;
};

class OsmKeyValueObjectStoreItem: public sserialize::Static::KeyValueObjectStoreItem {
private:
	typedef sserialize::Static::KeyValueObjectStoreItem MyBaseClass;
	typedef OsmKeyValueObjectStorePayload PayloadType;
private:
	OsmKeyValueObjectStore m_db;
	uint32_t m_id;
public:
	OsmKeyValueObjectStoreItem();
	OsmKeyValueObjectStoreItem(uint32_t id, const OsmKeyValueObjectStore & db, const sserialize::Static::KeyValueObjectStoreItem & kvoi);
	virtual ~OsmKeyValueObjectStoreItem();
	inline bool valid() const { return m_id != OsmKeyValueObjectStore::npos && db().size() > m_id; }
	inline const OsmKeyValueObjectStore & db() const { return this->m_db; }
	inline OsmKeyValueObjectStore & db() { return this->m_db; }
	inline uint32_t id() const { return m_id; }
	inline uint32_t internalId() const { return m_db.toInternalId(m_id); }
	inline int64_t osmId() const { return m_db.osmId(id()); }
	inline uint32_t score() const { return m_db.score(id()); }
	inline bool isRegion() const { return db().isRegion(id()); }
	inline sserialize::BoundedCompactUintArray cells() const { return m_db.cells(id()); }

	inline uint32_t strCount() const { return MyBaseClass::size(); }
	uint32_t strIdAt(uint32_t pos) const { return MyBaseClass::keyId(pos); }
	std::string strAt(uint32_t pos) const { return MyBaseClass::key(pos) + "=" + MyBaseClass::value(pos); }
	bool match(const std::string & str, sserialize::StringCompleter::QuerryType qt) const {
		return m_db.match(m_id, std::pair<std::string, sserialize::StringCompleter::QuerryType>( str, qt) );
	}
	bool hasAnyStrIdOf(const std::unordered_set<uint32_t> & testSet) const {
		uint32_t sc = strCount();
		for(size_t i = 0; i < sc; ++i) {
			if (testSet.count(strIdAt(i)) > 0)
				return true;
		}
		return false;
	}
	bool hasAnyStrIdOf(const sserialize::ItemIndex & testSet) const {
		uint32_t sc = strCount();
		for(size_t i = 0; i < sc; ++i) {
			if (testSet.count(strIdAt(i)) > 0)
				return true;
		}
		return false;
	}
	
	inline bool match(const sserialize::spatial::GeoRect & boundary) const { return m_db.match(m_id,boundary); }
	inline sserialize::spatial::GeoShapeType geoShapeType() const { return db().geoShapeType(id()); }
	inline sserialize::Static::spatial::GeoShape geoShape() const { return db().geoShape(id()); }
	inline uint32_t geoPointCount() const { return db().geoPointCount(id()); }
	inline sserialize::Static::spatial::GeoPoint geoPointAt(uint32_t pos) const { return db().geoPointAt(id(), pos); }
	inline OsmKeyValueObjectStorePayload payload() const { return m_db.payload(id()); }
	
	void print(std::ostream& out, bool withGeoPoints) const;
	void dump();
	std::string getAllStrings() const;
	inline std::string getAllPoIAsString() const { return std::string("no support");}
	std::string getAllGeoPointsAsString() const;
};

class OsmKeyValueObjectStorePrivate: public  sserialize::RefCountObject {
public:
	typedef sserialize::Static::KeyValueObjectStore::ValueStringTable ValueStringTable;
	typedef sserialize::Static::KeyValueObjectStore::KeyStringTable KeyStringTable;
	typedef sserialize::Static::KeyValueObjectStore KeyValueObjectStore;
private:
	sserialize::Static::Array<OsmKeyValueObjectStorePayload> m_payload;
	sserialize::BoundedCompactUintArray m_idToInternalId;
	KeyValueObjectStore m_kv; //objects are ordered according to their internalId
	sserialize::Static::spatial::GeoHierarchy m_gh; //storeId is the remapped id, not the internalId
	sserialize::Static::spatial::TriangulationGeoHierarchyArrangement m_ra;
	sserialize::Static::spatial::TracGraph m_cg;
	uint32_t m_size;
public:
	OsmKeyValueObjectStorePrivate(const sserialize::UByteArrayAdapter & data);
	OsmKeyValueObjectStorePrivate();
	virtual ~OsmKeyValueObjectStorePrivate();
	uint32_t size() const;
	sserialize::UByteArrayAdapter::OffsetType getSizeInBytes() const;
	inline uint32_t toInternalId(uint32_t pos) const { return m_idToInternalId.at(pos); }
	inline const KeyValueObjectStore & kvStore() const { return m_kv; }
	inline const KeyStringTable & keyStringTable() const { return m_kv.keyStringTable(); }
	inline const ValueStringTable & valueStringTable() const { return m_kv.valueStringTable(); }
	inline const sserialize::Static::spatial::GeoHierarchy & geoHierarchy() const { return m_gh; }
	inline const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement & regionArrangement() const { return m_ra; }
	inline const sserialize::Static::spatial::TracGraph & cellGraph() const { return m_cg; }
	sserialize::Static::KeyValueObjectStoreItem kvItem(uint32_t pos) const;
	
	sserialize::ItemIndex complete(const sserialize::spatial::GeoRect & rect) const;
	sserialize::ItemIndex filter(const sserialize::spatial::GeoRect & rect, bool approximate, const sserialize::ItemIndex & partner) const;
	
	bool match(uint32_t pos, const std::pair< std::string, sserialize::StringCompleter::QuerryType > & querry) const;
	sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;
	
	/** checks if any point of the item lies within boundary */
	bool match(uint32_t itemPos, const sserialize::spatial::GeoRect & boundary) const;
	
	sserialize::spatial::GeoShapeType geoShapeType(uint32_t itemPos) const;
	uint32_t geoPointCount(uint32_t itemPos) const;
	sserialize::Static::spatial::GeoPoint geoPointAt(uint32_t itemPos, uint32_t pos) const;
	
	sserialize::Static::spatial::GeoShape geoShapeAt(uint32_t itemPos) const;
	
	int64_t osmId(uint32_t itemPos) const;
	uint32_t score(uint32_t itemPos) const;
	bool isRegion(uint32_t itemPos) const;
	sserialize::BoundedCompactUintArray cells(uint32_t itemPos) const;
	OsmKeyValueObjectStorePayload payload(uint32_t itemPos) const;

	std::ostream & printStats(std::ostream & out) const;
};


}}//end namespace



#endif