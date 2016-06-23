#include "OsmKeyValueObjectStore.h"
#include <algorithm>
#include <osmpbf/primitiveblockinputadaptor.h>
#include <osmpbf/iway.h>
#include <osmpbf/inode.h>
#include <osmpbf/parsehelpers.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/containers/SimpleBitVector.h>
#include <sserialize/utility/assert.h>
#include <liboscar/OsmKeyValueObjectStore.h>
#include <liboscar/constants.h>
#include <osmtools/AreaExtractor.h>
#include "CellCreator.h"
#include "common.h"
#include "AreaExtractor.h"
#include "helpers.h"


namespace oscar_create {

bool readeSaveDefinitions(const std::string & fn, std::unordered_set<std::string> & dest) {
	std::ifstream file;
	file.open(fn);
	if (file.is_open()) {
		while (!file.eof()) {
			std::string key;
			std::getline(file, key);
			if (key.size())
				dest.insert(key);
		}
		file.close();
		return true;
	}
	return false;
}

bool readeSaveDefinitions(const std::string & fn, std::unordered_map<std::string, std::set<std::string> > & dest) {
	std::ifstream file;
	file.open(fn);
	if (file.is_open()) {
		while (!file.eof()) {
			std::string key, value; 
			std::getline(file, key);
			std::getline(file, value);
			if (key.size() && value.size())
				dest[key].insert(value);
		}
		file.close();
		return true;
	}
	return false;
}

void inflateValues(const std::string& value, std::set< std::string >& destination) {
	std::string c = "";
	std::string::const_iterator end( value.end() );
	for(std::string::const_iterator it = value.begin(); it != end; ++it) {
		if (*it == ';' || *it == ' ' || *it == ',' || *it == ':') {
			if (c.size() > 1) {
				destination.insert(c);
				c = "";
			}
		}
		else {
			c += *it;
		}
	}
	if (c.size() > 0)
		destination.insert(c);
}

///thread-safe
void addScore(OsmKeyValueRawItem & item, ScoreCreator & scoreCreator) {
	if (item.data.osmIdType.id() == 2812851) {
		std::cout << "Landkreis Esslingen get score" << std::endl;
		scoreCreator.dump(std::cout);
		std::cout << std::endl;
		std::cout << item.asKeyValues() << std::endl;
	}
	uint32_t & score = item.data.score = 0;
	for(OsmKeyValueRawItem::RawKeyValuesContainer::const_iterator it(item.rawKeyValues.begin()); it != item.rawKeyValues.end(); ++it) {
		if (scoreCreator.hasScore(it->first)) {
			score = std::max<uint32_t>(scoreCreator.score(it->first), score);
			for(const std::string & value : it->second) {
				score = std::max<uint32_t>(scoreCreator.score(it->first, value), score);
			}
		}
	}
}

bool OsmKeyValueObjectStore::SaveDirector::saveItem(OsmKeyValueRawItem& item) const {
	return process(item);
}

OsmKeyValueObjectStore::SingleTagSaveDirector::SingleTagSaveDirector(const std::string & keysToStoreFn, const std::string & keyValsToStoreFn, const std::string & itemsSavedByKey, const std::string & itemsSavedByKeyVals) {
	if (!readeSaveDefinitions(keysToStoreFn, m_keysToStore)) {
		std::cout << "Failed to read keys to store from " << keysToStoreFn << std::endl;
	}
	if (!readeSaveDefinitions(keyValsToStoreFn, m_keyValuesToStore)) {
		std::cout << "Failed to read key=value to store from " << keyValsToStoreFn << std::endl;
	}
	if (!readeSaveDefinitions(itemsSavedByKey, m_itemSaveByKey)) {
		std::cout << "Failed to read items saved by keys from " << itemsSavedByKey << std::endl;
	}
	if (!readeSaveDefinitions(itemsSavedByKeyVals, m_itemSaveByKeyValue)) {
		std::cout << "Failed to read items saved by key=value from " << itemsSavedByKeyVals << std::endl;
	}
}


bool OsmKeyValueObjectStore::SingleTagSaveDirector::process(oscar_create::OsmKeyValueRawItem & item) const {
	bool saveItem = false;
	OsmKeyValueRawItem::RawKeyValuesContainer newKvs;
	for(const auto & kv : item.rawKeyValues) {
		if (m_keysToStore.count(kv.first) > 0) {
			newKvs[kv.first] = kv.second;
		}
		else if (m_keyValuesToStore.count(kv.first) > 0 ) {
			auto x = sserialize::intersect(kv.second, m_keyValuesToStore.at(kv.first));
			if (x.size()) {
				newKvs[kv.first].swap(x);
			}
		}
		saveItem = saveItem || m_itemSaveByKey.count(kv.first) || (m_itemSaveByKeyValue.count(kv.first) > 0 && sserialize::haveCommonValue(m_itemSaveByKeyValue.at(kv.first), kv.second));
	}
	item.rawKeyValues.swap(newKvs);
	return saveItem;
}

void OsmKeyValueObjectStore::SingleTagSaveDirector::printStats(std::ostream & out) {
	out << "OsmKeyValueObjectStore::SingleTagSaveDirector::stats::begin" << std::endl;
	out << "KeysToStore: " << m_keysToStore.size() << std::endl;
	out << "KeyValuesStore: " << m_keyValuesToStore.size() << std::endl;
	out << "itemSavedByKey: " << m_itemSaveByKey.size() << std::endl;
	out << "itemSavedByKeyValue: " << m_itemSaveByKeyValue.size() << std::endl;
	out << "OsmKeyValueObjectStore::SingleTagSaveDirector::stats::end" << std::endl;
}

OsmKeyValueObjectStore::SaveEveryTagSaveDirector::SaveEveryTagSaveDirector(const std::string & itemsSavedByKey, const std::string & itemsSavedByKeyVals) {
	if (!readeSaveDefinitions(itemsSavedByKey, m_itemSaveByKey)) {
		std::cout << "Failed to read items saved by keys from " << itemsSavedByKey << std::endl;
	}
	if (!readeSaveDefinitions(itemsSavedByKeyVals, m_itemSaveByKeyValue)) {
		std::cout << "Failed to read items saved by key=value from " << itemsSavedByKeyVals << std::endl;
	}
}


bool OsmKeyValueObjectStore::SaveEveryTagSaveDirector::process(oscar_create::OsmKeyValueRawItem & item) const {
	bool saveItem = false;
	for(const auto & kv : item.rawKeyValues) {
		saveItem = saveItem || m_itemSaveByKey.count(kv.first) || (m_itemSaveByKeyValue.count(kv.first) > 0 && sserialize::haveCommonValue(m_itemSaveByKeyValue.at(kv.first), kv.second));
	}
	return saveItem;
}

void OsmKeyValueObjectStore::SaveEveryTagSaveDirector::printStats(std::ostream & out) {
	out << "OsmKeyValueObjectStore::SaveEveryTagSaveDirector::stats::begin" << std::endl;
	out << "itemSavedByKey: " << m_itemSaveByKey.size() << std::endl;
	out << "itemSavedByKeyValue: " << m_itemSaveByKeyValue.size() << std::endl;
	out << "OsmKeyValueObjectStore::SaveEveryTagSaveDirector::stats::end" << std::endl;
}

OsmKeyValueObjectStore::SaveAllItemsIgnoring::SaveAllItemsIgnoring(const std::string& keysToIgnore) {
	if (!readeSaveDefinitions(keysToIgnore, m_ignoreKeys)) {
		std::cout << "Failed to read items saved by keys from " << keysToIgnore << std::endl;
	}
}

bool OsmKeyValueObjectStore::SaveAllItemsIgnoring::process(OsmKeyValueRawItem& item) const {
	for(OsmKeyValueRawItem::RawKeyValuesContainer::iterator it(item.rawKeyValues.begin()), end(item.rawKeyValues.end()); it != end;) {
		if (m_ignoreKeys.count(it->first)) {
			it = item.rawKeyValues.erase(it);
		}
		else {
			++it;
		}
	}
	return item.rawKeyValues.size();
}

void OsmKeyValueObjectStore::SaveAllItemsIgnoring::printStats(std::ostream & out) {
	out << "OsmKeyValueObjectStore::SaveAllItemsIgnoring::stats::begin" << std::endl;
	out << "items ignored by keys: " << m_ignoreKeys.size() << std::endl;
	out << "OsmKeyValueObjectStore::SaveAllItemsIgnoring::stats::end" << std::endl;
}

bool OsmKeyValueObjectStore::SaveAllSaveDirector::process(oscar_create::OsmKeyValueRawItem & /*item*/) const {
	return true;
}

bool OsmKeyValueObjectStore::SaveAllHavingTagsSaveDirector::process(OsmKeyValueRawItem& item) const {
	return item.rawKeyValues.size();
}

OsmKeyValueObjectStore::Context::Context(OsmKeyValueObjectStore * parent, CreationConfig & cc) :
gphMMT(cc.incremental() ? sserialize::MM_SHARED_MEMORY : sserialize::MM_FAST_FILEBASED),
cc(&cc),
parent(parent),
inFile(cc.fileNames),
scoreCreator(cc.scoreConfigFileName),
cellMap(0),
// 		nodeIdToGeoPoint(cc.minNodeId, cc.maxNodeId, gphMMT, GeoPointHashMapHashBase( OADValueStorage(gphMMT), OADTableStorage(sserialize::MM_SHARED_MEMORY) )),
nodeIdToGeoPoint(cc.minNodeId, cc.maxNodeId, gphMMT),
itemIdForCells(sserialize::MM_FILEBASED),
totalItemCount(0),
totalGeoPointCount(0),
totalItemStringsCount(0)
{}

void OsmKeyValueObjectStore::Context::inflateValues(OsmKeyValueRawItem & rawItem, osmpbf::IPrimitive & prim) {
	for(int i = 0; i < prim.tagsSize(); i++) {
		std::string key = prim.key(i);
		std::string value = prim.value(i);
		if (cc->inflateValues.count(key))
			oscar_create::inflateValues(value, rawItem.rawKeyValues[key]);
		else
			rawItem.rawKeyValues[key].insert(value);
	}
}
	
void OsmKeyValueObjectStore::Context::getNodes() {
	GeoPointHashMap & nodesToStore = this->nodesToStore;
	inFile.reset();
	sserialize::MMVector< std::pair<int64_t, RawGeoPoint> > tmpDs(sserialize::MM_PROGRAM_MEMORY);
	std::mutex tmpDsLock;
	osmpbf::parseFileCPPThreads(inFile,
		[&tmpDs,&tmpDsLock, &nodesToStore](osmpbf::PrimitiveBlockInputAdaptor & pbi) -> bool {
			if (!pbi.nodesSize())
				return true;
			std::vector< std::pair<int64_t, RawGeoPoint> > tmp;
			for(osmpbf::INodeStream node(pbi.getNodeStream()); !node.isNull(); node.next()) {
				int64_t nodeId = node.id();
				if (nodesToStore.count(nodeId)) {
					tmp.emplace_back(nodeId, RawGeoPoint(
						sserialize::spatial::GeoPoint::snapLat(node.latd()),
						sserialize::spatial::GeoPoint::snapLon(node.lond()))
					);
				}
			}
			std::unique_lock<std::mutex> lck(tmpDsLock);
			tmpDs.push_back(tmp.begin(), tmp.end());
			return tmpDs.size() < nodesToStore.size();
		}, cc->numThreads, cc->blobFetchCount
	);
// 			std::cout << "Found " << tmpDs.size() << " out of " << nodesToStore.size() << " nodes" << std::endl;
	SSERIALIZE_CHEAP_ASSERT_SMALLER_OR_EQUAL(tmpDs.size(), nodesToStore.size());
	sserialize::mt_sort(tmpDs.begin(), tmpDs.end(), std::less< std::pair<int64_t, RawGeoPoint> >(), cc->numThreads);
	nodesToStore.clear();
	for(const std::pair<int64_t, RawGeoPoint> & x : tmpDs) {
		nodesToStore[x.first] = x.second;
	}
}

uint32_t OsmKeyValueObjectStore::Context::push_back(OsmKeyValueRawItem& rawItem, bool deleteShape) {
	uint32_t realItemId;
	itemFlushLock.lock();
	SSERIALIZE_CHEAP_ASSERT_SMALLER(rawItem.data.id, totalItemCount);
	realItemId = itemIdForCells.size();
	itemIdForCells.push_back(rawItem.data.id);
	itemScores.push_back(rawItem.data.score);
	parent->push_back(rawItem);
	itemFlushLock.unlock();
	if (deleteShape) {
		delete rawItem.data.shape;
	}
	if (relationItems.count(rawItem.data.osmIdType)) {
		relationItems[rawItem.data.osmIdType] = realItemId;
	}
	return realItemId;
}

osmpbf::AbstractTagFilter * OsmKeyValueObjectStore::Context::placeMarkerFilter() {
	return osmpbf::newAnd(
					new osmpbf::KeyMultiValueTagFilter("place", {"hamlet", "town", "village", "suburb"}),
					this->cc->rc.regionFilter->copy()
				);
}

OsmKeyValueObjectStore::MyRegionFilter::MyRegionFilter(const std::unordered_set<liboscar::OsmIdType> * d) :
m_d(d)
{}

void OsmKeyValueObjectStore::MyRegionFilter::assignInputAdaptor(const osmpbf::PrimitiveBlockInputAdaptor* /*pbi*/) {}

bool OsmKeyValueObjectStore::MyRegionFilter::rebuildCache() {
	return true;
}

osmpbf::AbstractTagFilter* OsmKeyValueObjectStore::MyRegionFilter::copy(osmpbf::AbstractTagFilter::CopyMap& copies) const {
	if (copies.count(this)) {
		return copies.at(this);
	}
	MyRegionFilter * tmp = new MyRegionFilter(m_d);
	copies[this] = tmp;
	return tmp;
}

bool OsmKeyValueObjectStore::MyRegionFilter::p_matches(const osmpbf::IPrimitive& primitive) {
	return m_d->count( liboscar::OsmIdType(primitive.id(), toOsmItemType(primitive.type())) );
}

OsmKeyValueObjectStore::OsmKeyValueObjectStore() : m_data(0,0) {}

OsmKeyValueObjectStore::~OsmKeyValueObjectStore() {}

void OsmKeyValueObjectStore::clear() {
	MyBaseClass::clear();
}

void OsmKeyValueObjectStore::push_back(const OsmKeyValueRawItem & item) {
	MyBaseClass::push_back(item.asKeyValues());
	m_data.push_back(item.data);
}

uint32_t OsmKeyValueObjectStore::addKeyValues(OsmKeyValueRawItem::RawKeyValuesContainer& kvs) {
	uint32_t itemStringsCount = 0;
	for(OsmKeyValueRawItem::RawKeyValuesContainer::const_iterator it(kvs.begin()), end(kvs.end()); it != end; ++it) {
		MyBaseClass::addKey(it->first);
		itemStringsCount += it->second.size()*2;
		for(std::set<std::string>::const_iterator jt(it->second.begin()), jend(it->second.end()); jt != jend; ++jt) {
			MyBaseClass::addValue(*jt);
		}
	}
	return itemStringsCount;
}

void OsmKeyValueObjectStore::createRegionStore(Context & ct) {
	sserialize::TimeMeasurer tm;
	tm.begin();
	osmtools::OsmTriangulationRegionStore trs;
	osmtools::OsmGridRegionTree<RegionInfo> polyStore;
	osmtools::AreaExtractor ae;

	{//fetch all residential areas without a name, try to name them with their city-node
		std::cout << "Fetching residential areas without matching tags but with a place-node inside" << std::endl;
		osmpbf::InversionFilter::invert( ct.cc->rc.regionFilter );
		generics::RCPtr<osmpbf::AbstractTagFilter> myFilter(
			osmpbf::newAnd(
				new osmpbf::KeyValueTagFilter("landuse", "residential"),
				ct.cc->rc.regionFilter->copy()
			)
		);
		osmpbf::InversionFilter::invert( ct.cc->rc.regionFilter );
		
		osmtools::OsmGridRegionTree<RegionInfo> polyStore;
		ae.extract(ct.inFile, [&polyStore](const std::shared_ptr<sserialize::spatial::GeoRegion> & region, osmpbf::IPrimitive & primitive) {
			liboscar::OsmIdType osmIdType(primitive.id(), oscar_create::toOsmItemType(primitive.type()));
			polyStore.push_back(*region, RegionInfo(osmIdType, region->type(), region->boundary()));
		}, osmtools::AreaExtractor::ET_ALL_SPECIAL_BUT_BUILDINGS, myFilter, ct.cc->numThreads, "Residential area extraction");
		
		polyStore.setRefinerOptions(2, 2, 10000);
		polyStore.addPolygonsToRaster(10, 10);
		polyStore.printStats(std::cout);
		
		//now try to assign the nameless polygons their city-node
		//we won't do this now, but instead later when adding the region since we only store the osmIdType for a region and its boundary
		//use ctx->regionItems to store the regions with a city node
		//we can then use MyRegionFilter to add these valid regions to the store in the next phase
		
		struct WorkContext {
			std::unordered_set<uint32_t> activeResidentialRegions;
			std::mutex residentialRegionsLock;
		} wct;
		struct MyProc {
			osmtools::OsmGridRegionTree<RegionInfo> * polyStore;
			Context * ctx;
			osmpbf::PrimitiveBlockInputAdaptor * m_pbi;
			osmpbf::RCFilterPtr m_filter;
			WorkContext * wct;
			MyProc(osmtools::OsmGridRegionTree<RegionInfo> * polyStore, Context * ctx, WorkContext * wct) :
			polyStore(polyStore),
			ctx(ctx),
			m_pbi(0),
			m_filter(ctx->placeMarkerFilter()),
			wct(wct)
			{}
			MyProc(const MyProc & other) : MyProc(other.polyStore, other.ctx, other.wct) {}
			void assignPbi(osmpbf::PrimitiveBlockInputAdaptor * pbi) {
				if (m_pbi != pbi) {
					m_filter->assignInputAdaptor(pbi);
				}
			}
			void operator()(osmpbf::PrimitiveBlockInputAdaptor & pbi) {
				if (!pbi.nodesSize()) {
					return;
				}
				assignPbi(&pbi);
				if (!m_filter->rebuildCache()) {
					return;
				}
				std::unordered_set<uint32_t> myIntersected;
				for(osmpbf::INodeStream node(pbi.getNodeStream()); !node.isNull(); node.next()) {
					if (m_filter->matches(node)) {
						polyStore->test(node.latd(), node.lond(), myIntersected);
					}
				}
				std::unique_lock<std::mutex> lck(wct->residentialRegionsLock);
				wct->activeResidentialRegions.insert(myIntersected.begin(), myIntersected.end());
			}
		};
		
		ct.inFile.reset();
		osmpbf::parseFileCPPThreads(ct.inFile, MyProc(&polyStore, &ct, &wct), ct.cc->numThreads, 1, true);
		for(uint32_t x : wct.activeResidentialRegions) {
			ct.residentialRegions.insert(polyStore.values().at(x).osmIdType);
		}
		std::cout << "Collected " << ct.residentialRegions.size() << " residential regions" << std::endl;
	}
	{//init store 
		osmpbf::RCFilterPtr myFilter(
			osmpbf::newOr(
				ct.cc->rc.regionFilter.get(),
				new MyRegionFilter(&(ct.residentialRegions))
			)
		);
		ae.extract(ct.inFile, [&polyStore](const std::shared_ptr<sserialize::spatial::GeoRegion> & region, osmpbf::IPrimitive & primitive) {
			liboscar::OsmIdType osmIdType(primitive.id(), oscar_create::toOsmItemType(primitive.type()));
			polyStore.push_back(*region, RegionInfo(osmIdType, region->type(), region->boundary()));
		}, osmtools::AreaExtractor::ET_ALL_SPECIAL_BUT_BUILDINGS, myFilter, ct.cc->numThreads, "Region extraction");
		polyStore.setRefinerOptions(2, 2, 10000);
		polyStore.addPolygonsToRaster(10, 10);
		polyStore.printStats(std::cout);
		trs.init(polyStore, ct.cc->numThreads);
		trs.initGrid(ct.cc->rc.polyStoreLatCount, ct.cc->rc.polyStoreLonCount);
		trs.makeConnected();
		SSERIALIZE_EXPENSIVE_ASSERT(ct.trs.selfTest());
	}
	//put current active regions into regionItems hash to make sure that the region is not checked with itself
	for(uint32_t i(0), s(polyStore.size()); i < s; ++i) {
		ct.regionItems.insert(polyStore.values().at(i).osmIdType);
	}
	
	struct WorkContext {
		std::mutex nodesToStoreLock;
		std::mutex regionFlushLock;
		sserialize::DynamicBitSet relevantCells;
		uint32_t roundsToProcess;
		WorkContext() : roundsToProcess(0) {}
	} wct;
	wct.relevantCells.resize(trs.cellCount());
	
	ct.inFile.reset();
	ct.progressInfo.begin(ct.inFile.dataSize(), "Finding relevant regions");
	while (ct.inFile.hasNext()) {
		++wct.roundsToProcess;
		//save current position
		osmpbf::OffsetType filePos = ct.inFile.dataPosition();

		//first get the node refs for our ways
		uint32_t blobsRead = osmpbf::parseFileCPPThreads(ct.inFile, [&ct, &wct, &polyStore](osmpbf::PrimitiveBlockInputAdaptor & pbi) -> bool {
			std::unordered_set<int64_t> tmpRefs;
			for(osmpbf::IWayStream way = pbi.getWayStream(); !way.isNull(); way.next()) {
				if (!way.refsSize() || !way.tagsSize() || ct.regionItems.count(liboscar::OsmIdType(way.id(), liboscar::OSMIT_WAY))) {
					continue;
				}
				OsmKeyValueRawItem rawItem;
				ct.inflateValues(rawItem, way);
				
				if (ct.cc->itemSaveDirector->process(rawItem)) {
					ct.totalGeoPointCount += way.refsSize();
				}
			}
			std::unique_lock<std::mutex> lck(wct.nodesToStoreLock);
			for(int64_t x : tmpRefs) {
				ct.nodesToStore.mark(x);
			}
			return ct.nodesToStore.size() < ct.cc->maxNodeCoordTableSize;
		}, ct.cc->numThreads, ct.cc->blobFetchCount);
		#ifdef SSERIALIZE_CHEAP_ASSERT_ENABLED
		osmpbf::OffsetType afterFilePos = ct.inFile.dataPosition();
		#endif
		
		//get the needed nodes this has to be done from the start
		if (ct.nodesToStore.size()) {
			ct.getNodes();
		}
		
		//assemble the ways and put them into storage
		ct.inFile.dataSeek(filePos);
		#ifdef SSERIALIZE_CHEAP_ASSERT_ENABLED
		uint32_t tmp =
		#endif
		osmpbf::parseFileCPPThreads(ct.inFile, [&ct, &wct, &trs](osmpbf::PrimitiveBlockInputAdaptor & pbi) -> void {
			ct.progressInfo(ct.inFile.dataPosition());
			std::vector<OsmKeyValueRawItem> tmpItems;
			std::unordered_set<uint32_t> hitCells;
			
			for (osmpbf::INodeStream node = pbi.getNodeStream(); !node.isNull(); node.next()) {
				if (node.tagsSize() == 0)
					continue;
				OsmKeyValueRawItem rawItem;
				ct.inflateValues(rawItem, node);
				if (ct.cc->itemSaveDirector->process(rawItem)) {
					hitCells.insert(trs.cellId(node.latd(), node.lond()));
				}
			}
				
			for(osmpbf::IWayStream way = pbi.getWayStream(); !way.isNull(); way.next()) {
				if (!way.refsSize() || !way.tagsSize() || ct.regionItems.count(liboscar::OsmIdType(way.id(), liboscar::OSMIT_WAY))) {
					continue;
				}
				OsmKeyValueRawItem rawItem;
				ct.inflateValues(rawItem, way);
				if (ct.cc->itemSaveDirector->process(rawItem)) {
					for(osmpbf::IWayStream::RefIterator it = way.refBegin(); it != way.refEnd(); ++it) {
						if (ct.nodeIdToGeoPoint.count(*it) > 0) {
							const RawGeoPoint & rgp = ct.nodeIdToGeoPoint[*it];
							hitCells.insert( trs.cellId(rgp.first, rgp.second) );
						}
					}
				}
			}
			std::unique_lock<std::mutex> lck(wct.regionFlushLock);
			{
				wct.relevantCells.set(hitCells.cbegin(), hitCells.cend());
			}
		}, ct.cc->numThreads, 1, false, blobsRead);
		SSERIALIZE_CHEAP_ASSERT_EQUAL(blobsRead, tmp);
		SSERIALIZE_CHEAP_ASSERT_EQUAL(afterFilePos, ct.inFile.dataPosition());
		
		//clear node table
		ct.nodesToStore.clear();
		
		SSERIALIZE_CHEAP_ASSERT_EQUAL(ct.itemIdForCells.size(), size());
	}
	ct.progressInfo.end();
	std::cout << "Found " << wct.relevantCells.size() << " cells containing items" << std::endl;
	{
		//delete grt to save space since we need to temprarily hold some copies
		polyStore.clearGRT();
		ct.regionItems.clear();
		
		sserialize::SimpleBitVector relevantRegions;
		relevantRegions.resize(polyStore.size());
		for(uint32_t cellId(0), s(trs.cellCount()); cellId < s; ++cellId) {
			if (wct.relevantCells.isSet(cellId)) {
				const osmtools::OsmTriangulationRegionStore::RegionList & rl = trs.regions(cellId);
				relevantRegions.set(rl.cbegin(), rl.cend());
			}
		}
		trs.clear();
		
		for(uint32_t i(0), s(polyStore.size()); i < s; ++i) {
			if (relevantRegions.isSet(i)) {
				auto & r = polyStore.regions().at(i);
				auto & v = polyStore.values().at(i);
				ct.polyStore.push_back(*r, RegionInfo(v.osmIdType, r->type(), r->boundary()));
				ct.regionItems.insert(v.osmIdType);
			}
		}
		polyStore.clear();
	}
	ct.polyStore.snapPoints();
	
	//update region info due to snapped points
	for(uint32_t i(0), s(ct.polyStore.size()); i < s; ++i) {
		ct.polyStore.values().at(i).boundary = ct.polyStore.regions().at(i)->boundary();
	}
	
	ct.polyStore.setRefinerOptions(2, 2, 10000);
	ct.polyStore.addPolygonsToRaster(10, 10);
	ct.polyStore.printStats(std::cout);
	std::cout << "Creating final TriangulationRegionStore" << std::endl;
	osmtools::OsmTriangulationRegionStore::LipschitzMeshCriteria refinerBase(ct.cc->rc.triangMaxCentroidDist, &(ct.trs.tds()));
	osmtools::OsmTriangulationRegionStore::RegionOnlyLipschitzMeshCriteria refiner(refinerBase);
	ct.trs.init(ct.polyStore, ct.cc->numThreads, &refiner, osmtools::OsmTriangulationRegionStore::MyRefineTag, true);
	ct.trs.initGrid(ct.cc->rc.polyStoreLatCount, ct.cc->rc.polyStoreLonCount);
	ct.trs.refineBySize(ct.cc->rc.polyStoreMaxTriangPerCell, 100, 100000, ct.cc->numThreads);
	SSERIALIZE_EXPENSIVE_ASSERT(ct.trs.selfTest());
	ct.cellMap = decltype(ct.cellMap)(ct.trs.cellCount());
	tm.end();
	ct.trs.printStats(std::cout);
	std::cout << "Total time to create the region store with " << ct.polyStore.size() << " regions: " << tm << std::endl;
}

void OsmKeyValueObjectStore::addPolyStoreItems(Context & ctx) {
	if (!ctx.polyStore.values().size()) {
		return;
	}
	struct WorkContext {
		std::unordered_map<liboscar::OsmIdType, uint32_t> osmIdToRegionId;
		WorkContext() {}
	} wct;
	for(uint32_t i(0), s(ctx.polyStore.values().size()); i < s; ++i) {
		wct.osmIdToRegionId[ctx.polyStore.values().at(i).osmIdType] = i;
	}
	
	struct Worker {
		Context & ctx;
		WorkContext & wct;
		std::atomic<uint32_t> foundRegionsCounter;
		std::vector<OsmKeyValueRawItem> rawItems;
		osmpbf::RCFilterPtr placeMarkerFilter;
		Worker(Context & ctx, WorkContext & wct) :
		ctx(ctx), wct(wct),
		foundRegionsCounter(0), rawItems(ctx.polyStore.size()),
		placeMarkerFilter(ctx.placeMarkerFilter())
		{}
		Worker(Worker & o) : ctx(o.ctx), wct(o.wct) { throw std::runtime_error("Illegal function call");}
		void flush() {
			SSERIALIZE_CHEAP_ASSERT_EQUAL(foundRegionsCounter.load(), ctx.polyStore.size());
			ctx.push_back(rawItems.begin(), rawItems.end(), false);
		}
		void processItem(uint32_t regionId, osmpbf::IPrimitive & primitive) {
			OsmKeyValueRawItem & rawItem = rawItems.at(regionId);
			ctx.inflateValues(rawItem, primitive);
			rawItem.data.osmIdType = ctx.polyStore.values().at(regionId).osmIdType;
			rawItem.data.shape = ctx.polyStore.regions().at(regionId);
			rawItem.data.id = regionId;
			
			++foundRegionsCounter;
			
			ctx.cc->itemSaveDirector->process(rawItem);
			oscar_create::addScore(rawItem, ctx.scoreCreator);
		}
		void processPlaceMarker(osmpbf::INode & node) {
			uint32_t myCellId = ctx.trs.cellId(node.latd(), node.lond());
			const auto & cellRegions = ctx.trs.regions(myCellId);
			for(const uint32_t & regionId : cellRegions) {
				const  RegionInfo & ri = ctx.polyStore.values().at(regionId);
				if (ctx.residentialRegions.count(ri.osmIdType)) {
					OsmKeyValueRawItem & destRawItem = rawItems.at(regionId);
					OsmKeyValueRawItem tmpRawItem;
					ctx.inflateValues(tmpRawItem, node);
					if (ctx.cc->itemSaveDirector->process(tmpRawItem)) {
						for(const auto & x : tmpRawItem.rawKeyValues) {
							destRawItem.rawKeyValues[x.first].insert(x.second.begin(), x.second.end());
						}
					}
				}
			}
		}
		void operator()(osmpbf::PrimitiveBlockInputAdaptor & pbi) {
			ctx.progressInfo(ctx.inFile.dataPosition());
			for(osmpbf::INodeStream node(pbi.getNodeStream()); !node.isNull(); node.next()) {
				if (placeMarkerFilter->matches(node)) {
					processPlaceMarker(node);
				}
			}
			for(osmpbf::IWayStream way(pbi.getWayStream()); !way.isNull(); way.next()) {
				liboscar::OsmIdType tmp(way.id(), liboscar::OSMIT_WAY);
				if (wct.osmIdToRegionId.count(tmp)) {
					processItem(wct.osmIdToRegionId.at(tmp), way);
				}
			}
			for(osmpbf::IRelationStream rel(pbi.getRelationStream()); !rel.isNull(); rel.next()) {
				liboscar::OsmIdType tmp(rel.id(), liboscar::OSMIT_RELATION);
				if (wct.osmIdToRegionId.count(tmp)) {
					processItem(wct.osmIdToRegionId.at(tmp), rel);
				}
			}
		}
	};
	Worker w(ctx, wct);
	ctx.progressInfo.begin(ctx.inFile.dataSize(), "Adding items from polystore");
	ctx.inFile.reset();
	osmpbf::parseFileCPPThreads<Worker*>(ctx.inFile, &w, ctx.cc->numThreads, ctx.cc->blobFetchCount);
	w.flush();
	ctx.progressInfo.end();
	
	//add regions to their cells
	if (ctx.cc->addRegionsToCells) {
		for(uint32_t cellId(0), s(ctx.trs.cellCount()); cellId < s; ++cellId) {
			for(uint32_t regionId : ctx.trs.regions(cellId)) {
				//BUG: cell boundaries are not correct here!
				ctx.cellMap.insert(cellId, regionId, sserialize::spatial::GeoRect());
			}
		}
	}
	SSERIALIZE_CHEAP_ASSERT_EQUAL(size(), ctx.polyStore.size());
	SSERIALIZE_CHEAP_ASSERT_EQUAL(ctx.itemIdForCells.size(), size());
	ctx.regionInfo = std::move(ctx.polyStore.values());
	ctx.polyStore.clear();
}

void OsmKeyValueObjectStore::insertItemStrings(OsmKeyValueObjectStore::Context& ct) {
	struct WorkContext {
		std::mutex addKeyLock;
	} wct;
	
	{//handle relation items, including multipolygon items which neeed to be handled differently while adding them
		auto wf = [&ct, &wct](osmpbf::PrimitiveBlockInputAdaptor & pbi) {
			for(osmpbf::IRelationStream rel(pbi.getRelationStream()); !rel.isNull(); rel.next()) {
				OsmKeyValueRawItem rawItem;
				ct.inflateValues(rawItem, rel);
				if (ct.cc->itemSaveDirector->process(rawItem)) {
					++ct.totalItemCount;
					
					uint32_t itemStringsCount;
					{
						std::unique_lock<std::mutex> lck(wct.addKeyLock);
						itemStringsCount = ct.parent->addKeyValues(rawItem.rawKeyValues);
					}
					ct.totalItemStringsCount += itemStringsCount;
				}
			}
		};
		ct.inFile.reset();
		uint32_t myNumThreads = std::min<uint32_t>(( ct.cc->numThreads ? ct.cc->numThreads : (uint32_t) std::thread::hardware_concurrency()), 2);
		osmpbf::parseFileCPPThreads(ct.inFile, wf, myNumThreads, ct.cc->blobFetchCount);
	}

	ct.progressInfo.begin(ct.inFile.dataSize(), "Fetching strings");
	{
		ct.inFile.reset();
		osmpbf::parseFileCPPThreads(ct.inFile, [&ct, &wct](osmpbf::PrimitiveBlockInputAdaptor & pbi) -> void {
			ct.progressInfo(ct.inFile.dataPosition());
			
			for (osmpbf::INodeStream node(pbi.getNodeStream()); !node.isNull(); node.next()) {
				if (!node.tagsSize()) {
					continue;
				}
				OsmKeyValueRawItem rawItem;
				ct.inflateValues(rawItem, node);
				if (ct.cc->itemSaveDirector->process(rawItem)) {
					++ct.totalItemCount;
					
					uint32_t itemStringsCount;
					{
						std::unique_lock<std::mutex> lck(wct.addKeyLock);
						itemStringsCount = ct.parent->addKeyValues(rawItem.rawKeyValues);
					}
					ct.totalItemStringsCount += itemStringsCount;
				}
			}
			
			for(osmpbf::IWayStream way(pbi.getWayStream()); !way.isNull(); way.next()) {
				if (!way.refsSize() || !way.tagsSize()) {
					continue;
				}
				OsmKeyValueRawItem rawItem;
				ct.inflateValues(rawItem, way);
				
				if (ct.cc->itemSaveDirector->process(rawItem) || ct.regionItems.count(liboscar::OsmIdType(way.id(), liboscar::OSMIT_WAY))) {
					++ct.totalItemCount;
					uint32_t itemStringsCount;
					{
						std::unique_lock<std::mutex> lck(wct.addKeyLock);
						itemStringsCount = ct.parent->addKeyValues(rawItem.rawKeyValues);
					}
					ct.totalItemStringsCount += itemStringsCount;
				}
			}
			
			for(osmpbf::IRelationStream rel(pbi.getRelationStream()); !rel.isNull(); rel.next()) {
				if (!rel.membersSize() || !rel.tagsSize())
					continue;
				
				OsmKeyValueRawItem rawItem;
				ct.inflateValues(rawItem, rel);
				
				if (ct.cc->itemSaveDirector->process(rawItem) || ct.regionItems.count(liboscar::OsmIdType(rel.id(), liboscar::OSMIT_RELATION))) {
					++ct.totalItemCount;
					uint32_t itemStringsCount;
					{
						std::unique_lock<std::mutex> lck(wct.addKeyLock);
						itemStringsCount = ct.parent->addKeyValues(rawItem.rawKeyValues);
					}
					ct.totalItemStringsCount += itemStringsCount;
				}
			}
			
		}, ct.cc->numThreads, ct.cc->blobFetchCount);
	}
	ct.progressInfo.end();
}

//Side note: points of relations are currently not snapped.
//Instead we snap the boundary during insertion into the cellmap
void OsmKeyValueObjectStore::insertItems(OsmKeyValueObjectStore::Context& ct) {
	struct WorkContext {
		std::mutex nodesToStoreLock;
		std::atomic<uint32_t> itemId;
		uint32_t roundsToProcess;
		WorkContext() : roundsToProcess(0) {}
	} wct;
	wct.itemId.store(size());
	
	SSERIALIZE_CHEAP_ASSERT_EQUAL(ct.regionInfo.size(), size());
	SSERIALIZE_CHEAP_ASSERT_EQUAL(ct.itemIdForCells.size(), size());
	
	{//handle relations
		struct RelationWorkContext {
			std::mutex multiPolyItemsLock;
			std::unordered_set<liboscar::OsmIdType> multiPolyItems;
		} rwct;
		
		{//fill ct.relationItems map, make sure to not add relations that are in the store or that are multipolygons
			generics::RCPtr<osmpbf::AbstractTagFilter> multiPolyFilter(osmtools::AreaExtractor::createExtractionFilter(osmtools::AreaExtractor::ET_ALL_MULTIPOLYGONS));
			auto wf = [&ct, &multiPolyFilter](osmpbf::PrimitiveBlockInputAdaptor & pbi) {
				if (!pbi.relationsSize()) {
					return;
				}
				std::unordered_set<liboscar::OsmIdType> tmp;
				for(osmpbf::IRelationStream rel(pbi.getRelationStream()); !rel.isNull(); rel.next()) {
					//multi-poly items are handled differntly
					if (multiPolyFilter->matches(rel)) {
						continue;
					}
					OsmKeyValueRawItem rawItem;
					ct.inflateValues(rawItem, rel);
					if (!ct.cc->itemSaveDirector->process(rawItem)) {
						continue;
					}
					for(osmpbf::IMemberStream mem(rel.getMemberStream()); !mem.isNull(); mem.next()) {
						tmp.insert(liboscar::OsmIdType(mem.id(), toOsmItemType(mem.type()) ));
					}
				}
				std::unique_lock<std::mutex> itemFlushLock(ct.itemFlushLock);
				for(const liboscar::OsmIdType & x : tmp) {
					ct.relationItems[x] = std::numeric_limits<uint32_t>::max();
				}
			};
			struct SaturationWorkContext {
				std::mutex lock;
				std::unordered_set<liboscar::OsmIdType> missingRelation;
			} swct;
			//function to saturate ct.relationItems map
			auto swf = [&ct, &swct](osmpbf::PrimitiveBlockInputAdaptor & pbi) {
				if (!pbi.relationsSize()) {
					return;
				}
				std::unordered_set<liboscar::OsmIdType> tmp;
				for(osmpbf::IRelationStream rel(pbi.getRelationStream()); !rel.isNull(); rel.next()) {
					if (!ct.relationItems.count(liboscar::OsmIdType(rel.id(), liboscar::OSMIT_RELATION))) {
						continue;
						
					}
					for(osmpbf::IMemberStream mem(rel.getMemberStream()); !mem.isNull(); mem.next()) {
						liboscar::OsmIdType osmIdType(mem.id(), toOsmItemType(mem.type()) );
						if (!ct.relationItems.count(osmIdType)) {
							tmp.insert(osmIdType);
						}
					}
				}
				std::unique_lock<std::mutex> lck(swct.lock);
				swct.missingRelation.insert(tmp.begin(), tmp.end());
			};
			ct.inFile.reset();
			osmpbf::parseFileCPPThreads(ct.inFile, wf, ct.cc->numThreads, ct.cc->blobFetchCount);
			do {
				for(const liboscar::OsmIdType & x : swct.missingRelation) {
					ct.relationItems[x] = std::numeric_limits<uint32_t>::max();
				}
				swct.missingRelation.clear();
				ct.inFile.reset();
				osmpbf::parseFileCPPThreads(ct.inFile, swf, ct.cc->numThreads, ct.cc->blobFetchCount);
			} while(swct.missingRelation.size());
		}
		
		{//handle relation multi polyons skip the ones that are in the store
			osmtools::AreaExtractor ae;
			generics::RCPtr<osmpbf::AbstractTagFilter> myRegionFilter(new MyRegionFilter(&ct.regionItems));
			osmpbf::InversionFilter::invert(myRegionFilter);
			auto wf = [&ct, &wct, &rwct](const std::shared_ptr<sserialize::spatial::GeoRegion> & region, osmpbf::IPrimitive & primitive) {
				OsmKeyValueRawItem rawItem;
				ct.inflateValues(rawItem, primitive);
				if (ct.cc->itemSaveDirector->process(rawItem)) {
					rawItem.data.osmIdType.id(primitive.id());
					rawItem.data.osmIdType.type(liboscar::OSMIT_RELATION);
					rawItem.data.shape = region.get();
					rawItem.data.id = wct.itemId.fetch_add(1);
					oscar_create::addScore(rawItem, ct.scoreCreator);
					ct.parent->createCell<sserialize::spatial::GeoPolygon, sserialize::spatial::GeoMultiPolygon>(rawItem, ct);
					ct.push_back(rawItem, false);
					std::unique_lock<std::mutex> lck(rwct.multiPolyItemsLock);
					rwct.multiPolyItems.insert(liboscar::OsmIdType(primitive.id(), liboscar::OSMIT_RELATION));
				}
			};
			ae.extract(ct.inFile, wf, osmtools::AreaExtractor::ET_ALL_MULTIPOLYGONS, myRegionFilter, ct.cc->numThreads, "Fetching multipolygon items");

			SSERIALIZE_CHEAP_ASSERT_EQUAL(wct.itemId.load(), ct.itemIdForCells.size());
		}
	}
	
	ct.inFile.reset();
	ct.progressInfo.begin(ct.inFile.dataSize(), "Processing file");
	while (ct.inFile.hasNext()) {
		++wct.roundsToProcess;
		//save current position
		osmpbf::OffsetType filePos = ct.inFile.dataPosition();

		//first get the node refs for our ways
		uint32_t blobsRead = osmpbf::parseFileCPPThreads(ct.inFile, [&ct, &wct](osmpbf::PrimitiveBlockInputAdaptor & pbi) -> bool {
			std::vector<int64_t> myNodesToStore;
			for(osmpbf::IWayStream way = pbi.getWayStream(); !way.isNull(); way.next()) {
				if (!way.refsSize() || !way.tagsSize()) {
					continue;
				}
				OsmKeyValueRawItem rawItem;
				ct.inflateValues(rawItem, way);
				
				if (ct.cc->itemSaveDirector->process(rawItem)) {
					ct.totalGeoPointCount += way.refsSize();
					myNodesToStore.insert(myNodesToStore.end(), way.refBegin(), way.refEnd());
				}
			}
			std::unique_lock<std::mutex> lck(wct.nodesToStoreLock);
			for(auto x : myNodesToStore) {
				ct.nodesToStore.mark(x);
			}
			return ct.nodesToStore.size() < ct.cc->maxNodeCoordTableSize;
		}, ct.cc->numThreads, ct.cc->blobFetchCount);
		#ifdef SSERIALIZE_CHEAP_ASSERT_ENABLED
		osmpbf::OffsetType afterFilePos = ct.inFile.dataPosition();
		#endif
		
		//get the needed nodes this has to be done from the start
		if (ct.nodesToStore.size()) {
			ct.getNodes();
		}
		
		//assemble the ways and put them into storage
		ct.inFile.dataSeek(filePos);
		#ifdef SSERIALIZE_CHEAP_ASSERT_ENABLED
		uint32_t tmp =
		#endif
		osmpbf::parseFileCPPThreads(ct.inFile, [&ct, &wct](osmpbf::PrimitiveBlockInputAdaptor & pbi) -> void {
			ct.progressInfo(ct.inFile.dataPosition());
			std::vector<OsmKeyValueRawItem> tmpItems;
			for (osmpbf::INodeStream node = pbi.getNodeStream(); !node.isNull(); node.next()) {
				if (node.tagsSize() == 0)
					continue;
				OsmKeyValueRawItem rawItem;
				ct.inflateValues(rawItem, node);
				if (ct.cc->itemSaveDirector->process(rawItem)) {
					rawItem.data.osmIdType.id(node.id());
					rawItem.data.osmIdType.type(liboscar::OSMIT_NODE);
					rawItem.data.shape = new sserialize::spatial::GeoPoint(
						sserialize::spatial::GeoPoint::snapLat(node.latd()),
						sserialize::spatial::GeoPoint::snapLon(node.lond())
					);
					rawItem.data.id = wct.itemId.fetch_add(1);
					oscar_create::addScore(rawItem, ct.scoreCreator);
					ct.parent->createCell<sserialize::spatial::GeoPolygon, sserialize::spatial::GeoMultiPolygon>(rawItem, ct);
					tmpItems.emplace_back(std::move(rawItem));
				}
			}
				
			for(osmpbf::IWayStream way = pbi.getWayStream(); !way.isNull(); way.next()) {
				if (!way.refsSize() || !way.tagsSize() || ct.regionItems.count(liboscar::OsmIdType(way.id(), liboscar::OSMIT_WAY)) ) {
					continue;
				}
				OsmKeyValueRawItem rawItem;
				ct.inflateValues(rawItem, way);
				if (ct.cc->itemSaveDirector->process(rawItem)) {
					rawItem.data.osmIdType.id(way.id());
					rawItem.data.osmIdType.type(liboscar::OSMIT_WAY);
					sserialize::spatial::GeoWay * gw = new sserialize::spatial::GeoWay();
					for(osmpbf::IWayStream::RefIterator it = way.refBegin(); it != way.refEnd(); ++it) {
						if (ct.nodeIdToGeoPoint.count(*it) > 0) {
							const RawGeoPoint & rgp = ct.nodeIdToGeoPoint[*it];
							SSERIALIZE_NORMAL_ASSERT(sserialize::spatial::GeoPoint(rgp).valid());
							gw->points().emplace_back(rgp);
						}
					}
					gw->recalculateBoundary();
					
					rawItem.data.id = wct.itemId.fetch_add(1);
					rawItem.data.shape = gw;
					
					oscar_create::addScore(rawItem, ct.scoreCreator);
					ct.parent->createCell<sserialize::spatial::GeoPolygon, sserialize::spatial::GeoMultiPolygon>(rawItem, ct);
					tmpItems.emplace_back(std::move(rawItem));
				}
			}
			//flush items to store
			ct.push_back(tmpItems.begin(), tmpItems.end(), true);
		}, ct.cc->numThreads, 1, false, blobsRead);
		SSERIALIZE_CHEAP_ASSERT_EQUAL(blobsRead, tmp);
		SSERIALIZE_CHEAP_ASSERT_EQUAL(afterFilePos, ct.inFile.dataPosition());
		
		//clear node table
		ct.nodesToStore.clear();
		
		SSERIALIZE_CHEAP_ASSERT_EQUAL(ct.itemIdForCells.size(), wct.itemId.load());
		SSERIALIZE_CHEAP_ASSERT_EQUAL(ct.itemIdForCells.size(), size());
	}
	ct.progressInfo.end();
	std::cout << "Took " << wct.roundsToProcess << " rounds to process the file" << std::endl;
	//completely clear node hashes
	ct.nodeIdToGeoPoint = Context::GeoPointHashMap();
}

bool OsmKeyValueObjectStore::populate(CreationConfig & cc) {
	SSERIALIZE_CHEAP_ASSERT_LARGER(cc.rc.triangMaxCentroidDist, 1);
	
	Context ct(this, cc);
	createRegionStore(ct);
	
	//Polygon items need to be handled before all other items
	insertItemStrings(ct);
	
	std::cout << "Finalizing string table..." << std::flush;
	ct.tm.begin();
	MyBaseClass::finalizeStringTables();
	ct.tm.end();
	std::cout << ct.tm.elapsedMilliSeconds() << " msecs" << std::endl;

	//Polygon items need to be handled before all other items
	std::cout << "Inserting region store items" << std::endl;
	addPolyStoreItems(ct);
	SSERIALIZE_CHEAP_ASSERT_EQUAL(ct.itemIdForCells.size(), ct.regionInfo.size());

	insertItems(ct);
	
	applySort(ct);

	SSERIALIZE_CHEAP_ASSERT_EQUAL(ct.itemIdForCells.size(), size());
	processCellMap(ct);
	
	return true;
}

//T_FUNC sorts in ascending order
template<typename T_SORT_FUNC>
void createNewToOldItemIdMap(std::vector<uint32_t> & newToOldItemId, uint32_t regionCount, T_SORT_FUNC func) {
	sserialize::mt_sort(newToOldItemId.begin(), newToOldItemId.begin()+regionCount, func, std::thread::hardware_concurrency()*2);
	sserialize::mt_sort(newToOldItemId.begin()+regionCount, newToOldItemId.end(), func, std::thread::hardware_concurrency()*2);
}

struct SortByScore {
	const uint32_t * scores;
	SortByScore(const SortByScore & other) : SortByScore(other.scores) {}
	SortByScore(const uint32_t * scores) : scores(scores) {}
	//a > b
	bool operator()(uint32_t aId, uint32_t bId) {
		return scores[aId] > scores[bId];
	}
};

//this sorts as follows:
//existance of name > score > name
struct SortByScoreName {
	const uint32_t * scores;
	const uint32_t * nameValueIds;
	SortByScoreName(const SortByScoreName & other) : SortByScoreName(other.scores, other.nameValueIds) {}
	SortByScoreName(const uint32_t * scores, const uint32_t * nameValueIds) : scores(scores), nameValueIds(nameValueIds) {}
	bool operator()(uint32_t aId, uint32_t bId) {
		uint32_t nameValueIdA = nameValueIds[aId];
		uint32_t nameValueIdB = nameValueIds[bId];
		uint32_t scoreA = scores[aId];
		uint32_t scoreB = scores[bId];
		
		if (nameValueIdA == std::numeric_limits<uint32_t>::max() && nameValueIdB == std::numeric_limits<uint32_t>::max()) {
			return scoreA > scoreB;
		}
		if (nameValueIdA == std::numeric_limits<uint32_t>::max()) {
			return true;
		}
		if (nameValueIdB == std::numeric_limits<uint32_t>::max()) {
			return false;
		}
		//both have a name, sort by score 
		if (scoreA == scoreB) {
			return nameValueIdA < nameValueIdB;
		}
		else {
			return scoreA > scoreB;
		}
	}
};

///Sort first by score and then by strings
struct SortByPrioStrings {
	OsmKeyValueObjectStore * store;
	const uint32_t * scores;
	//maps keyids to [0..n] whre the position indicates the priority of the key
	const std::unordered_map<uint32_t, uint32_t> * stringToPrio;
	sserialize::SimpleBitVector m_a;
	sserialize::SimpleBitVector m_b;
	SortByPrioStrings(OsmKeyValueObjectStore * store, const uint32_t * scores, const std::unordered_map<uint32_t, uint32_t> * stringToPrio) :
	store(store),
	scores(scores),
	stringToPrio(stringToPrio)
	{
		m_a.resize(stringToPrio->size());
		m_b.resize(stringToPrio->size());
	}
	//Item(aId) < Item(bId)
	bool operator()(uint32_t aId, uint32_t bId) {
		uint32_t aScore = scores[aId];
		uint32_t bScore = scores[bId];
		if (aScore == bScore) {
			m_a.reset();
			m_b.reset();
			OsmKeyValueObjectStore::MyBaseClass::Item itemA(static_cast<OsmKeyValueObjectStore::MyBaseClass*>(store)->at(aId));
			OsmKeyValueObjectStore::MyBaseClass::Item itemB(static_cast<OsmKeyValueObjectStore::MyBaseClass*>(store)->at(bId));
			uint32_t sa = itemA.size();
			uint32_t sb = itemB.size();
			for(uint32_t iA(0), iB(0); iA < sa && iB < sb;) {
				uint32_t akId = itemA.keyId(iA);
				uint32_t bkId = itemB.keyId(iB);
				if (akId == bkId) {
					if (stringToPrio->count(akId)) {
						uint32_t vA = itemA.valueId(iA);
						uint32_t vB = itemB.valueId(iB);
						if (vA < vB) {
							m_b.set(stringToPrio->at(akId));
						}
						else if (vB < vA) {
							m_a.set(stringToPrio->at(akId));
						}
					}
					++iA;
					++iB;
				}
				else if (akId < bkId) {
					if (stringToPrio->count(akId)) { //a is larger
						m_b.set(stringToPrio->at(akId));
					}
					++iA;
				}
				else { //bkId < akId
					if (stringToPrio->count(bkId)) {
						m_b.set(stringToPrio->at(bkId));//b is larger
					}
					++iB;
				}
				
			}
			for(uint32_t i(0), s(stringToPrio->size()); i < s; ++i) {
				if (!m_a.isSet(i) && m_b.isSet(i)) { //a is smaller if any entry has a zero for a and a 1 for b
					return true;
				}
				else if (m_a.isSet(i) && !m_b.isSet(i)) { //a is larger if any entry has a one for a and a 0 for b
					return false;
				}
			}
			return false;
		}
		else {
			return aScore > bScore;
		}
	}
};

void OsmKeyValueObjectStore::applySort(oscar_create::OsmKeyValueObjectStore::Context& ctx) {
	SSERIALIZE_CHEAP_ASSERT(!m_newToOldItemId.size());
	SSERIALIZE_CHEAP_ASSERT_EQUAL(ctx.regionInfo.size(), ctx.regionItems.size());
	SSERIALIZE_CHEAP_ASSERT(ctx.regionInfo.size() || !ctx.trs.cellCount());
	SSERIALIZE_CHEAP_ASSERT_EQUAL(ctx.itemScores.size(), size());
	m_newToOldItemId.resize(size());
	for(uint32_t i(0), s(size()); i < s; ++i) {
		m_newToOldItemId[i] = i;
	}
	sserialize::TimeMeasurer tm;
	tm.begin();
	std::cout << "Sorting items..." << std::endl;
	switch(ctx.cc->sortOrder) {
	case ISO_NONE:
		break;
	case ISO_SCORE:
		createNewToOldItemIdMap(m_newToOldItemId, ctx.regionInfo.size(), SortByScore(ctx.itemScores.begin()));
		break;
	case ISO_SCORE_NAME:
		if (keyStringTable().count("name")) {
			sserialize::MMVector<uint32_t> nameValueIds(sserialize::MM_PROGRAM_MEMORY, size(), std::numeric_limits<uint32_t>::max());
			sserialize::ProgressInfo pinfo;
			uint32_t nameId = keyStringTable().at("name");
			pinfo.begin(size(), "Sorter: Precalculating name info");
			for(uint32_t i(0), s(size()); i < s; ++i) {
				MyBaseClass::Item item(MyBaseClass::at(i));
				uint32_t strPos = item.findKey(nameId);
				if (strPos != MyBaseClass::Item::npos) {
					nameValueIds.at(i) = item.valueId(strPos);
				}
			}
			pinfo.end();
			createNewToOldItemIdMap(m_newToOldItemId, ctx.regionInfo.size(), SortByScoreName(ctx.itemScores.begin(), nameValueIds.begin()));
		}
		else {
			createNewToOldItemIdMap(m_newToOldItemId, ctx.regionInfo.size(), SortByScore(ctx.itemScores.begin()));
		}
		break;
	case ISO_SCORE_PRIO_STRINGS:
	{
		std::vector<std::string> tmp;
		std::unordered_map<uint32_t, uint32_t> stringToPrio;
		readKeysFromFile(ctx.cc->prioStringsFn, std::back_inserter(tmp));
		for(const std::string & x : tmp) {
			if(keyStringTable().count(x)) {
				uint32_t keyId = keyStringTable().at(x);
				if (!stringToPrio.count(keyId)) {
					uint32_t prio = stringToPrio.size();
					stringToPrio[keyId] = prio;
				}
			}
		}
		createNewToOldItemIdMap(m_newToOldItemId, ctx.regionInfo.size(), SortByPrioStrings(this, ctx.itemScores.begin(), &stringToPrio));
		break;
	}
	default:
		break;
	}
	tm.end();
	std::cout << "Sorting items took " << tm << std::endl;
}

bool OsmKeyValueObjectStore::processCellMap(Context & ctx) {
	SSERIALIZE_CHEAP_ASSERT_EQUAL(m_newToOldItemId.size(), m_data.size());
	SSERIALIZE_CHEAP_ASSERT_EQUAL(ctx.itemIdForCells.size(), size());
	//Let's create our Hierarchy
	CellCreator::CellListType cellList;
	CellCreator cc;
	sserialize::Static::spatial::TracGraph tracGraph;
	{//create celllist and the TriangulationGeoHierarchyArrangement
		std::vector<uint32_t> newToOldCellId;
		cc.createCellList(ctx.cellMap, ctx.trs, cellList, newToOldCellId);
		std::unordered_map<uint32_t, uint32_t> oldToNewCellId;
		for(uint32_t i(0), s(newToOldCellId.size()); i < s; ++i) {
			oldToNewCellId[newToOldCellId.at(i)] = i;
		}
		m_ra = sserialize::UByteArrayAdapter::createCache(newToOldCellId.size()*4, sserialize::MM_FILEBASED);
		ctx.trs.append(m_ra, oldToNewCellId);
		sserialize::UByteArrayAdapter::OffsetType tracGraphBegin = m_ra.size();
		ctx.trs.cellGraph().append(m_ra, oldToNewCellId);
		m_ra << ctx.trs.cellCenterOfMass(oldToNewCellId);
		tracGraph = sserialize::Static::spatial::TracGraph(m_ra+tracGraphBegin);
		ctx.trs.clear();
	}
	cc.createGeoHierarchy(cellList, ctx.regionInfo.size(), m_gh);
	//set the neighbor pointers
	m_gh.createNeighborPointers(tracGraph);
	m_gh.printStats(std::cout);
	{ //remap the items in the gh to the unremapped ids
		
		std::vector<uint32_t> tmpIdToReorderedId(ctx.itemIdForCells.size(), 0);
		for(uint32_t i(0), s(ctx.itemIdForCells.size()); i < s; ++i) {
			uint32_t tmpItemId = ctx.itemIdForCells.at(i);
			tmpIdToReorderedId.at(tmpItemId) = i;
		}
		for(uint32_t i = 0, s = m_gh.cells().size(); i < s; ++i) {
			auto cellItems = m_gh.cells()[i].items();
			for(auto cIt(cellItems.begin()), cEnd(cellItems.end()); cIt != cEnd; ++cIt) {
				*cIt = tmpIdToReorderedId[*cIt];
			}
			std::sort(cellItems.begin(), cellItems.end());
			SSERIALIZE_EXPENSIVE_ASSERT(sserialize::is_strong_monotone_ascending(cellItems.begin(), cellItems.end()));
// 			cellItems.resize( std::distance(cellItems.begin(), std::unique(cellItems.begin(), cellItems.end())));
		}
	}
	{ //now add cell ptrs to items
		std::vector< std::vector<uint32_t> > cellPtrs(size());
		for(uint32_t i = 0, s = m_gh.cells().size(); i < s; ++i) {
			auto items = m_gh.cells().at(i).items();
			for(auto it(items.begin()), end(items.end()); it != end; ++it) {
				cellPtrs[*it].push_back(i);
			}
		}
		PayloadContainerType tmp(m_data.size(), m_data.reservedSize());
		for(uint32_t i = 0, s = m_data.size(); i < s; ++i) {
			sserialize::UByteArrayAdapter & d = tmp.beginRawPush();
			d.putData( m_data.dataAt(i) );
			sserialize::BoundedCompactUintArray::create(cellPtrs[i], d);
			tmp.endRawPush();
		}
		using std::swap;
		swap(m_data, tmp);
	}
	
	//apply the item reorder map, ids are unremapped ids
	{
		std::vector<uint32_t> oldToNewItemId(m_newToOldItemId.size());
		for(uint32_t i(0), s(m_newToOldItemId.size()); i < s; ++i) {
			oldToNewItemId.at(m_newToOldItemId.at(i)) = i;
		}
		for(uint32_t i = 0, s = m_gh.cells().size(); i < s; ++i) {
			auto cellItems = m_gh.cells()[i].items();
			for(auto cIt(cellItems.begin()), cEnd(cellItems.end()); cIt != cEnd; ++cIt) {
				*cIt = oldToNewItemId.at(*cIt);
			}
			std::sort(cellItems.begin(), cellItems.end());
			SSERIALIZE_EXPENSIVE_ASSERT(sserialize::is_strong_monotone_ascending(cellItems.begin(), cellItems.end()));
		}
		for(sserialize::spatial::GeoHierarchy::Region & r : m_gh.regions()) {
			const RegionInfo & ri = ctx.regionInfo.at(r.storeId);
			r.boundary = ri.boundary;
			r.type = ri.gsType;
			r.storeId = oldToNewItemId.at(r.storeId);
		}
	}
	
	m_gh.createRootRegion();
	if (!m_gh.checkConsistency()) {
		std::cout << "GeoHierarchy failed the consistency check!" << std::endl;
		return false;
	}
	else {
		std::cout << "GeoHierarchy passed the consistency check." << std::endl;
		return true;
	}
}

std::ostream & OsmKeyValueObjectStore::stats(std::ostream & out) {
	out << "OsmKeyValueObjectStore::stats::begin" << std::endl;
	out << "Items: " << m_data.size() << std::endl;
	out << "Keys: " << keyStringTable().size() << std::endl;
	out << "Values: " << valueStringTable().size() << std::endl;
	out << "Boundary: " << boundary() << std::endl;
	out << "OsmKeyValueObjectStore::stats::end" << std::endl;
	return out;
}

sserialize::spatial::GeoRect OsmKeyValueObjectStore::boundary() const {
	if (!m_data.size()) {
		return sserialize::spatial::GeoRect();
	}
	sserialize::spatial::GeoRect rect;
	size_t i = 0;

	//find the first valid rect
	for(; i < m_data.size(); i++) {
		PayloadType pl = payloadAt(i);
		sserialize::Static::spatial::GeoShape shape = pl.shape();
		if (shape.type() != sserialize::spatial::GS_NONE) {
			rect = shape.boundary();
			break;
		}
	}
	for(; i < m_data.size(); i++) {
		PayloadType pl = payloadAt(i);
		sserialize::Static::spatial::GeoShape shape = pl.shape();
		if (shape.type() != sserialize::spatial::GS_NONE) {
			rect.enlarge(shape.boundary());
		}
	}
	return rect;
}

sserialize::UByteArrayAdapter::OffsetType OsmKeyValueObjectStore::serialize(sserialize::UByteArrayAdapter & dest, sserialize::ItemIndexFactory & idxFactory, bool fullRegionIndex) const {
	sserialize::UByteArrayAdapter::OffsetType dataBegin = dest.tellPutPtr();
	dest.putUint8(7);//version
	std::cout << "Serializing OsmKeyValueObjectStore payload..." << std::flush;
	sserialize::TimeMeasurer tm;
	tm.begin();
	m_data.toArray(dest);
	tm.end();
	std::cout << "took " << tm.elapsedSeconds() << " seconds" << std::endl;
	sserialize::BoundedCompactUintArray::create(m_newToOldItemId, dest);
	MyBaseClass::serialize(dest);
	std::cout << "Serializing gh...";
	sserialize::UByteArrayAdapter ghd =  m_gh.append(dest, idxFactory, fullRegionIndex);
	sserialize::Static::spatial::GeoHierarchy sgh(ghd);
	if (!m_gh.testEquality(sgh)) {
		std::cout << "ERROR: GeoHierarchy serialization error. GeoHierarchies are not equivalent" << std::endl;
	}
	else {
		std::cout << "OK" << std::endl;
	}
	//append the TriangulationGeoHierarchyArrangement data
	dest.putData(m_ra);
	return dest.tellPutPtr() - dataBegin;
}

sserialize::UByteArrayAdapter operator<<(sserialize::UByteArrayAdapter & dest, const OsmKeyValueRawItem::OsmKeyValueDataPayload & src) {
	dest << src.osmIdType;
	dest.putVlPackedUint32(src.score);
	return sserialize::spatial::GeoShape::appendWithTypeInfo(src.shape, dest);
}


}//end namespace