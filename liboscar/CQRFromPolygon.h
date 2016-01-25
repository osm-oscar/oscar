#ifndef LIBOSCAR_CQR_FROM_POLYGON_H
#define LIBOSCAR_CQR_FROM_POLYGON_H
#include "OsmKeyValueObjectStore.h"
#include <sserialize/spatial/GeoPolygon.h>
#include <sserialize/Static/GeoPolygon.h>
#include <sserialize/Static/GeoMultiPolygon.h>

namespace liboscar {
namespace detail {
	class CQRFromPolygon;
}

class CQRFromPolygon final {
public:
	///AC_POLYGON_CELL and better arecurrently unimplemented and fall back to AC_POLYGON_CELL_BBOX
	enum Accuracy : uint32_t { AC_AUTO, AC_POLYGON_BBOX_CELL_BBOX, AC_POLYGON_CELL_BBOX, AC_POLYGON_CELL, AC_POLYGON_ITEM_BBOX, AC_POLYGON_ITEM};
public:
	CQRFromPolygon(const CQRFromPolygon & other);
	CQRFromPolygon(const Static::OsmKeyValueObjectStore & store, const sserialize::Static::ItemIndexStore & idxStore);
	~CQRFromPolygon();
	const Static::OsmKeyValueObjectStore & store() const;
	const sserialize::Static::spatial::GeoHierarchy & geoHierarchy() const;
	const sserialize::Static::ItemIndexStore & idxStore() const;
	///returns only fm cells, only usefull with AC_POLYGON_BBOX_CELL and AC_POLYGON_CELL_BBOX
	sserialize::ItemIndex fullMatches(const sserialize::spatial::GeoPolygon & gp, Accuracy ac) const;
	///supports AC_POLYGON_ITEM_BBOX and AC_POLYGON_ITEM, does NOT support AC_POLYGON_CELL, falls back to AC_POLYGON_CELL_BBOX
	sserialize::CellQueryResult cqr(const sserialize::spatial::GeoPolygon & gp, Accuracy ac) const;
private:
	sserialize::RCPtrWrapper<detail::CQRFromPolygon> m_priv;
};

namespace detail {

class CQRFromPolygon: public sserialize::RefCountObject {
public:
	CQRFromPolygon(const Static::OsmKeyValueObjectStore & store, const sserialize::Static::ItemIndexStore & idxStore);
	virtual ~CQRFromPolygon();
	const Static::OsmKeyValueObjectStore & store() const;
	const sserialize::Static::spatial::GeoHierarchy & geoHierarchy() const;
	const sserialize::Static::ItemIndexStore & idxStore() const;
	sserialize::ItemIndex fullMatches(const sserialize::spatial::GeoPolygon& gp, liboscar::CQRFromPolygon::Accuracy ac) const;
	sserialize::CellQueryResult cqr(const sserialize::spatial::GeoPolygon & gp, liboscar::CQRFromPolygon::Accuracy ac) const;
private:
	template<typename T_OPERATOR>
	void visit(const sserialize::spatial::GeoPolygon & gp, const sserialize::Static::spatial::GeoPolygon& sgp, T_OPERATOR & op) const;
	sserialize::Static::spatial::GeoPolygon toStatic(const sserialize::spatial::GeoPolygon & gp) const;
	sserialize::ItemIndex intersectingCellsPolygonCellBBox(const sserialize::spatial::GeoPolygon & gp) const;
	template<typename T_OPERATOR>
	sserialize::CellQueryResult intersectingCellsPolygonItem(const sserialize::spatial::GeoPolygon & gp) const;
private:
	Static::OsmKeyValueObjectStore m_store;
	sserialize::Static::ItemIndexStore m_idxStore;
};

template<typename T_OPERATOR>
void CQRFromPolygon::visit(const sserialize::spatial::GeoPolygon& gp, const sserialize::Static::spatial::GeoPolygon& sgp, T_OPERATOR & op) const {
	typedef sserialize::Static::spatial::GeoHierarchy::Region Region;
	typedef sserialize::Static::spatial::GeoHierarchy GeoHierarchy;

	const GeoHierarchy & gh = m_store.geoHierarchy();
	sserialize::spatial::GeoRect rect(gp.boundary());
	
	std::deque<uint32_t> queue;
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
		if (gs.type() == sserialize::spatial::GS_POLYGON) {
			enclosed = sgp.encloses(*(gs.get<sserialize::Static::spatial::GeoPolygon>()));
		}
		else if (gs.type() == sserialize::spatial::GS_MULTI_POLYGON) {
			enclosed = gs.get<sserialize::Static::spatial::GeoMultiPolygon>()->enclosed(sgp);
		}
		if (enclosed) {
			//checking the itemsCount of the region does only work if the hierarchy was created with a full region item index
			//so instead we have to check the cellcount
			uint32_t cIdxPtr = r.cellIndexPtr();
			if (m_idxStore.idxSize(cIdxPtr)) {
				sserialize::ItemIndex idx(m_idxStore.at(cIdxPtr));
				op.enclosed(idx);
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
				op.candidates(idx);
			}
		}
	}
}

namespace CQRFromPolygonHelpers {

///T_OPERATOR needs to implement bool intersects(uint32_t itemId) returning if the item intersects the query polygon
template<typename T_OPERATOR>
struct PolyCellItemIntersectBaseOp {
	typedef T_OPERATOR MySubClass;
	const sserialize::spatial::GeoPolygon & gp;
	const sserialize::Static::spatial::GeoPolygon & sgp;
	const sserialize::Static::spatial::GeoHierarchy & gh;
	const liboscar::Static::OsmKeyValueObjectStore & store;
	const sserialize::Static::ItemIndexStore & idxStore;
	std::unordered_set<uint32_t> & fullMatches;
	std::map<uint32_t, sserialize::ItemIndex> & partialMatches;
	
	//temporary storage
	std::vector<uint32_t> intersectingItems;
	
	void enclosed(const sserialize::ItemIndex & enclosedCells) {
		fullMatches.insert(enclosedCells.cbegin(), enclosedCells.cend());
	}
	void candidates(const sserialize::ItemIndex & candidateCells) {
		for(uint32_t cellId : candidateCells) {
			if (!fullMatches.count(cellId) && !partialMatches.count(cellId) && gp.intersects(gh.cellBoundary(cellId))) {
				sserialize::ItemIndex cellItems( idxStore.at( gh.cellItemsPtr(cellId) ) );
				for(uint32_t itemId : cellItems) {
					if (static_cast<MySubClass*>(this)->intersects(itemId)) {
						intersectingItems.push_back(itemId);
					}
				}
				if (intersectingItems.size() == cellItems.size()) { //this is a fullmatch
					fullMatches.insert(cellId);
				}
				else if (intersectingItems.size()) {
					partialMatches[cellId] = sserialize::ItemIndex(std::move(intersectingItems));
					intersectingItems = std::vector<uint32_t>();
				}
			}
		}
	}
	PolyCellItemIntersectBaseOp(const sserialize::spatial::GeoPolygon & gp,
				const sserialize::Static::spatial::GeoPolygon & sgp,
				const sserialize::Static::spatial::GeoHierarchy & gh,
				const liboscar::Static::OsmKeyValueObjectStore & store,
				const sserialize::Static::ItemIndexStore & idxStore,
				std::unordered_set<uint32_t> & fullMatches,
				std::map<uint32_t, sserialize::ItemIndex> & partialMatches) :
	gp(gp), sgp(sgp), gh(gh), store(store), idxStore(idxStore), fullMatches(fullMatches), partialMatches(partialMatches)
	{}
};

struct PolyCellItemBBoxIntersectOp: public PolyCellItemIntersectBaseOp<PolyCellItemBBoxIntersectOp> {
	inline bool intersects(uint32_t itemId) {
		return gp.intersects(store.geoShape(itemId).boundary());
	}
	PolyCellItemBBoxIntersectOp(const sserialize::spatial::GeoPolygon & gp,
				const sserialize::Static::spatial::GeoPolygon & sgp,
				const sserialize::Static::spatial::GeoHierarchy & gh,
				const liboscar::Static::OsmKeyValueObjectStore & store,
				const sserialize::Static::ItemIndexStore & idxStore,
				std::unordered_set<uint32_t> & fullMatches,
				std::map<uint32_t, sserialize::ItemIndex> & partialMatches) :
	PolyCellItemIntersectBaseOp(gp, sgp, gh, store, idxStore, fullMatches, partialMatches)
	{}
};

struct PolyCellItemIntersectOp: public PolyCellItemIntersectBaseOp<PolyCellItemIntersectOp> {
	inline bool intersects(uint32_t itemId) {
		sserialize::Static::spatial::GeoShape gs( store.geoShape(itemId) );
		switch(gs.type()) {
		case sserialize::spatial::GS_POINT:
			return sgp.contains(*gs.get<sserialize::Static::spatial::GeoPoint>());
		case sserialize::spatial::GS_WAY:
			return sgp.intersects(*gs.get<sserialize::Static::spatial::GeoWay>());
		case sserialize::spatial::GS_POLYGON:
			return sgp.intersects(*gs.get<sserialize::Static::spatial::GeoWay>());
		case sserialize::spatial::GS_MULTI_POLYGON:
			return gs.get<sserialize::Static::spatial::GeoMultiPolygon>()->intersects(sgp);
		default:
			return false;
		};
	}
	PolyCellItemIntersectOp(const sserialize::spatial::GeoPolygon & gp,
				const sserialize::Static::spatial::GeoPolygon & sgp,
				const sserialize::Static::spatial::GeoHierarchy & gh,
				const liboscar::Static::OsmKeyValueObjectStore & store,
				const sserialize::Static::ItemIndexStore & idxStore,
				std::unordered_set<uint32_t> & fullMatches,
				std::map<uint32_t, sserialize::ItemIndex> & partialMatches) :
	PolyCellItemIntersectBaseOp(gp, sgp, gh, store, idxStore, fullMatches, partialMatches)
	{}
};

}//end namespace CQRFromPolygonHelpers

template<typename T_OPERATOR>
sserialize::CellQueryResult CQRFromPolygon::intersectingCellsPolygonItem(const sserialize::spatial::GeoPolygon & gp) const {
	//use a hash and map here since this operation is very expensive anyway
	sserialize::Static::spatial::GeoPolygon sgp(toStatic(gp));
	
	std::unordered_set<uint32_t> fullMatches;
	std::map<uint32_t, sserialize::ItemIndex> partialMatches;
	
	T_OPERATOR myOp(gp, sgp, m_store.geoHierarchy(), m_store, idxStore(), fullMatches, partialMatches);

	visit(gp, sgp, myOp);
	std::vector<uint32_t> fullMatchesSorted(fullMatches.begin(), fullMatches.end());
	std::sort(fullMatchesSorted.begin(), fullMatchesSorted.end());
	
	std::vector<uint32_t> partialMatchesSorted(partialMatches.size());
	std::vector<sserialize::ItemIndex> partialMatchesIdx;
	for(std::pair<const uint32_t, sserialize::ItemIndex> & p : partialMatches) {
		partialMatchesSorted[partialMatchesIdx.size()] = p.first;
		partialMatchesIdx.emplace_back(std::move(p.second));
	}
	
	sserialize::ItemIndex fmIdx(std::move(fullMatchesSorted));
	sserialize::ItemIndex pmIdx(std::move(partialMatchesSorted));
	return sserialize::CellQueryResult(fmIdx, pmIdx, partialMatchesIdx.cbegin(), geoHierarchy(), idxStore());
};

}}//end namespace liboscar::detail

#endif