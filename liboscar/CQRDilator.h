#ifndef LIBOSCAR_CQR_DILATOR_H
#define LIBOSCAR_CQR_DILATOR_H
#include <sserialize/Static/TracGraph.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/GeoPoint.h>
#include <sserialize/containers/SimpleBitVector.h>
#include <sserialize/spatial/LatLonCalculations.h>
#include <sserialize/spatial/CellQueryResult.h>

namespace liboscar {
namespace detail {

//Dilate a CQR
class CQRDilator: public sserialize::RefCountObject {
public:
private:
	typedef sserialize::Static::Array<sserialize::Static::spatial::GeoPoint> CellInfo;
public:
	///@param d the weight-center of cells
	CQRDilator(const CellInfo & d, const sserialize::Static::spatial::TracGraph & tg);
	~CQRDilator();
	///@param amount in meters, @return list of cells that are part of the dilated area (input cells are NOT part of this list)
	template<typename TCELL_ID_ITERATOR>
	sserialize::ItemIndex dilate(TCELL_ID_ITERATOR begin, TCELL_ID_ITERATOR end, double diameter) const;
private:
	double distance(const sserialize::Static::spatial::GeoPoint & gp1, const sserialize::Static::spatial::GeoPoint & gp2) const;
	double distance(const sserialize::Static::spatial::GeoPoint & gp, uint32_t cellId) const;
	double distance(uint32_t cellId1, uint32_t cellId2) const;
private:
	CellInfo m_ci;
	sserialize::Static::spatial::TracGraph m_tg;
};

}//end namespace detail
/**
  * ------------------------------------------------------------
  * Array<GeoPoint>
  * ------------------------------------------------------------
  * Static::Array<Static::GeoPoint>
  * ------------------------------------------------------------
  *
  *
  *
  */
  
class CQRDilator {
public:
	typedef sserialize::Static::Array<sserialize::Static::spatial::GeoPoint> CellInfo;
public:
	///@param d the weight-center of cells
	CQRDilator(const CellInfo & d, const sserialize::Static::spatial::TracGraph & tg);
	~CQRDilator();
	///@param amount in meters, @return list of cells that are part of the dilated area (input cells are NOT part of this list)
	sserialize::ItemIndex dilate(const sserialize::CellQueryResult & src, double diameter) const;

	///@param amount in meters, @return list of cells that are part of the dilated area (input cells are NOT part of this list)
	sserialize::ItemIndex dilate(const sserialize::ItemIndex & src, double diameter) const;
private:
	sserialize::RCPtrWrapper<detail::CQRDilator> m_priv;
};

namespace detail {

//dilating TreedCQR doesn't make any sense, since we need the real result with it
//
template<typename TCELL_ID_ITERATOR>
sserialize::ItemIndex CQRDilator::dilate(TCELL_ID_ITERATOR begin, TCELL_ID_ITERATOR end, double diameter) const {
	typedef TCELL_ID_ITERATOR MyIterator;
	SSERIALIZE_EXPENSIVE_ASSERT(std::is_sorted(begin, end));
	if (!(begin != end)) {
		return sserialize::ItemIndex();
	}
	uint32_t lowestCellId = *begin;
	sserialize::SimpleBitVector baseCells;
	auto isNotBaseCell = [&baseCells, lowestCellId](uint32_t cid) {
		return (cid < lowestCellId || !baseCells.isSet(cid-lowestCellId));
	};
	sserialize::SimpleBitVector dilatedMarks;
	std::vector<uint32_t> dilated;
	for(MyIterator it(begin); it != end; ++it) {
		baseCells.set(*it-lowestCellId);
	}
	
	std::unordered_set<uint32_t> relaxed;
	std::vector<uint32_t> queue;
	for(MyIterator it(begin); it != end; ++it) {
		uint32_t cellId = *it;
		sserialize::Static::spatial::GeoPoint cellGp( m_ci.at(cellId) );
		auto node(m_tg.node(cellId));
		//put neighbors into workqueue
		for(uint32_t i(0), s(node.neighborCount()); i < s; ++i) {
			uint32_t nId = node.neighborId(i);
			if (isNotBaseCell(nId) && distance(cellGp, nId) < diameter) {
				queue.push_back(nId);
				relaxed.insert(nId);
			}
		}
		//now explore the neighborhood
		while(queue.size()) {
			uint32_t oCellId = queue.back();
			queue.pop_back();
			//iterator over all neighbors
			auto tmpNode(m_tg.node(oCellId));
			for(uint32_t i(0), s(tmpNode.neighborCount()); i < s; ++i) {
				uint32_t nId = tmpNode.neighborId(i);
				if (isNotBaseCell(nId) && !relaxed.count(nId) && distance(cellGp, nId) < diameter) {
					queue.push_back(nId);
					relaxed.insert(nId);
				}
			}
		}
		//push them to the ouput
		for(auto x : relaxed) {
			if (!dilatedMarks.isSet(x)) {
				dilated.push_back(x);
			}
			dilatedMarks.set(x);
		}
		relaxed.clear();
		queue.clear();
	}
	//depending on the size of dilated it should be faster to just get them from dilatedMarks since these are ordered
	uint32_t dilatedSize = (uint32_t) dilated.size()*sizeof(uint32_t);
	if (dilatedSize > 1024 && dilatedSize*(sserialize::fastLog2(dilatedSize)-1) > dilatedMarks.storageSizeInBytes()) {
		dilatedMarks.getSet(dilated.begin());
	}
	else {
		std::sort(dilated.begin(), dilated.end());
	}
	return sserialize::ItemIndex(std::move(dilated));
}

}}//end namespace liboscar::detail

#endif