#include "CQRFromPolygon.h"
#include <sserialize/Static/GeoPolygon.h>
#include <sserialize/Static/GeoMultiPolygon.h>

namespace liboscar {

CQRFromPolygon::CQRFromPolygon(const CQRFromPolygon & other) :
m_priv(other.m_priv)
{}

CQRFromPolygon::CQRFromPolygon(const Static::OsmKeyValueObjectStore & store, const sserialize::Static::ItemIndexStore & idxStore) :
m_priv(new detail::CQRFromPolygon(store, idxStore))
{}

CQRFromPolygon::~CQRFromPolygon() {}

const sserialize::Static::spatial::GeoHierarchy & CQRFromPolygon::geoHierarchy() const {
	return m_priv->geoHierarchy();
}
sserialize::ItemIndex CQRFromPolygon::intersectingCells(const sserialize::spatial::GeoPolygon & gp, Accuracy ac) const {
	return m_priv->intersectingCells(gp, ac);
}

namespace detail {

CQRFromPolygon::CQRFromPolygon(const Static::OsmKeyValueObjectStore& store, const sserialize::Static::ItemIndexStore & idxStore) :
m_store(store),
m_idxStore(idxStore)
{}

CQRFromPolygon::~CQRFromPolygon() {}

const sserialize::Static::spatial::GeoHierarchy& CQRFromPolygon::geoHierarchy() const {
	return m_store.geoHierarchy();
}

sserialize::ItemIndex CQRFromPolygon::intersectingCells(const sserialize::spatial::GeoPolygon & gp, liboscar::CQRFromPolygon::Accuracy ac) const {
	switch (ac) {
	case liboscar::CQRFromPolygon::AC_POLYGON_CELL:
	case liboscar::CQRFromPolygon::AC_POLYGON_CELL_BBOX:
	{
		return intersectingCellsPolygonCellBBox(gp);
		break;
	}
	case liboscar::CQRFromPolygon::AC_POLYGON_BBOX_CELL_BBOX:
	default:
		return m_store.geoHierarchy().intersectingCells(m_idxStore, gp.boundary());
		break;
	};
}

sserialize::ItemIndex CQRFromPolygon::intersectingCellsPolygonCellBBox(const sserialize::spatial::GeoPolygon& gp) const {
	typedef sserialize::Static::spatial::GeoHierarchy::Region Region;
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;
	
	sserialize::Static::spatial::GeoPolygon sgp;
	{//as of now intersection between static and non-static geometry is not available
		sserialize::UByteArrayAdapter d(sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
		gp.append(d);
		d.resetPtrs();
		sgp = sserialize::Static::spatial::GeoPolygon(d);
	}
	
	const GeoHierarchy & gh = m_store.geoHierarchy();
	sserialize::spatial::GeoRect rect(gp.boundary());
	
	std::deque<uint32_t> queue;
	std::vector<uint32_t> intersectingCells;
	std::unordered_set<uint32_t> visitedRegions;
	Region r(gh.rootRegion());
	for(uint32_t i(0), s(r.childrenSize()); i < s; ++i) {
		uint32_t childId = r.child(i);
		if (rect.overlap(gh.regionBoundary(childId))) {
			uint32_t childStoreId = gh.ghIdToStoreId(childId);
			sserialize::Static::spatial::GeoShape gs(m_store.geoShape(childStoreId));
			if(gs.get<sserialize::spatial::GeoRegion>()->intersects(sgp)) {
				queue.push_back(childId);
				visitedRegions.insert(childId);
			}
		}
	}
	while (queue.size()) {
		//by definition: regions in the queue intersect the query polygon
		r = gh.region(queue.front());
		queue.pop_front();
		sserialize::Static::spatial::GeoShape gs(m_store.geoShape(r.storeId()));
		bool enclosed = false;
		if (sgp.type() == sserialize::spatial::GS_POLYGON) {
			enclosed = sgp.encloses(*(gs.get<sserialize::Static::spatial::GeoPolygon>()));
		}
		else if (sgp.type() == sserialize::spatial::GS_MULTI_POLYGON) {
			enclosed = gs.get<sserialize::Static::spatial::GeoMultiPolygon>()->enclosed(sgp);
		}
		if (enclosed) {
			//checking the itemsCount of the region does only work if the hierarchy was created with a full region item index
			//so instead we have to check the cellcount
			uint32_t cIdxPtr = r.cellIndexPtr();
			if (m_idxStore.idxSize(cIdxPtr)) {
				sserialize::ItemIndex idx(m_idxStore.at(cIdxPtr));
				intersectingCells.insert(intersectingCells.end(), idx.cbegin(), idx.cend());
			}
		}
		else {//just an intersection, check the children and the region exclusive cells
			for(uint32_t i(0), s(r.childrenSize()); i < s; ++i) {
				uint32_t childId = r.child(i);
				if (!visitedRegions.count(childId) && rect.overlap(gh.regionBoundary(childId))) {
					uint32_t childStoreId = gh.ghIdToStoreId(childId);
					sserialize::Static::spatial::GeoShape gs(m_store.geoShape(childStoreId));
					if(gs.get<sserialize::spatial::GeoRegion>()->intersects(sgp)) {
						queue.push_back(childId);
						visitedRegions.insert(childId);
					}
				}
			}
			//check cells that are not part of children regions
			uint32_t exclusiveCellIndexPtr = r.exclusiveCellIndexPtr();
			if (m_idxStore.idxSize(exclusiveCellIndexPtr)) {
				sserialize::ItemIndex idx(m_idxStore.at(exclusiveCellIndexPtr));
				for(uint32_t cellId : idx) {
					if (gp.intersects(gh.cellBoundary(cellId))) {
						intersectingCells.push_back(cellId);
					}
				}
			}
		}
	}
	std::sort(intersectingCells.begin(), intersectingCells.end());
	intersectingCells.resize(std::unique(intersectingCells.begin(), intersectingCells.end())-intersectingCells.begin());
	return sserialize::ItemIndex(std::move(intersectingCells)); 
}



}}//end namespace liboscar::detail