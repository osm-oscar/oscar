#ifndef LIBOSCAR_CQR_FROM_COMPLEX_SPATIAL_QUERY_H
#define LIBOSCAR_CQR_FROM_COMPLEX_SPATIAL_QUERY_H
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/Static/GeoHierarchySubSetCreator.h>
#include "CQRFromPolygon.h"

namespace liboscar {
namespace detail {

class CQRFromComplexSpatialQuery;

}//end namespace detail

class CQRFromComplexSpatialQuery final {
public:
	enum UnaryOp : uint32_t {UO_INVALID=0, UO_NORTH_OF, UO_EAST_OF, UO_SOUTH_OF, UO_WEST_OF};
	enum BinaryOp : uint32_t {BO_INVALID=0, BO_BETWEEN};
public:
	CQRFromComplexSpatialQuery(const CQRFromComplexSpatialQuery & other);
	CQRFromComplexSpatialQuery(const sserialize::spatial::GeoHierarchySubSetCreator & ssc, const CQRFromPolygon & cqrfp);
	~CQRFromComplexSpatialQuery();
	sserialize::CellQueryResult compassOp(const sserialize::CellQueryResult & cqr, UnaryOp direction) const;
	sserialize::CellQueryResult betweenOp(const sserialize::CellQueryResult & cqr1, const sserialize::CellQueryResult & cqr2) const;
	const liboscar::CQRFromPolygon & cqrfp() const;
private:
	sserialize::RCPtrWrapper<detail::CQRFromComplexSpatialQuery> m_priv;
};

namespace detail {

class CQRFromComplexSpatialQuery: public sserialize::RefCountObject {
public:
	typedef sserialize::Static::spatial::GeoHierarchy::SubSet SubSet;
public:
	CQRFromComplexSpatialQuery(const sserialize::spatial::GeoHierarchySubSetCreator& ssc, const liboscar::CQRFromPolygon& cqrfp);
	virtual ~CQRFromComplexSpatialQuery();
	sserialize::CellQueryResult compassOp(const sserialize::CellQueryResult& cqr, liboscar::CQRFromComplexSpatialQuery::UnaryOp direction) const;
	sserialize::CellQueryResult betweenOp(const sserialize::CellQueryResult & cqr1, const sserialize::CellQueryResult & cqr2) const;
	const liboscar::CQRFromPolygon & cqrfp() const;
public: //cqr creation
	//uses auto-detection of accuracy
	sserialize::CellQueryResult cqrFromPolygon(const sserialize::spatial::GeoPolygon & gp) const;
private://polygon creation functions
	///between 0->360, north is at 0
	double bearing(double fromLat, double fromLon, double toLat, double toLon) const;
	void normalize(std::vector<sserialize::spatial::GeoPoint> & gp) const;
	void createPolygon(const sserialize::spatial::GeoRect & rect1, const sserialize::spatial::GeoRect & rect2, std::vector<sserialize::spatial::GeoPoint> & gp) const;
private:
	SubSet createSubSet(const sserialize::CellQueryResult cqr) const;
	SubSet::NodePtr determineRelevantRegion(const SubSet & subset) const;
	///@return itemId
	uint32_t determineRelevantItem(const SubSet & subSet, const SubSet::NodePtr & rPtr) const;
private: //accessor function
	const liboscar::Static::OsmKeyValueObjectStore & store() const;
	const sserialize::Static::spatial::GeoHierarchy & geoHierarchy() const;
	const sserialize::Static::ItemIndexStore & idxStore() const;
private:
	sserialize::spatial::GeoHierarchySubSetCreator m_ssc;
	liboscar::CQRFromPolygon m_cqrfp;
};

}}//end namespace liboscar::detail

#endif