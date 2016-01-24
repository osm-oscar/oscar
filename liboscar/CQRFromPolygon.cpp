#include "CQRFromPolygon.h"

namespace liboscar {

CQRFromPolygon::CQRFromPolygon(const CQRFromPolygon & other) :
m_priv(other.m_priv)
{}

CQRFromPolygon::CQRFromPolygon(const Static::OsmKeyValueObjectStore & store, const sserialize::Static::ItemIndexStore & idxStore) :
m_priv(new detail::CQRFromPolygon(store, idxStore))
{}

CQRFromPolygon::~CQRFromPolygon() {}

const Static::OsmKeyValueObjectStore& CQRFromPolygon::store() const {
	return m_priv->store();
}

const sserialize::Static::spatial::GeoHierarchy & CQRFromPolygon::geoHierarchy() const {
	return m_priv->geoHierarchy();
}

const sserialize::Static::ItemIndexStore & CQRFromPolygon::idxStore() const {
	return m_priv->idxStore();
}

sserialize::ItemIndex CQRFromPolygon::fullMatches(const sserialize::spatial::GeoPolygon & gp, Accuracy ac) const {
	return m_priv->fullMatches(gp, ac);
}

sserialize::CellQueryResult CQRFromPolygon::cqr(const sserialize::spatial::GeoPolygon& gp, Accuracy ac) const {
	return m_priv->cqr(gp, ac);
}

namespace detail {

CQRFromPolygon::CQRFromPolygon(const Static::OsmKeyValueObjectStore& store, const sserialize::Static::ItemIndexStore & idxStore) :
m_store(store),
m_idxStore(idxStore)
{}

CQRFromPolygon::~CQRFromPolygon() {}

const Static::OsmKeyValueObjectStore& CQRFromPolygon::store() const {
	return m_store;
}

const sserialize::Static::spatial::GeoHierarchy& CQRFromPolygon::geoHierarchy() const {
	return m_store.geoHierarchy();
}

const sserialize::Static::ItemIndexStore & CQRFromPolygon::idxStore() const {
	return m_idxStore;
}

sserialize::ItemIndex CQRFromPolygon::fullMatches(const sserialize::spatial::GeoPolygon & gp, liboscar::CQRFromPolygon::Accuracy ac) const {
	switch (ac) {
	case liboscar::CQRFromPolygon::AC_POLYGON_CELL_BBOX:
	case liboscar::CQRFromPolygon::AC_POLYGON_CELL:
	case liboscar::CQRFromPolygon::AC_POLYGON_ITEM_BBOX:
	case liboscar::CQRFromPolygon::AC_POLYGON_ITEM:
	{
		return intersectingCellsPolygonCellBBox(gp);
		break;
	}
	case liboscar::CQRFromPolygon::AC_POLYGON_BBOX_CELL_BBOX:
	default:
		return m_store.geoHierarchy().intersectingCells(idxStore(), gp.boundary());
		break;
	};
}

sserialize::CellQueryResult CQRFromPolygon::cqr(const sserialize::spatial::GeoPolygon& gp, liboscar::CQRFromPolygon::Accuracy ac) const {
	switch (ac) {
	case liboscar::CQRFromPolygon::AC_POLYGON_BBOX_CELL_BBOX:
		return sserialize::CellQueryResult(m_store.geoHierarchy().intersectingCells(idxStore(), gp.boundary()), m_store.geoHierarchy(), idxStore());
	case liboscar::CQRFromPolygon::AC_POLYGON_CELL_BBOX:
	case liboscar::CQRFromPolygon::AC_POLYGON_CELL:
		return sserialize::CellQueryResult(intersectingCellsPolygonCellBBox(gp), m_store.geoHierarchy(), idxStore());
	case liboscar::CQRFromPolygon::AC_POLYGON_ITEM_BBOX:
		return intersectingCellsPolygonItem<detail::CQRFromPolygonHelpers::PolyCellItemBBoxIntersectOp>(gp);
	case liboscar::CQRFromPolygon::AC_POLYGON_ITEM:
		return intersectingCellsPolygonItem<detail::CQRFromPolygonHelpers::PolyCellItemIntersectOp>(gp);
	default:
		throw sserialize::InvalidEnumValueException("CQRFromPolygon::Accuracy does not have " + std::to_string(ac) + " as value");
		return sserialize::CellQueryResult();;
	};
}

sserialize::Static::spatial::GeoPolygon detail::CQRFromPolygon::toStatic(const sserialize::spatial::GeoPolygon& gp) const {
	//as of now intersection between static and non-static geometry is not available
	sserialize::UByteArrayAdapter d(sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
	gp.append(d);
	d.resetPtrs();
	return sserialize::Static::spatial::GeoPolygon(d);
}

sserialize::ItemIndex CQRFromPolygon::intersectingCellsPolygonCellBBox(const sserialize::spatial::GeoPolygon& gp) const {
	std::vector<uint32_t> intersectingCells;
	
	struct MyOperator {
		const sserialize::spatial::GeoPolygon & gp;
		const sserialize::Static::spatial::GeoHierarchy & gh;
		std::vector<uint32_t> & intersectingCells;
		
		void enclosed(const sserialize::ItemIndex & enclosedCells) {
			intersectingCells.insert(intersectingCells.end(), enclosedCells.cbegin(), enclosedCells.cend());
		}
		void candidates(const sserialize::ItemIndex & candidateCells) {
			for(uint32_t cellId : candidateCells) {
				if (gp.intersects(gh.cellBoundary(cellId))) {
					intersectingCells.push_back(cellId);
				}
			}
		}
		MyOperator(const sserialize::spatial::GeoPolygon & gp, const sserialize::Static::spatial::GeoHierarchy & gh, std::vector<uint32_t> & intersectingCells) :
		gp(gp), gh(gh), intersectingCells(intersectingCells)
		{}
	};
	MyOperator myOp(gp, m_store.geoHierarchy(), intersectingCells);

	visit(gp, toStatic(gp), myOp);

	std::sort(intersectingCells.begin(), intersectingCells.end());
	intersectingCells.resize(std::unique(intersectingCells.begin(), intersectingCells.end())-intersectingCells.begin());
	return sserialize::ItemIndex(std::move(intersectingCells));
}

}}//end namespace liboscar::detail