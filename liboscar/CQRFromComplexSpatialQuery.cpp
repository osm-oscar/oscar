#include "CQRFromComplexSpatialQuery.h"
#include <sserialize/spatial/LatLonCalculations.h>

//CGAL stuff
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/convex_hull_traits_2.h>
#include <CGAL/ch_graham_andrew.h>

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

sserialize::CellQueryResult CQRFromComplexSpatialQuery::compassOp(const sserialize::CellQueryResult & cqr, UnaryOp direction) const {
	return m_priv->compassOp(cqr, direction);
}

sserialize::CellQueryResult CQRFromComplexSpatialQuery::betweenOp(const sserialize::CellQueryResult& cqr1, const sserialize::CellQueryResult& cqr2) const {
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

const liboscar::Static::OsmKeyValueObjectStore& CQRFromComplexSpatialQuery::store() const {
	return m_cqrfp.store();
}

const sserialize::Static::spatial::GeoHierarchy & CQRFromComplexSpatialQuery::geoHierarchy() const {
	return m_cqrfp.geoHierarchy();
}

const sserialize::Static::ItemIndexStore & CQRFromComplexSpatialQuery::idxStore() const {
	return m_cqrfp.idxStore();
}

//the helper funtionts

sserialize::CellQueryResult CQRFromComplexSpatialQuery::cqrFromPolygon(const sserialize::spatial::GeoPolygon & gp) const {
	return cqrfp().cqr(gp, liboscar::CQRFromPolygon::AC_AUTO);
}

//Now the polygon creation functions

double CQRFromComplexSpatialQuery::bearing(double fromLat, double fromLon, double toLat, double toLon) const {
	return sserialize::spatial::bearingTo(fromLat, fromLon, toLat, toLon);
}

void CQRFromComplexSpatialQuery::normalize(std::vector< sserialize::spatial::GeoPoint >& gp) const {
	//normalize points
	for(sserialize::spatial::GeoPoint & p : gp) {
		p.normalize(sserialize::spatial::GeoPoint::NT_CLIP);
	}
}

void 
CQRFromComplexSpatialQuery::
createPolygon(const sserialize::spatial::GeoPoint& p1, const sserialize::spatial::GeoPoint& p2, std::vector< sserialize::spatial::GeoPoint >& pp) const {
	sserialize::spatial::GeoPoint mp, tp;
	sserialize::spatial::midPoint(p1.lat(), p1.lon(), p2.lat(), p2.lon(), mp.lat(), mp.lon());
	double dist = sserialize::spatial::distanceTo(p1.lat(), p1.lon(), p2.lat(), p2.lon())/4.0; //half-diameter ellipse
	double myBearing = sserialize::spatial::bearingTo(mp.lat(), mp.lon(), p2.lat(), p2.lon());
	
	double destBearing = ::fmod(myBearing+90.0, 360.0);
	sserialize::spatial::destinationPoint(mp.lat(), mp.lon(), destBearing, dist, tp.lat(), tp.lon());
	
	pp.emplace_back(p1);
	pp.emplace_back(mp);
	pp.emplace_back(p2);
	
	destBearing = ::fmod(myBearing+270.0, 360.0);
	sserialize::spatial::destinationPoint(mp.lat(), mp.lon(), destBearing, dist, tp.lat(), tp.lon());
	
	pp.emplace_back(mp);
	pp.emplace_back(p1);
	this->normalize(pp);
}

struct CHFromPoints {
	typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
	typedef CGAL::Convex_hull_traits_2<K> Traits;
	typedef K::Point_2 Point_2;
	struct MyOutputIterator {
		std::vector<sserialize::spatial::GeoPoint> & pp;
		MyOutputIterator & operator++() { return *this;}
		MyOutputIterator & operator++(int) { return *this; }
		MyOutputIterator & operator*() { return *this; }
		MyOutputIterator & operator=(const Point_2 & p) {
			pp.emplace_back(p.x(), p.y());
			return *this;
		}
		MyOutputIterator(std::vector<sserialize::spatial::GeoPoint> & pp) : pp(pp) {}
	};
	template<typename T_RANDOM_ACCESS_ITERATOR>
	static void calc(T_RANDOM_ACCESS_ITERATOR begin, T_RANDOM_ACCESS_ITERATOR end, std::vector<sserialize::spatial::GeoPoint> & pp) {
		CGAL::ch_graham_andrew( begin, end, MyOutputIterator(pp), Traits());
	}
};

void 
CQRFromComplexSpatialQuery::
createPolygon(
	const sserialize::Static::spatial::GeoWay& gw1,
	const sserialize::Static::spatial::GeoWay& gw2,
	std::vector<sserialize::spatial::GeoPoint> & pp) const
{
	typedef CHFromPoints::Point_2 Point_2;
	std::vector<Point_2> points;
	points.reserve(gw1.size()+gw2.size());
	for(sserialize::spatial::GeoPoint x : gw1) {
		points.emplace_back(x.lat(), x.lon());
	}
	for(sserialize::spatial::GeoPoint x : gw2) {
		points.emplace_back(x.lat(), x.lon());
	}
	CHFromPoints::calc(points.begin(), points.end(), pp);
	this->normalize(pp);
}


void
CQRFromComplexSpatialQuery::
createPolygon(
	const sserialize::spatial::GeoRect& rect1,
	const sserialize::spatial::GeoRect& rect2,
	std::vector<sserialize::spatial::GeoPoint> &gp) const
{
	//real bearing may be the wrong bearing here
	double bearing = this->bearing(rect1.midLat(), rect1.midLon(), rect2.midLat(), rect2.midLon());
	
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
	this->normalize(gp);
}

void 
CQRFromComplexSpatialQuery::
createPolygon(
	const sserialize::Static::spatial::GeoWay& gw,
	const sserialize::Static::spatial::GeoPoint& gp,
	std::vector< sserialize::spatial::GeoPoint >& pp) const
{
	pp.emplace_back(gp);
	pp.insert(pp.end(), gw.cbegin(), gw.cend());
	pp.emplace_back(gp);
	this->normalize(pp);
}

void
CQRFromComplexSpatialQuery::
createPolygon(
	const sserialize::spatial::GeoRect & polyRect,
	const sserialize::spatial::GeoPoint & point,
	std::vector< sserialize::spatial::GeoPoint >& pp) const
{
	//real bearing may be the wrong bearing here
	double bearing = this->bearing(polyRect.midLat(), polyRect.midLon(), point.lat(), point.lon());
	
	//we distinguish 8 different cases, bearing is the angle to the north
	if (bearing > (360.0-22.5) || bearing < 22.5) { //north
		pp.emplace_back(polyRect.midLat(), polyRect.minLon());
		pp.emplace_back(polyRect.maxLat(), polyRect.minLon());
		pp.emplace_back(point);
		pp.emplace_back(polyRect.maxLat(), polyRect.maxLon());
		pp.emplace_back(polyRect.midLat(), polyRect.maxLon());
	}
	else if (bearing < (45.0+22.5)) { //north-east
		pp.emplace_back(polyRect.maxLat(), polyRect.minLon());
		pp.emplace_back(point);
		pp.emplace_back(polyRect.minLat(), polyRect.maxLon());
	}
	else if (bearing < (90.0+22.5)) { //east
		pp.emplace_back(polyRect.maxLat(), polyRect.midLon());
		pp.emplace_back(polyRect.maxLat(), polyRect.maxLon());
		pp.emplace_back(point);
		pp.emplace_back(polyRect.minLat(), polyRect.maxLon());
		pp.emplace_back(polyRect.minLat(), polyRect.midLon());
	}
	else if (bearing < 135.0+22.5) { //south-east
		pp.emplace_back(polyRect.maxLat(), polyRect.maxLon());
		pp.emplace_back(point);
		pp.emplace_back(polyRect.minLat(), polyRect.minLon());
	}
	else if (bearing < (180.0+22.5)) { //south
		pp.emplace_back(point);
		pp.emplace_back(polyRect.minLat(), polyRect.minLon());
		pp.emplace_back(polyRect.midLat(), polyRect.minLon());
		pp.emplace_back(polyRect.midLat(), polyRect.maxLon());
		pp.emplace_back(polyRect.minLat(), polyRect.maxLon());
	}
	else if (bearing < (225.0+22.5)) { //south-west
		pp.emplace_back(polyRect.maxLat(), polyRect.minLon());
		pp.emplace_back(point);
		pp.emplace_back(polyRect.minLat(), polyRect.minLon());
	}
	else if (bearing < (270.0+22.5)) { //west
		pp.emplace_back(polyRect.minLat(), polyRect.midLon());
		pp.emplace_back(polyRect.minLat(), polyRect.minLon());
		pp.emplace_back(point);
		pp.emplace_back(polyRect.maxLat(), polyRect.minLon());
		pp.emplace_back(polyRect.maxLat(), polyRect.midLon());
	}
	else { //north-west
		pp.emplace_back(polyRect.minLat(), polyRect.minLon());
		pp.emplace_back(point);
		pp.emplace_back(polyRect.maxLat(), polyRect.maxLon());
	}
	this->normalize(pp);
}

void
CQRFromComplexSpatialQuery::
createPolygon(
	const sserialize::spatial::GeoRect& polyRect,
	const sserialize::Static::spatial::GeoWay& gw,
	std::vector< sserialize::spatial::GeoPoint >& pp) const
{
	typedef CHFromPoints::Point_2 Point_2;
	std::vector<Point_2> points;
	points.reserve(4+gw.size());
	for(sserialize::spatial::GeoPoint x : gw) {
		points.emplace_back(x.lat(), x.lon());
	}
	points.emplace_back(polyRect.minLat(), polyRect.minLon());
	points.emplace_back(polyRect.maxLat(), polyRect.minLon());
	points.emplace_back(polyRect.maxLat(), polyRect.maxLon());
	points.emplace_back(polyRect.minLat(), polyRect.maxLon());
	CHFromPoints::calc(points.begin(), points.end(), pp);
	this->normalize(pp);
}

//there are 3 cases:
// region<->item or item<->region
// region<->region
// item<->item
// set accuracy depending on the size of the rectangle to be?:
// rectangle diagonal less thant 500m -> POLYGON_ITEM

#define ITEM_QUERY_ITEM_THRESHOLD 100
#define ITEM_QUERY_CELL_THREADSHOLD 2

sserialize::CellQueryResult CQRFromComplexSpatialQuery::betweenOp(const sserialize::CellQueryResult& cqr1, const sserialize::CellQueryResult& cqr2) const {
	SubSet s1(createSubSet(cqr1)), s2(createSubSet(cqr2));
	SubSet::NodePtr np1(determineRelevantRegion(s1)), np2(determineRelevantRegion(s2));
	if (!np1.get() || !np2.get() || np1->ghId() == np2->ghId()) {
		return sserialize::CellQueryResult();
	}
	
	//not many items, interpret as item query? but how?
	//just use first item? or the item with the highest dimension
	//That would select buildings instead of streets
	//or sum them all up into a single big rectangle?
	//construct the convex hull?
	
	bool cqr1IsItemQuery = cqr1.cellCount() < ITEM_QUERY_CELL_THREADSHOLD && np1->maxItemsSize() < ITEM_QUERY_ITEM_THRESHOLD;
	bool cqr2IsItemQuery = cqr2.cellCount() < ITEM_QUERY_CELL_THREADSHOLD && np2->maxItemsSize() < ITEM_QUERY_ITEM_THRESHOLD;
	
	if (cqr1IsItemQuery && cqr2IsItemQuery) {
		//lets find the item
		uint32_t itemId1 = determineRelevantItem(s1, np1);
		uint32_t itemId2 = determineRelevantItem(s2, np2);
		auto shape1(store().geoShape(itemId1));
		auto shape2(store().geoShape(itemId2));
		auto st1(shape1.type());
		auto st2(shape2.type());
		//polygon construction depends on the involved types
		//point-point need a diamond (ellipse)
		//way-way use both ways as part of the polygon and connect their endpoints
		//poly-poly use bbox of polygons
		//point-way use the way as one part of the polygon and the point as another point
		//point-poly use bbox of poly and the point
		//way-poly use way and bbox of poly

		//the accuracy depends on the size of the polygon
		std::vector<sserialize::spatial::GeoPoint> pp;
		if (st1 == sserialize::spatial::GS_POINT && st2 == sserialize::spatial::GS_POINT) { //point-point
			createPolygon(
				*shape1.get<sserialize::Static::spatial::GeoPoint>(),
				*shape2.get<sserialize::Static::spatial::GeoPoint>(),
				pp
			);
		}
		else if (st1 == sserialize::spatial::GS_WAY && st2 == sserialize::spatial::GS_WAY) { //way-way
			createPolygon(
				*shape1.get<sserialize::Static::spatial::GeoWay>(),
				*shape2.get<sserialize::Static::spatial::GeoWay>(),
				pp
			);
		}
		else if ((st1 == sserialize::spatial::GS_POLYGON || st1 == sserialize::spatial::GS_MULTI_POLYGON) &&
					(st2 == sserialize::spatial::GS_POLYGON || st2 == sserialize::spatial::GS_MULTI_POLYGON)) // poly-poly
		{
			createPolygon(
				shape1.boundary(),
				shape2.boundary(),
				pp
			);
		}
		else if (st1 == sserialize::spatial::GS_POINT && st2 == sserialize::spatial::GS_WAY) { //point-way
			createPolygon(
				*shape2.get<sserialize::Static::spatial::GeoWay>(),
				*shape1.get<sserialize::Static::spatial::GeoPoint>(),
				pp
			);
		}
		else if (st1 == sserialize::spatial::GS_WAY && st2 == sserialize::spatial::GS_POINT) { //way-point
			createPolygon(
				*shape1.get<sserialize::Static::spatial::GeoWay>(),
				*shape2.get<sserialize::Static::spatial::GeoPoint>(),
				pp
			);
		}
		else if (st1 == sserialize::spatial::GS_POINT && (st2 == sserialize::spatial::GS_POLYGON || st2 == sserialize::spatial::GS_MULTI_POLYGON)) { //point-poly
			createPolygon(
				shape2.boundary(),
				*shape1.get<sserialize::Static::spatial::GeoPoint>(),
				pp
			);
		}
		else if ((st1 == sserialize::spatial::GS_POLYGON || st1 == sserialize::spatial::GS_MULTI_POLYGON) && st2 == sserialize::spatial::GS_POINT) {
			createPolygon(
				shape1.boundary(),
				*shape2.get<sserialize::Static::spatial::GeoPoint>(),
				pp);
		}
		else if (st1 == sserialize::spatial::GS_WAY && (st2 == sserialize::spatial::GS_POLYGON || st2 == sserialize::spatial::GS_MULTI_POLYGON)) {
			createPolygon(
				shape2.boundary(),
				*shape1.get<sserialize::Static::spatial::GeoWay>(),
				pp
			);
		}
		else if ((st1 == sserialize::spatial::GS_POLYGON || st1 == sserialize::spatial::GS_MULTI_POLYGON) && st2 == sserialize::spatial::GS_WAY) {
			createPolygon(
				shape1.boundary(),
				*shape2.get<sserialize::Static::spatial::GeoWay>(),
				pp
			);
		}
		else {
			createPolygon(
				shape1.boundary(),
				shape2.boundary(),
				pp
			);
		}
		
		return cqrFromPolygon( sserialize::spatial::GeoPolygon(pp) );
	}
	else if (cqr1IsItemQuery || cqr2IsItemQuery) {
		//item <-> region, just use the bounding box of the item and the bounding box of the region
		sserialize::spatial::GeoRect rect1, rect2;
		if (cqr1IsItemQuery) {
			uint32_t itemId = determineRelevantItem(s1, np1);
			rect1 = store().geoShape(itemId).boundary();
			rect2 = geoHierarchy().regionBoundary(np2->ghId());
		}
		else {
			uint32_t itemId = determineRelevantItem(s2, np2);
			rect1 = store().geoShape(itemId).boundary();
			rect2 = geoHierarchy().regionBoundary(np1->ghId());
		}
		std::vector<sserialize::spatial::GeoPoint> gp;
		createPolygon(rect1, rect2, gp);
		return cqrFromPolygon(sserialize::spatial::GeoPolygon(gp));
	}
	else {
		std::vector<sserialize::spatial::GeoPoint> gp;
		createPolygon(geoHierarchy().regionBoundary(np1->ghId()), geoHierarchy().regionBoundary(np2->ghId()), gp);
		
		sserialize::ItemIndex tmp(cqrfp().fullMatches(sserialize::spatial::GeoPolygon(gp), liboscar::CQRFromPolygon::AC_POLYGON_CELL_BBOX));
		//now remove the cells that are part of the input regions
		tmp = tmp - idxStore().at(geoHierarchy().regionCellIdxPtr(np1->ghId()));
		tmp = tmp - idxStore().at(geoHierarchy().regionCellIdxPtr(np2->ghId()));
		return sserialize::CellQueryResult(tmp, geoHierarchy(), idxStore());
	}
}

//todo: clip
sserialize::CellQueryResult CQRFromComplexSpatialQuery::compassOp(const sserialize::CellQueryResult& cqr, liboscar::CQRFromComplexSpatialQuery::UnaryOp direction) const {
	if (cqr.cellCount() == 0) {
		return sserialize::CellQueryResult();
	}
	
	double inDirectionScale = 2.0; //in direction of compass
	double orthoToDirectionScale = 0.5; //orthogonal to compass direction

	SubSet subSet( createSubSet(cqr) );
	SubSet::NodePtr myRegion( determineRelevantRegion(subSet) );
	if (!myRegion.get()) {
		return sserialize::CellQueryResult();
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
		return sserialize::CellQueryResult();
	};
	//normalize points
	normalize(gp);
	
	sserialize::ItemIndex tmp(m_cqrfp.fullMatches(sserialize::spatial::GeoPolygon(gp), liboscar::CQRFromPolygon::AC_POLYGON_CELL_BBOX));
	tmp = tmp - idxStore().at(m_cqrfp.geoHierarchy().regionCellIdxPtr(myRegion->ghId()));
	return sserialize::CellQueryResult(tmp, geoHierarchy(), idxStore());
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
		MyIterator & operator=(const SubSet::NodePtr & node) { nodePtr = node; return *this;}
		MyIterator(SubSet::NodePtr & nodePtr) : nodePtr(nodePtr) {}
	};
	MyIterator myIt(nodePtr);
	subset.pathToBranch<MyIterator>(myIt, fraction, globalUnfoldRatio);
	return nodePtr;
}

uint32_t CQRFromComplexSpatialQuery::determineRelevantItem(const SubSet & subSet, const SubSet::NodePtr & rPtr) const {
	sserialize::ItemIndex items( subSet.idx(rPtr) );
	double resDiag = 0.0;
	uint32_t resId = liboscar::Static::OsmKeyValueObjectStore::npos;
	for(uint32_t itemId : items) {
		auto type = store().geoShapeType(itemId);
		switch (type) {
		case sserialize::spatial::GS_POINT:
		{
			if (resDiag == 0.0) {
				resId = itemId;
			}
			break;
		}
		case sserialize::spatial::GS_WAY:
		{
			return itemId;
		}
		case sserialize::spatial::GS_POLYGON:
		{
			double tmp = store().geoShape(itemId).boundary().diagInM();
			if (resDiag < tmp) {
				resId = itemId;
				resDiag = tmp;
			}
			break;
		}
		default:
			break;
		};
	}
	return resId;
}

}}//end namespace liboscar::detail