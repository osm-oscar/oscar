#include "CQRFromComplexSpatialQuery.h"

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

sserialize::ItemIndex CQRFromComplexSpatialQuery::betweenOp(const sserialize::CellQueryResult& cqr1, const sserialize::CellQueryResult& cqr2) const {

	return sserialize::ItemIndex();
}

sserialize::ItemIndex CQRFromComplexSpatialQuery::compassOp(const sserialize::CellQueryResult& cqr, liboscar::CQRFromComplexSpatialQuery::UnaryOp direction) const {

	return sserialize::ItemIndex();
}


}}//end namespace liboscar::detail