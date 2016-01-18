#ifndef LIBOSCAR_CQR_FROM_COMPLEX_SPATIAL_QUERY_H
#define LIBOSCAR_CQR_FROM_COMPLEX_SPATIAL_QUERY_H
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/Static/GeoHierarchy.h>

namespace liboscar {

class CQRFromComplexSpatialQuery {
public:
	enum UnaryOp : uint32_t {UO_INVALID=0, UO_NORTH_OF, UO_EAST_OF, UO_SOUTH_OF, UO_WEST_OF};
	enum BinaryOp : uint32_t {BO_INVALID=0, BO_BETWEEN};
public:
	CQRFromComplexSpatialQuery(const sserialize::Static::spatial::GeoHierarchy & gh);
	~CQRFromComplexSpatialQuery();
	sserialize::ItemIndex compassOp(const sserialize::CellQueryResult & cqr, UnaryOp direction) const;
	sserialize::ItemIndex betweenOp(const sserialize::CellQueryResult & cqr1, const sserialize::CellQueryResult & cqr2) const;
private:
	sserialize::Static::spatial::GeoHierarchy m_gh;
};

}//end namespace

#endif