#include "CQRFromComplexSpatialQuery.h"

namespace liboscar {

CQRFromComplexSpatialQuery::CQRFromComplexSpatialQuery(const sserialize::Static::spatial::GeoHierarchy& gh) :
m_gh(gh)
{}

CQRFromComplexSpatialQuery::~CQRFromComplexSpatialQuery() {}

sserialize::ItemIndex CQRFromComplexSpatialQuery::betweenOp(const sserialize::CellQueryResult& cqr1, const sserialize::CellQueryResult& cqr2) const {

	return sserialize::ItemIndex();
}

sserialize::ItemIndex CQRFromComplexSpatialQuery::compassOp(const sserialize::CellQueryResult& cqr, CQRFromComplexSpatialQuery::UnaryOp direction) const {

	return sserialize::ItemIndex();
}


}//end namespace liboscar