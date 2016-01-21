#include "CQRFromComplexSpatialQuery.h"
#include <sserialize/spatial/LatLonCalculations.h>

namespace liboscar {

CQRFromComplexSpatialQuery::CQRFromComplexSpatialQuery(const sserialize::spatial::GeoHierarchySubSetCreator & ssc, const CQRFromPolygon & cqrfp) :
m_priv(new detail::CQRFromComplexSpatialQuery(ssc, cqrfp))
{}

CQRFromComplexSpatialQuery::CQRFromComplexSpatialQuery(const CQRFromComplexSpatialQuery& other) :
m_priv(other.m_priv)
{}

CQRFromComplexSpatialQuery::~CQRFromComplexSpatialQuery() {}

const liboscar::CQRFromPolygon & CQRFromComplexSpatialQuery::cqrfp() const {
	return m_priv->cqrfp();
}

sserialize::ItemIndex CQRFromComplexSpatialQuery::compassOp(const sserialize::CellQueryResult & cqr, UnaryOp direction) const {
	return m_priv->compassOp(cqr, direction);
}

sserialize::ItemIndex CQRFromComplexSpatialQuery::betweenOp(const sserialize::CellQueryResult & cqr1, const sserialize::CellQueryResult & cqr2) const {
	return m_priv->betweenOp(cqr1, cqr2);
}

namespace detail {

CQRFromComplexSpatialQuery::CQRFromComplexSpatialQuery(const sserialize::spatial::GeoHierarchySubSetCreator & ssc, const liboscar::CQRFromPolygon & cqrfp) :
m_ssc(ssc),
m_cqrfp(cqrfp)
{}

CQRFromComplexSpatialQuery::~CQRFromComplexSpatialQuery() {}

const liboscar::CQRFromPolygon & CQRFromComplexSpatialQuery::cqrfp() const {
	return m_cqrfp;
}

const sserialize::Static::ItemIndexStore& CQRFromComplexSpatialQuery::idxStore() const {
	return m_cqrfp.idxStore();
}

sserialize::ItemIndex CQRFromComplexSpatialQuery::betweenOp(const sserialize::CellQueryResult& cqr1, const sserialize::CellQueryResult& cqr2) const {
	SubSet s1(createSubSet(cqr1)), s2(createSubSet(cqr2));
	SubSet::NodePtr np1(determineRelevantRegion(s1)), np2(determineRelevantRegion(s2));
	if (!np1.get() || !np2.get() || np1->ghId() == np2->ghId()) {
		return sserialize::ItemIndex();
	}
	
	sserialize::spatial::GeoRect rect1(m_cqrfp.geoHierarchy().regionBoundary(np1->ghId()));
	sserialize::spatial::GeoRect rect2(m_cqrfp.geoHierarchy().regionBoundary(np2->ghId()));
	
	//real bearing may be the wrong bearing here
	double bearing = sserialize::spatial::bearingTo(rect1.midLat(), rect1.midLon(), rect2.midLat(), rect2.midLon());
// 	bearing = ::sin(bearing);
	
	std::vector<sserialize::spatial::GeoPoint> gp;
	
	//we distinguish 8 different cases, bearing is the angle to the north
	if (bearing > (360.0-22.5) || bearing < 22.5) { //north
		//used points: mid points of the boundaries
		gp.emplace_back(rect1.midLat(), rect1.minLon()); //lower left
		gp.emplace_back(rect2.midLat(), rect2.minLon()); //upper left
		gp.emplace_back(rect2.midLat(), rect2.maxLon()); //upper right
		gp.emplace_back(rect1.midLat(), rect1.maxLon()); //lower right
	}
	else if (bearing < (45.0+22.5)) { //north-east
		//used points: diagonal points of the bbox
		gp.emplace_back(rect1.minLat(), rect1.maxLon()); //lower left
		gp.emplace_back(rect1.maxLat(), rect1.minLon()); //upper left
		gp.emplace_back(rect2.maxLat(), rect2.minLon()); //upper right
		gp.emplace_back(rect2.minLat(), rect2.maxLon()); //lower right
	}
	else if (bearing < (90.0+22.5)) { //east
		gp.emplace_back(rect1.minLat(), rect1.midLon()); //lower left
		gp.emplace_back(rect1.maxLat(), rect1.midLon()); //upper left
		gp.emplace_back(rect2.maxLat(), rect2.midLon()); //upper right
		gp.emplace_back(rect2.minLat(), rect2.midLon()); //lower right
	}
	else if (bearing < 135.0+22.5) { //south-east
		gp.emplace_back(rect1.minLat(), rect1.minLon()); //lower left
		gp.emplace_back(rect1.maxLat(), rect1.maxLon()); //upper left
		gp.emplace_back(rect2.maxLat(), rect2.maxLon()); //upper right 
		gp.emplace_back(rect2.minLat(), rect2.minLon()); //lower left
	}
	else if (bearing < (180.0+22.5)) { //south
		gp.emplace_back(rect2.midLat(), rect2.minLon()); //lower left
		gp.emplace_back(rect1.midLat(), rect1.minLon()); //upper left
		gp.emplace_back(rect1.midLat(), rect1.maxLon()); //upper right
		gp.emplace_back(rect2.midLat(), rect2.maxLon()); //lower right
	}
	else if (bearing < (225.0+22.5)) { //south-west
		gp.emplace_back(rect2.minLat(), rect2.maxLon()); //lower left
		gp.emplace_back(rect2.maxLat(), rect2.minLon()); //upper left
		gp.emplace_back(rect1.maxLat(), rect1.minLon()); //upper right
		gp.emplace_back(rect1.minLat(), rect1.maxLon()); //lower right
	}
	else if (bearing < (270.0+22.5)) { //west
		gp.emplace_back(rect2.minLat(), rect2.midLon()); //lower left
		gp.emplace_back(rect2.maxLat(), rect2.midLon()); //upper left
		gp.emplace_back(rect1.maxLat(), rect1.midLon()); //upper right
		gp.emplace_back(rect1.minLat(), rect1.midLon()); //lower right
	}
	else { //north-west
		gp.emplace_back(rect2.minLat(), rect2.minLon()); //lower left
		gp.emplace_back(rect2.maxLat(), rect2.maxLon()); //upper left
		gp.emplace_back(rect1.maxLat(), rect1.maxLon()); //upper right
		gp.emplace_back(rect1.minLat(), rect1.minLon()); //lower right
	}
	
	//normalize points
	for(sserialize::spatial::GeoPoint & p : gp) {
		p.normalize(sserialize::spatial::GeoPoint::NT_CLIP);
	}
	
	sserialize::ItemIndex tmp(m_cqrfp.intersectingCells(sserialize::spatial::GeoPolygon(gp), liboscar::CQRFromPolygon::AC_POLYGON_CELL_BBOX));
	//now remove the cells that are part of the input regions
	tmp = tmp - idxStore().at(m_cqrfp.geoHierarchy().regionCellIdxPtr(np1->ghId()));
	tmp = tmp - idxStore().at(m_cqrfp.geoHierarchy().regionCellIdxPtr(np2->ghId()));
	return tmp;
}

//todo: clip
sserialize::ItemIndex CQRFromComplexSpatialQuery::compassOp(const sserialize::CellQueryResult& cqr, liboscar::CQRFromComplexSpatialQuery::UnaryOp direction) const {
	double inDirectionScale = 2.0; //in direction of compass
	double orthoToDirectionScale = 0.5; //orthogonal to compass direction

	SubSet subSet( createSubSet(cqr) );
	SubSet::NodePtr myRegion( determineRelevantRegion(subSet) );
	if (!myRegion.get()) {
		return sserialize::ItemIndex();
	}
	//now construct the polygon
	std::vector<sserialize::spatial::GeoPoint> gp;
	sserialize::spatial::GeoRect rect(m_cqrfp.geoHierarchy().regionBoundary(myRegion->ghId()));
	double latDist = rect.maxLat() - rect.minLat();
	double lonDist = rect.maxLon()-rect.minLon();
	
	switch (direction) {
	case liboscar::CQRFromComplexSpatialQuery::UO_NORTH_OF:
	{
		double minLat = rect.minLat() + latDist/2.0;
		double maxLat = minLat + latDist*inDirectionScale;
		gp.emplace_back(minLat, rect.minLon()); //lower left
		gp.emplace_back(maxLat, rect.minLon()-lonDist*orthoToDirectionScale); //upper left
		gp.emplace_back(maxLat, rect.maxLon()+lonDist*orthoToDirectionScale); //upper right
		gp.emplace_back(minLat, rect.maxLon()); //lower right
		break;
	}
	case liboscar::CQRFromComplexSpatialQuery::UO_EAST_OF:
	{
		double minLon = rect.minLon() + lonDist/2.0;
		double maxLon = minLon + lonDist*inDirectionScale;
		gp.emplace_back(rect.minLat(), minLon);//lower left
		gp.emplace_back(rect.maxLat(), minLon); //upper left
		gp.emplace_back(rect.maxLat()+latDist*orthoToDirectionScale, maxLon); //upper right
		gp.emplace_back(rect.minLat()-latDist*orthoToDirectionScale, maxLon);//lower right
		break;
	}
	case liboscar::CQRFromComplexSpatialQuery::UO_SOUTH_OF:
	{
		double maxLat = rect.minLat() + latDist/2.0;
		double minLat = rect.minLat() - latDist*inDirectionScale;
		double lonDist = rect.maxLon() - rect.minLon();
		gp.emplace_back(minLat, rect.minLon()-lonDist*orthoToDirectionScale); //lower left
		gp.emplace_back(maxLat, rect.minLon()); //upper left
		gp.emplace_back(maxLat, rect.maxLon()); //upper right
		gp.emplace_back(minLat, rect.maxLon()+lonDist*orthoToDirectionScale); //lower right
		break;
	}
	case liboscar::CQRFromComplexSpatialQuery::UO_WEST_OF:
	{
		double minLon = rect.minLon() + lonDist/2.0;
		double maxLon = minLon + lonDist*inDirectionScale;
		gp.emplace_back(rect.minLat()-latDist*orthoToDirectionScale, minLon);//lower left
		gp.emplace_back(rect.maxLat()+latDist*orthoToDirectionScale, minLon); //upper left
		gp.emplace_back(rect.maxLat(), maxLon); //upper right
		gp.emplace_back(rect.minLat(), maxLon);//lower right
		break;
	}
	default:
		return sserialize::ItemIndex();
	};
	//normalize points
	for(sserialize::spatial::GeoPoint & p : gp) {
		p.normalize(sserialize::spatial::GeoPoint::NT_CLIP);
	}
	
	sserialize::ItemIndex tmp(m_cqrfp.intersectingCells(sserialize::spatial::GeoPolygon(gp), liboscar::CQRFromPolygon::AC_POLYGON_CELL_BBOX));
	tmp = tmp - idxStore().at(m_cqrfp.geoHierarchy().regionCellIdxPtr(myRegion->ghId()));
	return tmp;
}

detail::CQRFromComplexSpatialQuery::SubSet
CQRFromComplexSpatialQuery::createSubSet(const sserialize::CellQueryResult cqr) const {
	return m_ssc.subSet(cqr, true);
}

detail::CQRFromComplexSpatialQuery::SubSet::NodePtr
CQRFromComplexSpatialQuery::determineRelevantRegion(const detail::CQRFromComplexSpatialQuery::SubSet& subset) const {
	//now walk the subset and try to find the region where most hits are inside
	//we can use the ohPath option here (TODO: add ability to set this from outside)
	
	double fraction = 0.95;
	bool globalUnfoldRatio = true;
	SubSet::NodePtr nodePtr;
	struct MyIterator {
		SubSet::NodePtr & nodePtr;
		MyIterator & operator*() { return *this; }
		MyIterator & operator++() { return *this; }
		MyIterator & operator=(const SubSet::NodePtr & node) { nodePtr = node; }
		MyIterator(SubSet::NodePtr & nodePtr) : nodePtr(nodePtr) {}
	};
	MyIterator myIt(nodePtr);
	subset.pathToBranch<MyIterator>(myIt, fraction, globalUnfoldRatio);
	return nodePtr;
}

}}//end namespace liboscar::detail