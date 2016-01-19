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
	sserialize::ItemIndex compassOp(const sserialize::CellQueryResult & cqr, UnaryOp direction) const;
	sserialize::ItemIndex betweenOp(const sserialize::CellQueryResult & cqr1, const sserialize::CellQueryResult & cqr2) const;
private:
	sserialize::RCPtrWrapper<detail::CQRFromComplexSpatialQuery> m_priv;
};

namespace detail {

class CQRFromComplexSpatialQuery: public sserialize::RefCountObject {
public:
	CQRFromComplexSpatialQuery(const sserialize::spatial::GeoHierarchySubSetCreator& ssc, const liboscar::CQRFromPolygon& cqrfp);
	virtual ~CQRFromComplexSpatialQuery();
	sserialize::ItemIndex compassOp(const sserialize::CellQueryResult& cqr, liboscar::CQRFromComplexSpatialQuery::UnaryOp direction) const;
	sserialize::ItemIndex betweenOp(const sserialize::CellQueryResult & cqr1, const sserialize::CellQueryResult & cqr2) const;
private:
	sserialize::spatial::GeoHierarchySubSetCreator m_ssc;
	liboscar::CQRFromPolygon m_cqrfp;
};

}}//end namespace liboscar::detail

#endif