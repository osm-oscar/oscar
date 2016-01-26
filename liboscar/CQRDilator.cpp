#include "CQRDilator.h"
#include <unordered_set>

namespace liboscar {

CQRDilator::CQRDilator(const CellInfo & d, const sserialize::Static::spatial::TracGraph & tg) :
m_priv(new detail::CQRDilator(d, tg))
{}

CQRDilator::~CQRDilator() {}

//dilating TreedCQR doesn't make any sense, since we need the real result with it
sserialize::ItemIndex CQRDilator::dilate(const sserialize::CellQueryResult& src, double diameter) const {
	struct MyIterator {
		sserialize::CellQueryResult::const_iterator m_it;
		inline bool operator!=(const MyIterator & other) { return m_it != other.m_it; }
		inline bool operator==(const MyIterator & other) { return m_it == other.m_it; }
		inline MyIterator & operator++() { ++m_it; return *this; }
		inline uint32_t operator*() { return m_it.cellId(); }
		MyIterator(const sserialize::CellQueryResult::const_iterator & it) : m_it(it) {}
	};
	return m_priv->dilate<MyIterator>(MyIterator(src.begin()), MyIterator(src.end()), diameter);
}

sserialize::ItemIndex CQRDilator::dilate(const sserialize::ItemIndex& src, double diameter) const {
	return m_priv->dilate<sserialize::ItemIndex::const_iterator>(src.cbegin(), src.cend(), diameter);
}

namespace detail {

CQRDilator::CQRDilator(const CellInfo & d, const sserialize::Static::spatial::TracGraph & tg) :
m_ci(d),
m_tg(tg)
{}

CQRDilator::~CQRDilator() {}

double CQRDilator::distance(const sserialize::Static::spatial::GeoPoint& gp1, const sserialize::Static::spatial::GeoPoint& gp2) const {
	return std::abs<double>( sserialize::spatial::distanceTo(gp1.lat(), gp1.lon(), gp2.lat(), gp2.lon()) );
}

double CQRDilator::distance(const sserialize::Static::spatial::GeoPoint& gp, uint32_t cellId) const {
	return distance(gp, m_ci.at(cellId));
}

double CQRDilator::distance(uint32_t cellId1, uint32_t cellId2) const {
	return distance(m_ci.at(cellId1), m_ci.at(cellId2));
}


}}//end namespace liboscar::detail