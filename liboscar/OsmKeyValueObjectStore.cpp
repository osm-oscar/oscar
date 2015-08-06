#include "OsmKeyValueObjectStore.h"
#include <sserialize/utility/exceptions.h>
#include <sserialize/Static/GeoWay.h>
#include <sserialize/Static/GeoMultiPolygon.h>


namespace liboscar {
namespace Static {

OsmKeyValueObjectStore::OsmKeyValueObjectStore(OsmKeyValueObjectStorePrivate * data): MyBaseClass(data) {}

OsmKeyValueObjectStore::OsmKeyValueObjectStore() : 
MyBaseClass( new OsmKeyValueObjectStorePrivate() )
{}

OsmKeyValueObjectStore::OsmKeyValueObjectStore(const OsmKeyValueObjectStore & other) : 
MyBaseClass( other )
{}

OsmKeyValueObjectStore::OsmKeyValueObjectStore(const sserialize::UByteArrayAdapter & data) :
MyBaseClass( new OsmKeyValueObjectStorePrivate(data) )
{}

OsmKeyValueObjectStore::~OsmKeyValueObjectStore() {}


OsmKeyValueObjectStore & OsmKeyValueObjectStore::operator=(const OsmKeyValueObjectStore & other) {
	MyBaseClass::operator=(other);
	return *this;
}

uint32_t OsmKeyValueObjectStore::size() const {
	return priv()->size();
}

const OsmKeyValueObjectStore::KeyStringTable &OsmKeyValueObjectStore::keyStringTable() const {
	return priv()->keyStringTable();
}

const OsmKeyValueObjectStore::ValueStringTable & OsmKeyValueObjectStore::valueStringTable() const {
	return priv()->valueStringTable();
}

const sserialize::Static::spatial::GeoHierarchy & OsmKeyValueObjectStore::geoHierarchy() const {
	return priv()->geoHierarchy();
}

const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement& OsmKeyValueObjectStore::regionArrangement() const {
	return priv()->regionArrangement();
}

const sserialize::Static::spatial::TracGraph& OsmKeyValueObjectStore::cellGraph() const {
	return priv()->cellGraph();
}

sserialize::UByteArrayAdapter::OffsetType OsmKeyValueObjectStore::getSizeInBytes() const {
	return priv()->getSizeInBytes();
}

uint32_t OsmKeyValueObjectStore::toInternalId(uint32_t itemId) const {
	return priv()->toInternalId(itemId);
}

OsmKeyValueObjectStoreItem OsmKeyValueObjectStore::at(uint32_t pos) const {
	if (pos < size()) {
		return OsmKeyValueObjectStoreItem(pos, *this, priv()->kvItem(pos));
	}
	else {
		return OsmKeyValueObjectStoreItem();
	}
}

bool OsmKeyValueObjectStore::match(uint32_t pos, const std::pair< std::string, sserialize::StringCompleter::QuerryType >& querry) const {
	return priv()->match(pos, querry);
}

sserialize::StringCompleter::SupportedQuerries OsmKeyValueObjectStore::getSupportedQuerries() const {
	return priv()->getSupportedQuerries();
}


sserialize::ItemIndex OsmKeyValueObjectStore::complete(const sserialize::spatial::GeoRect & rect, bool /*approximate*/) const {
	return priv()->complete(rect);
}

sserialize::ItemIndex OsmKeyValueObjectStore::filter(const sserialize::spatial::GeoRect & rect, bool approximate, const sserialize::ItemIndex & partner) const {
	return priv()->filter(rect, approximate, partner);
}

sserialize::ItemIndexIterator OsmKeyValueObjectStore::filter(const sserialize::spatial::GeoRect & rect, bool /*approximate*/, const sserialize::ItemIndexIterator & partner) const {
	sserialize::ItemIndexIteratorGeoDB< OsmKeyValueObjectStore > * dbIt = new sserialize::ItemIndexIteratorGeoDB< OsmKeyValueObjectStore >(*this, rect, partner);
	return sserialize::ItemIndexIterator( dbIt );
}

/** checks if any point of the item lies within boundary */
bool OsmKeyValueObjectStore::match(uint32_t itemPos, const sserialize::spatial::GeoRect & boundary) const {
	return priv()->match(itemPos, boundary);
}

sserialize::spatial::GeoShapeType OsmKeyValueObjectStore::geoShapeType(uint32_t itemPos) const {
	return priv()->geoShapeType(itemPos);
}

uint32_t OsmKeyValueObjectStore::geoPointCount(uint32_t itemPos) const {
	return priv()->geoPointCount(itemPos);
}

sserialize::Static::spatial::GeoPoint OsmKeyValueObjectStore::geoPointAt(uint32_t itemPos, uint32_t pos) const {
	return priv()->geoPointAt(itemPos, pos);
}

sserialize::Static::spatial::GeoShape OsmKeyValueObjectStore::geoShape(uint32_t itemPos) const {
	return priv()->geoShapeAt(itemPos);	
}

int64_t OsmKeyValueObjectStore::osmId(uint32_t itemPos) const {
	return priv()->osmId(itemPos);
}

uint32_t OsmKeyValueObjectStore::score(uint32_t itemPos) const {
	return priv()->score(itemPos);
}

sserialize::BoundedCompactUintArray OsmKeyValueObjectStore::cells(uint32_t itemPos) const {
	return priv()->cells(itemPos);
}

OsmKeyValueObjectStorePayload OsmKeyValueObjectStore::payload(uint32_t itemPos) const {
	return priv()->payload(itemPos);
}

sserialize::spatial::GeoRect OsmKeyValueObjectStore::boundary() const {
	if (!size())
		return sserialize::spatial::GeoRect();
	sserialize::spatial::GeoRect rect;
	size_t i = 0;

	//find the first valid rect
	for(uint32_t s = size(); i < s; ++i) {
		sserialize::Static::spatial::GeoShape shape = geoShape(i);
		if (shape.type() != sserialize::spatial::GS_NONE) {
			rect = shape.boundary();
			break;
		}
	}
	
	for(uint32_t s = size(); i < s; ++i) {
		sserialize::Static::spatial::GeoShape shape = geoShape(i);
		if (shape.type() != sserialize::spatial::GS_NONE) {
			rect.enlarge(shape.boundary());
		}
	}
	return rect;
}

sserialize::ItemIndex OsmKeyValueObjectStore::complete(const std::string & /*str*/, sserialize::StringCompleter::QuerryType /*qtype*/) const {
	return sserialize::ItemIndex();
}

sserialize::ItemIndexIterator OsmKeyValueObjectStore::partialComplete(const std::string & /*str*/, sserialize::StringCompleter::QuerryType /*qtype*/) const {
	return sserialize::ItemIndexIterator();
}

sserialize::ItemIndex OsmKeyValueObjectStore::select(const std::unordered_set<uint32_t> & /*strIds*/) const {
	return sserialize::ItemIndex();
}

bool OsmKeyValueObjectStore::match(uint32_t pos, const sserialize::ItemIndex strIds) const {
	Item item = at(pos);
	for(uint32_t i = 0, s = item.size(); i < s; ++i) {
		if (strIds.find(item.keyId(i)) > 0)
			return true;
	}
	return false;
}

sserialize::ItemIndex OsmKeyValueObjectStore::select(const sserialize::ItemIndex & strIds) const {
	std::vector<uint32_t> res;
	for(uint32_t i = 0, s = size(); i < s; ++i) {
		if (match(i, strIds))
			res.push_back(i);
	}
	return sserialize::ItemIndex::absorb(res);
}


sserialize::ItemIndexIterator OsmKeyValueObjectStore::partialComplete(const sserialize::spatial::GeoRect & rect, bool /*approximate*/) const {
	sserialize::ItemIndexIteratorPrivate * rangeIt = new sserialize::ItemIndexIteratorPrivateFixedRange(0, size());
	sserialize::ItemIndexIteratorPrivate * dbIt = new sserialize::ItemIndexIteratorGeoDB< OsmKeyValueObjectStore >(*this, rect, sserialize::ItemIndexIterator(rangeIt));
	return sserialize::ItemIndexIterator( dbIt );
}

std::string OsmKeyValueObjectStore::getName() const {
	return std::string("OsmKeyValueObjectStore");
}

std::ostream & OsmKeyValueObjectStore::printStats(std::ostream & out) const {
	return priv()->printStats(out);
}

bool OsmKeyValueObjectStore::sanityCheck() const {
	uint32_t keysSize = keyStringTable().size();
	uint32_t valuesSize = valueStringTable().size();

	for(uint32_t i = 0, s = size(); i < s; ++i) {
		Item item(at(i));
		for(uint32_t j = 0, sj = item.size(); j < sj; ++j) {
			if (item.keyId(j) >= keysSize) {
				std::cout << "Invalid keyid=" << item.keyId(j) << ">=" << keysSize << " in item " << i << std::endl;
// 				return false;
			}
			if (item.valueId(j) >= valuesSize) {
				std::cout << "Invalid valueid=" << item.valueId(j) << ">=" << valuesSize << " in item " << i << std::endl;
// 				return false;
			}
		}
	}
	return true;
}


/* OsmKeyValueObjectStoreItem begin */

OsmKeyValueObjectStoreItem::OsmKeyValueObjectStoreItem(): m_id(OsmKeyValueObjectStore::npos) {}

OsmKeyValueObjectStoreItem::OsmKeyValueObjectStoreItem(uint32_t id, const OsmKeyValueObjectStore & db, const sserialize::Static::KeyValueObjectStoreItem & kvoi) :
MyBaseClass(kvoi),
m_db(db),
m_id(id)
{}

void OsmKeyValueObjectStoreItem::print(std::ostream& out, bool withGeoPoints) const {
	PayloadType pl(payload());
	sserialize::Static::spatial::GeoShape shape = geoShape();
	out << "[id=" << id() << ", osmId=" << pl.osmId() << ", score=" << pl.score() << ", kv=(" << getAllStrings() << ")";
	out << ", boundary=" << shape.boundary();
	sserialize::BoundedCompactUintArray myCells = cells();
	out << ", cells={";
	for(uint32_t i(0), s(myCells.size()); i < s; ++i) {
		out << myCells.at(i) << ", ";
	}
	out << "}";
	out << ", geometry={";
	if (!withGeoPoints) {
		out << shape.boundary() << "}";
	}
	else {
		shape.priv()->asString(out);
	}
	out << "]";
}

std::string OsmKeyValueObjectStoreItem::getAllStrings() const {
	uint8_t strC = strCount();
	switch (strC) {
	case 0:
		return std::string();
	case 1:
		return strAt(0);
	default:
		{
			std::string str = "";
			uint8_t count = 0;
			while (true) {
				str += strAt(count);
				count++;
				if (count < strC)
					str += ", ";
				else
					return str;
			}
			return str;
		}
	}
}

std::string OsmKeyValueObjectStoreItem::getAllGeoPointsAsString() const {
	std::stringstream ss;
	sserialize::Static::spatial::GeoShape shape = geoShape();
	shape.priv()->asString(ss);
	return ss.str();
}

OsmKeyValueObjectStoreItem::~OsmKeyValueObjectStoreItem() {}


/* OsmKeyValueObjectStoreItem end */

/* OsmKeyValueObjectStorePrivate begin */

OsmKeyValueObjectStorePrivate::OsmKeyValueObjectStorePrivate(const sserialize::UByteArrayAdapter & data) :
m_payload(data+1),
m_idToInternalId(data+(1+m_payload.getSizeInBytes())),
m_kv(data+(1+m_payload.getSizeInBytes()+m_idToInternalId.getSizeInBytes())),
m_gh(data+(1+m_payload.getSizeInBytes()+m_idToInternalId.getSizeInBytes()+m_kv.getSizeInBytes())),
m_ra(data+(1+m_payload.getSizeInBytes()+m_idToInternalId.getSizeInBytes()+m_kv.getSizeInBytes()+m_gh.getSizeInBytes())),
m_cg(data+(1+m_payload.getSizeInBytes()+m_idToInternalId.getSizeInBytes()+m_kv.getSizeInBytes()+m_gh.getSizeInBytes()+m_ra.getSizeInBytes()))
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(LIBOSCAR_OSM_KEY_VALUE_OBJECT_STORE_VERSION, data.at(0), "OsmKeyValueObjectStore")
	if (m_payload.size() != m_kv.size())
		throw sserialize::CorruptDataException("OsmKeyValueObjectStore: m_shapes.size() != m_kv.size()");
	if (m_gh.regionSize() > m_kv.size())
		throw sserialize::CorruptDataException("OsmKeyValueObjectStore: m_gh.size() != m_kv.size()");
	if (m_kv.size() != m_idToInternalId.size()) {
		throw sserialize::CorruptDataException("OsmKeyValueObjectStore: m_kv.size() != m_idToInternalId.size()");
	}
	if (m_ra.cellCount() != m_gh.cellSize()) {
		throw sserialize::CorruptDataException("OsmKeyValueObjectStore: m_ra.cellCount() != m_gh.cellSize()");
	}
	if (m_ra.cellCount() != m_cg.size()) {
		throw sserialize::CorruptDataException("OsmKeyValueObjectStore: m_ra.cellCount() != m_cg.size()");
	}
	m_size = m_payload.size();
}

OsmKeyValueObjectStorePrivate::OsmKeyValueObjectStorePrivate() : m_size(0) {}

OsmKeyValueObjectStorePrivate::~OsmKeyValueObjectStorePrivate() {}

uint32_t OsmKeyValueObjectStorePrivate::size() const { return m_size; }

sserialize::UByteArrayAdapter::OffsetType OsmKeyValueObjectStorePrivate::getSizeInBytes() const {
	return 1 + m_payload.getSizeInBytes() + m_idToInternalId.getSizeInBytes() + m_kv.getSizeInBytes() + m_gh.getSizeInBytes() + m_ra.getSizeInBytes();
}

sserialize::Static::KeyValueObjectStoreItem OsmKeyValueObjectStorePrivate::kvItem(uint32_t pos) const {
	return m_kv.at(toInternalId(pos));
}

sserialize::StringCompleter::SupportedQuerries OsmKeyValueObjectStorePrivate::getSupportedQuerries() const {
	return m_kv.getSupportedQuerries();
}

/** checks if any point of the item lies within boundary */
bool OsmKeyValueObjectStorePrivate::match(uint32_t itemPos, const sserialize::spatial::GeoRect & boundary) const {
	if (itemPos >= size())
		return false;
	return payload(itemPos).shape().intersects(boundary);
}

sserialize::spatial::GeoShapeType OsmKeyValueObjectStorePrivate::geoShapeType(uint32_t itemPos) const {
	return payload(itemPos).shape().type();
}
uint32_t OsmKeyValueObjectStorePrivate::geoPointCount(uint32_t itemPos) const {
	return payload(itemPos).shape().size();
}
sserialize::Static::spatial::GeoPoint OsmKeyValueObjectStorePrivate::geoPointAt(uint32_t itemPos, uint32_t pos) const {
	return static_cast<sserialize::Static::spatial::GeoWay*>( payload(itemPos).shape().priv().get())->points().at(pos);
}

sserialize::Static::spatial::GeoShape OsmKeyValueObjectStorePrivate::geoShapeAt(uint32_t itemPos) const {
	return payload(itemPos).shape();
}

int64_t OsmKeyValueObjectStorePrivate::osmId(uint32_t itemPos) const {
	return payload(itemPos).osmId();
}

uint32_t OsmKeyValueObjectStorePrivate::score(uint32_t itemPos) const {
	return payload(itemPos).score();
}

sserialize::BoundedCompactUintArray OsmKeyValueObjectStorePrivate::cells(uint32_t itemPos) const {
	return payload(itemPos).cells();
}

OsmKeyValueObjectStorePayload OsmKeyValueObjectStorePrivate::payload(uint32_t itemPos) const {
	return m_payload.at(toInternalId(itemPos));
}

sserialize::ItemIndex OsmKeyValueObjectStorePrivate::complete(const sserialize::spatial::GeoRect & rect) const {
	size_t s = size();
	sserialize::UByteArrayAdapter cache( sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY) );
	sserialize::ItemIndexPrivateSimpleCreator creator(0, s, s, cache);
	for(size_t i = 0; i < s; ++i) {
		if (match(i, rect))
			creator.push_back(i);
	}
	creator.flush();
	return creator.getIndex();
}

sserialize::ItemIndex OsmKeyValueObjectStorePrivate::filter(const sserialize::spatial::GeoRect & rect, bool /*approximate*/, const sserialize::ItemIndex & partner) const {
	if (!partner.size())
		return partner;
	sserialize::UByteArrayAdapter cache( sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY) );
	sserialize::ItemIndexPrivateSimpleCreator creator(partner.front(), partner.back(), partner.size(), cache);
	for(size_t i = 0; i < partner.size(); i++) {
		uint32_t itemId = partner.at(i);
		if (match(itemId, rect))
			creator.push_back(itemId);
	}
	creator.flush();
	return creator.getIndex();
}

bool OsmKeyValueObjectStorePrivate::match(uint32_t pos, const std::pair< std::string, sserialize::StringCompleter::QuerryType >& querry) const {
	return m_kv.matchValues(pos, querry);
}

std::ostream & OsmKeyValueObjectStorePrivate::printStats(std::ostream & out) const {
	out << "OsmKeyValueObjectStore::printStats -- BEGIN" << std::endl;
	out << "Size of payload: " << m_payload.getSizeInBytes() << "(" << ((double)m_payload.getSizeInBytes())/getSizeInBytes()*100.0 << "%)" << std::endl;
	uint64_t geoShapeSizeInBytes = 0;
	for(uint32_t i(0), s(size()); i < s; ++i) {
		 geoShapeSizeInBytes += geoShapeAt(i).getSizeInBytes();
	}
	out << "Size of geoshapes: " << geoShapeSizeInBytes << "(" << (double)geoShapeSizeInBytes/getSizeInBytes()*100.0 << "%)" << std::endl;
	uint64_t totalKeyValueStringSize = 0;
	{
		const KeyStringTable & kst = keyStringTable();
		const ValueStringTable & vst = valueStringTable();
		for(uint32_t i(0), s(size()); i < s; ++i) {
			sserialize::Static::KeyValueObjectStore::Item kvitem(m_kv.at(i));
			for(uint32_t j(0), js(kvitem.size()); j < js; ++j) {
				uint32_t keyStrSize = kst.strSize(kvitem.keyId(j));
				uint32_t valueStrSize = vst.strSize(kvitem.valueId(j));
				totalKeyValueStringSize += keyStrSize + valueStrSize;
			}
		}
	}
	out << "Total concatenated size of key:value strings: " << totalKeyValueStringSize << "\n";
	m_kv.printStats(out);
	uint64_t totalRegionPoints = 0;
	uint64_t totalGeoPoints = 0;
	uint64_t totalRegionPolygons = 0;
	{
		uint64_t tmp = 0;
		uint64_t tmp2 = 0;
		auto myf = [&tmp, &tmp2](const sserialize::Static::spatial::GeoShape & gs) {
			switch (gs.type()) {
				case sserialize::spatial::GS_POINT:
					tmp += 1;
					break;
				case sserialize::spatial::GS_POLYGON:
					++tmp2;
				case sserialize::spatial::GS_WAY:
					tmp += gs.get<sserialize::Static::spatial::GeoWay>()->size();
					break;
				case sserialize::spatial::GS_MULTI_POLYGON:
				{
					const sserialize::Static::spatial::GeoMultiPolygon * gmp = gs.get<sserialize::Static::spatial::GeoMultiPolygon>();
					for(uint32_t j(0), js(gmp->outerPolygons().size()); j < js; ++j) {
						tmp += gmp->outerPolygons().at(j).size();
					}
					for(uint32_t j(0), js(gmp->innerPolygons().size()); j < js; ++j) {
						tmp += gmp->innerPolygons().at(j).size();
					}
					tmp2 += gmp->outerPolygons().size() + gmp->innerPolygons().size();
					break;
				}
				default:
					break;
			}
		};
		uint32_t i(0);
		for(uint32_t s(m_gh.regionSize()); i < s; ++i) {
			myf(geoShapeAt(i));
		}
		totalRegionPoints = tmp;
		totalRegionPolygons = tmp2;
		for(uint32_t s(size()); i < s; ++i) {
			myf(geoShapeAt(i));
		}
		totalGeoPoints = tmp;
	}
	out << "Total number of polygons regions are made out of: " << totalRegionPolygons << "\n";
	out << "Total number of points in regions: " << totalRegionPoints << "\n";
	out << "Total number of points in all items: " << totalGeoPoints << "\n";
	m_ra.tds().printStats(out);
	out << "\n";
	out << "OsmKeyValueObjectStore::printStats -- END" << std::endl;
	return out;
}


}}//end namespace