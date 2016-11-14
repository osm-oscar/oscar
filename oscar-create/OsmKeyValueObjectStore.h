#ifndef OSCAR_CREATE_OSM_KEY_VALUE_OBJECT_STORE_H
#define OSCAR_CREATE_OSM_KEY_VALUE_OBJECT_STORE_H
#include <sserialize/Static/DynamicKeyValueObjectStore.h>
#include <sserialize/Static/DynamicVector.h>
#include <sserialize/spatial/GeoHierarchy.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/containers/DirectHugeHash.h>
#include <sserialize/containers/MMVector.h>
#include <sserialize/containers/OADHashTable.h>
#include <sserialize/containers/DirectHugeHashSet.h>
#include <osmpbf/filter.h>
#include <osmpbf/pbistream.h>
#include <liboscar/OsmKeyValueObjectStore.h>
#include <algorithm>
#include <utility>
#include "ScoreCreator.h"
#include "CellCreator.h"
#include "TagStore.h"

namespace oscar_create {

struct OsmKeyValueRawItem {
	typedef std::vector< std::pair<std::string, std::string> > KeyValues;
	//This is BAD!!!
	typedef std::map< std::string, std::set<std::string> > RawKeyValuesContainer;
	struct OsmKeyValueDataPayload {
		OsmKeyValueDataPayload() : osmIdType(), shape(0), id(0xFFFFFFFF) {}
		OsmKeyValueDataPayload(const OsmKeyValueDataPayload & other) : osmIdType(other.osmIdType), shape(other.shape), score(other.score), id(other.id) {}
		///shape has to be manualy deleted
		~OsmKeyValueDataPayload() {}
		liboscar::OsmIdType osmIdType;
		sserialize::spatial::GeoShape * shape;
		uint32_t score;
		///only used temporarily
        uint32_t id;
	};
	
	RawKeyValuesContainer rawKeyValues;
	OsmKeyValueDataPayload data;

	OsmKeyValueRawItem() {}
	OsmKeyValueRawItem(const OsmKeyValueRawItem & other) : rawKeyValues(other.rawKeyValues), data(other.data) {}
	OsmKeyValueRawItem(OsmKeyValueRawItem && other) : rawKeyValues(std::move(other.rawKeyValues)), data(std::move(other.data)) {}
	
	OsmKeyValueRawItem & operator=(const OsmKeyValueRawItem & other) {
		rawKeyValues = other.rawKeyValues;
		data = other.data;
		return *this;
	}

	OsmKeyValueRawItem & operator=(OsmKeyValueRawItem && other) {
		rawKeyValues = std::move(other.rawKeyValues);
		data = std::move(other.data);
		return *this;
	}

	inline KeyValues asKeyValues() const {
		KeyValues kv;
		for(RawKeyValuesContainer::const_iterator kIt(rawKeyValues.begin()); kIt != rawKeyValues.end(); ++kIt) {
			for(std::set<std::string>::const_iterator vIt(kIt->second.begin()); vIt != kIt->second.end(); ++vIt) {
				kv.push_back(std::pair<std::string, std::string>(kIt->first, *vIt) );
			}
		}
		return kv;
	}
	
	inline void insertKeyValues(const KeyValues & kv) {
		for(KeyValues::const_iterator it(kv.begin()), end(kv.end()); it != end; ++it) {
			rawKeyValues[it->first].insert(it->second);
		}
	}

	inline void swap(OsmKeyValueRawItem & other) {
		using std::swap;
		swap(rawKeyValues, other.rawKeyValues);
		swap(data, other.data);
	}
};

class OsmKeyValueObjectStore: public sserialize::Static::DynamicKeyValueObjectStore {
public:
	typedef sserialize::Static::DynamicKeyValueObjectStore MyBaseClass;
	typedef liboscar::Static::OsmKeyValueObjectStorePayload PayloadType;
protected:
	typedef sserialize::Static::DynamicVector<OsmKeyValueRawItem::OsmKeyValueDataPayload, PayloadType> PayloadContainerType;
public:

	typedef enum {ISO_NONE, ISO_SCORE, ISO_SCORE_NAME, ISO_SCORE_PRIO_STRINGS} ItemSortOrder;

	struct SaveDirector {
		SaveDirector() {}
		virtual ~SaveDirector() {}
		///@return save item or not, calls process by default by may be faster for certain SaveDirectors
		virtual bool saveItem(OsmKeyValueRawItem & item) const;
		///Process the item (i.e. removing/adding more keyValues) @return save this item
		virtual bool process(OsmKeyValueRawItem & item) const = 0;
	};
	
	class SingleTagSaveDirector: public SaveDirector {
	private:
		std::unordered_set<std::string> m_keysToStore;
		std::unordered_map<std::string, std::set<std::string> > m_keyValuesToStore;
		std::unordered_set<std::string> m_itemSaveByKey;
		std::unordered_map<std::string, std::set<std::string> > m_itemSaveByKeyValue;
	public:
		SingleTagSaveDirector() {}
		SingleTagSaveDirector(const std::string & keysToStoreFn, const std::string & keyValsToStoreFn,const std::string & itemsSavedByKey, const std::string & itemsSavedByKeyVals);
		virtual ~SingleTagSaveDirector() {}
		virtual bool process(OsmKeyValueRawItem & item) const override;
		void printStats(std::ostream & out);
	};
	
	class SaveEveryTagSaveDirector: public SaveDirector {
	private:
		std::unordered_set<std::string> m_itemSaveByKey;
		std::unordered_map<std::string, std::set<std::string> > m_itemSaveByKeyValue;
	public:
		SaveEveryTagSaveDirector() {}
		SaveEveryTagSaveDirector(const std::string & itemsSavedByKey, const std::string & itemsSavedByKeyVals);
		virtual ~SaveEveryTagSaveDirector() {}
		virtual bool process(OsmKeyValueRawItem & item) const;
		void printStats(std::ostream & out);
	};
	
	///Saves all items having tags but removes keys defined in keysToIgnore
	struct SaveAllItemsIgnoring: public SaveDirector {
		std::unordered_set<std::string> m_ignoreKeys;
	public:
		SaveAllItemsIgnoring(const std::string & keysToIgnore);
		virtual ~SaveAllItemsIgnoring() {}
		virtual bool process(OsmKeyValueRawItem & item) const;
		void printStats(std::ostream & out);
	};
	
	struct SaveAllSaveDirector: public SaveDirector {
		virtual bool process(OsmKeyValueRawItem & item) const;
	};
	
	struct SaveAllHavingTagsSaveDirector: public SaveDirector {
		virtual bool process(OsmKeyValueRawItem & item) const;
	};
	
	class Item: public MyBaseClass::Item {
	protected:
		uint32_t m_id;
	protected:
		const OsmKeyValueObjectStore * store() const { return static_cast<const OsmKeyValueObjectStore *>( MyBaseClass::Item::store() ); }
		inline uint32_t id() const { return m_id; }
	public:
		Item() {}
		Item(uint32_t id, const MyBaseClass::Item & item) : MyBaseClass::Item(item), m_id(id) {}
		~Item() {}
		PayloadType payload() const { return store()->payloadAt(id()); }
	};

	struct CreationConfig {
		uint32_t maxNodeCoordTableSize; //24 Bytes per entry + 8/hash_load_factor()
		std::vector<std::string> fileNames;
		std::string scoreConfigFileName;
		std::shared_ptr<SaveDirector> itemSaveDirector;
		uint64_t minNodeId; //min node id for the direct node hash
		uint64_t maxNodeId; //max node id for the direct node hash
		uint32_t numThreads;
		uint32_t blobFetchCount;
		std::unordered_set<std::string> inflateValues;
		ItemSortOrder sortOrder;
		std::string prioStringsFn;//needed if sortOrder == ISO_SCORE_PRIO_STRINGS
		bool addRegionsToCells;
		//a filter that defines regions
		struct RegionConfig {
			generics::RCPtr<osmpbf::AbstractTagFilter> regionFilter;
			uint32_t polyStoreLatCount;
			uint32_t polyStoreLonCount;
			uint32_t polyStoreMaxTriangPerCell;
			double triangMaxCentroidDist;
			RegionConfig() : polyStoreLatCount(100), polyStoreLonCount(100), polyStoreMaxTriangPerCell(std::numeric_limits<uint32_t>::max()), triangMaxCentroidDist(std::numeric_limits<double>::max()) {}
		} rc;
		CreationConfig() : maxNodeCoordTableSize(std::numeric_limits<uint32_t>::max()), minNodeId(0), maxNodeId(0), numThreads(1), sortOrder(ISO_SCORE), addRegionsToCells(false) {}
		inline bool incremental() { return maxNodeCoordTableSize != std::numeric_limits<uint32_t>::max(); }
	};

private:
	struct RawGeoPoint: public std::pair<double, double> {
		RawGeoPoint(double first, double second) : std::pair<double, double>(first, second) {}
		RawGeoPoint() : std::pair<double, double>(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()) {}
		RawGeoPoint(const std::pair<double, double> & other) : std::pair<double, double>(other) {}
		RawGeoPoint(const RawGeoPoint & other) : std::pair<double, double>(other) {}
		~RawGeoPoint() {}
	};
	
	struct RegionInfo {
		liboscar::OsmIdType osmIdType;
		sserialize::spatial::GeoShapeType gsType;
		sserialize::spatial::GeoRect boundary;
		RegionInfo(const liboscar::OsmIdType & osmIdType, sserialize::spatial::GeoShapeType gst, const sserialize::spatial::GeoRect & boundary) :
		osmIdType(osmIdType), gsType(gst), boundary(boundary)
		{}
	};
	
	struct Context {
		typedef sserialize::MMVector< std::pair<uint64_t, RawGeoPoint> > OADValueStorage;
		typedef sserialize::MMVector<uint32_t> OADTableStorage;
		typedef sserialize::OADHashTable<	uint64_t,
											RawGeoPoint,
											sserialize::detail::OADHashTable::DefaultHash<uint64_t, false>,
											sserialize::detail::OADHashTable::DefaultHash<uint64_t, true>,
											OADValueStorage,
											OADTableStorage
											> GeoPointHashMapHashBase;
	// 	typedef sserialize::DirectHugeHashMap<RawGeoPoint,  GeoPointHashMapHashBase> GeoPointHashMap;
		typedef sserialize::DirectHugeHashMap<RawGeoPoint, std::unordered_map<uint64_t, RawGeoPoint> > GeoPointHashMap;
		const sserialize::MmappedMemoryType gphMMT;
		CreationConfig * cc;
		
		OsmKeyValueObjectStore * parent;
		
		osmpbf::PbiStream inFile;
		
		//the grt is only used temporarily and the store data will be deleted by addPolygonStoreItems
		osmtools::OsmGridRegionTree<RegionInfo> polyStore;
		osmtools::OsmTriangulationRegionStore trs;
		
		std::unordered_set<liboscar::OsmIdType> residentialRegions;
		
		//populated after adding the polygonstore items
		std::vector<RegionInfo> regionInfo;
		
		ScoreCreator scoreCreator;
		CellCreator::CellMapType cellMap;
		sserialize::ProgressInfo progressInfo;
		sserialize::TimeMeasurer tm;

		GeoPointHashMapHashBase nodeIdToGeoPointBase;
		GeoPointHashMap nodeIdToGeoPoint;
		GeoPointHashMap & nodesToStore = nodeIdToGeoPoint;

		std::unordered_set<liboscar::OsmIdType> regionItems;

		//the ids of items needed by relations, default value for itemId should be std::numeric_limits<uint32_t>::max()
		std::unordered_map<liboscar::OsmIdType, uint32_t> relationItems;

		//hold temporary itemid mapping from kvstore.id -> rawitem.data.id for remapping of ids in cells
		sserialize::MMVector<uint32_t> itemIdForCells;
		
		//hold score of items
		sserialize::MMVector<uint32_t> itemScores;

		std::atomic<uint32_t> totalItemCount;
		std::atomic<uint64_t> totalGeoPointCount;
		std::atomic<uint64_t> totalItemStringsCount;
		std::mutex itemFlushLock;
		///initializes regionItems with items from cc.polygonStore
		Context(OsmKeyValueObjectStore * parent, CreationConfig & cc);
		Context(const Context & other) = delete;
		
		void inflateValues(OsmKeyValueRawItem & rawItem, osmpbf::IPrimitive & prim);
		void getNodes();
		uint32_t push_back(OsmKeyValueRawItem & rawItem, bool deleteShape);
		template<typename T_IT>
		void push_back(T_IT begin, T_IT end, bool deleteShape) {
			itemFlushLock.lock();
			for(T_IT it(begin); it != end; ++it) {
				SSERIALIZE_CHEAP_ASSERT_SMALLER(it->data.id, totalItemCount);
				uint32_t realItemId = (uint32_t) itemIdForCells.size();
				itemIdForCells.push_back(it->data.id);
				itemScores.push_back(it->data.score);
				parent->push_back(*it);
				if (relationItems.count(begin->data.osmIdType)) {
					relationItems[begin->data.osmIdType] = realItemId;
				}
			}
			itemFlushLock.unlock();
			if (deleteShape) {
				for(T_IT it(begin); it != end; ++it) {
					delete it->data.shape;
				}
			}
		}
		osmpbf::AbstractTagFilter * placeMarkerFilter();
	};
	
// 	//Accept primitive if it is in @param acceptedPrimitives
	class MyRegionFilter: public osmpbf::AbstractTagFilter {
	private:
		const std::unordered_set<liboscar::OsmIdType> * m_d;
	public:
		MyRegionFilter(const std::unordered_set<liboscar::OsmIdType> * acceptedPrimitives);
		virtual ~MyRegionFilter() {}
		virtual void assignInputAdaptor(const osmpbf::PrimitiveBlockInputAdaptor* pbi);
		virtual bool rebuildCache();
	protected:
		virtual AbstractTagFilter* copy(CopyMap& copies) const;
		virtual bool p_matches(const osmpbf::IPrimitive& primitive);
	};

private:
	PayloadContainerType m_data;
	std::vector<uint32_t> m_newToOldItemId;
	sserialize::spatial::GeoHierarchy m_gh;
	sserialize::UByteArrayAdapter m_ra; //triangulation geohierarchy arrangement with cellgraph
private:
	void push_back(const OsmKeyValueRawItem & item);
	uint32_t addKeyValues(oscar_create::OsmKeyValueRawItem::RawKeyValuesContainer& kvs);
	
	template<typename T_PolygonType, typename T_MultiPolygonType>
	void createCell(OsmKeyValueRawItem & item, Context & ctx);
	
	void createRegionStore(oscar_create::OsmKeyValueObjectStore::Context& ctx);
	
	void insertItemStrings(Context & ctx);

	void insertItems(Context & ctx);
	///deletes ctx.polyStore, populates ctx.regionInfo
	void addPolyStoreItems(Context & ctx);

	bool processCellMap(Context & ctx);
	
	void applySort(Context & ctx);
	
	OsmKeyValueObjectStore & operator=(const OsmKeyValueObjectStore & other);
public:
	OsmKeyValueObjectStore();
	///deletes all shapes!
	virtual ~OsmKeyValueObjectStore();
	void clear();
	std::ostream & stats(std::ostream & out);
	
	///BUG: this only supports adding a single file, not multiple!
	bool populate(CreationConfig & cc);
	
	inline Item at(uint32_t pos) const {
		return Item(pos, MyBaseClass::at(pos));
	}
	
	inline PayloadType payloadAt(uint32_t pos) const { return m_data.at(pos); }

	sserialize::spatial::GeoRect boundary() const;
	
	sserialize::UByteArrayAdapter::OffsetType serialize(sserialize::UByteArrayAdapter& dest, sserialize::ItemIndexFactory& idxFactory, bool fullRegionIndex) const;
};

template<typename T_PolygonType, typename T_MultiPolygonType>
void OsmKeyValueObjectStore::createCell(OsmKeyValueRawItem & item, Context & ctx) {
	typedef typename T_PolygonType::MyBaseClass MyGeoWay;
	typedef std::unordered_map<uint32_t, sserialize::spatial::GeoRect> MyCellMap;
	std::unordered_set<uint32_t> polys;
	uint32_t itemId = item.data.id;
	if ( dynamic_cast<MyGeoWay*>( item.data.shape ) ) { //ways
		MyGeoWay * gw = static_cast<MyGeoWay*>(item.data.shape);
		MyCellMap tmp;
		for(typename MyGeoWay::const_iterator pit(gw->cbegin()), pend(gw->cend()); pit != pend; ++pit) {
			uint32_t cellId = ctx.trs.cellId(*pit);
			if (cellId == osmtools::OsmTriangulationRegionStore::InfiniteFacesCellId) {
				continue;
			}
			if (tmp.count(cellId)) {
				tmp[cellId].enlarge((*pit).boundary());
			}
			else {
				tmp[cellId] = (*pit).boundary();
			}
		}
		SSERIALIZE_CHEAP_ASSERT(tmp.size());
		for(MyCellMap::iterator it(tmp.begin()), end(tmp.end()); it != end; ++it) {
			//snap boundary here. All points creating the rect are snapped to the same coords anyway
			ctx.cellMap.insert(it->first, item.data.id, it->second);
		}
	}
	else if (dynamic_cast<const sserialize::spatial::GeoPoint*>(item.data.shape) ) {
		const sserialize::spatial::GeoPoint * gp = static_cast<const sserialize::spatial::GeoPoint*>(item.data.shape);
		uint32_t cellId = ctx.trs.cellId(*gp);
		SSERIALIZE_CHEAP_ASSERT_NOT_EQUAL(cellId, osmtools::OsmTriangulationRegionStore::InfiniteFacesCellId);
		ctx.cellMap.insert(cellId, itemId, gp->boundary());
	}
	else if (dynamic_cast<const T_MultiPolygonType*>(item.data.shape)) {
		typedef T_MultiPolygonType MyGMP;
		typedef typename MyGMP::PolygonList::value_type MyP;
		
		MyGMP * gmp = static_cast<MyGMP*>(item.data.shape);
		MyCellMap tmp;
		for(typename MyGMP::PolygonList::const_iterator it(gmp->outerPolygons().begin()), end(gmp->outerPolygons().end()); it != end; ++it) {
			typename MyGMP::PolygonList::const_reference gw = *it;
			for(typename MyP::const_iterator pit(gw.cbegin()), pend(gw.cend()); pit != pend; ++pit) {
				uint32_t cellId = ctx.trs.cellId(*pit);
				if (cellId == osmtools::OsmTriangulationRegionStore::InfiniteFacesCellId) {
					continue;
				}
				if (tmp.count(cellId)) {
					tmp[cellId].enlarge((*pit).boundary());
				}
				else {
					tmp[cellId] = (*pit).boundary();
				}
			}
		}
		SSERIALIZE_CHEAP_ASSERT(tmp.size());
		for(MyCellMap::iterator it(tmp.begin()), end(tmp.end()); it != end; ++it) {
			ctx.cellMap.insert(it->first, item.data.id, it->second);
		}
	}
}

sserialize::UByteArrayAdapter operator<<(sserialize::UByteArrayAdapter & dest, const OsmKeyValueRawItem::OsmKeyValueDataPayload & src);

}//end namespace


#endif