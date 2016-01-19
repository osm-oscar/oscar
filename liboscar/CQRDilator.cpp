#include "CQRDilator.h"
#include <sserialize/spatial/LatLonCalculations.h>
#include <sserialize/containers/SimpleBitVector.h>
#include <unordered_set>

namespace liboscar {

CQRDilator::CQRDilator(const CellInfo & d, const sserialize::Static::spatial::TracGraph & tg) :
m_ci(d),
m_tg(tg)
{}

CQRDilator::~CQRDilator() {}

//dilating TreedCQR doesn't make any sense, since we need the real result with it
sserialize::ItemIndex CQRDilator::dilate(const sserialize::CellQueryResult& src, double diameter) const {
	typedef sserialize::CellQueryResult CQRType;
	
	if (!src.cellCount()) {
		return sserialize::ItemIndex();
	}
	uint32_t lowestCellId = src.cellId(0);
	uint32_t highestCellId = src.cellId(src.cellCount()-1);
	sserialize::SimpleBitVector baseCells(highestCellId-lowestCellId+1);
	auto isNotBaseCell = [&baseCells, lowestCellId, highestCellId](uint32_t cid) {
		return (cid < lowestCellId || cid > highestCellId || !baseCells.isSet(cid-lowestCellId));
	};
	sserialize::SimpleBitVector dilatedMarks;
	std::vector<uint32_t> dilated;
	for(sserialize::CellQueryResult::const_iterator it(src.begin()), end(src.end()); it != end; ++it) {
		baseCells.set(it.cellId()-lowestCellId);
	}
	
	std::unordered_set<uint32_t> relaxed;
	std::vector<uint32_t> queue;
	for(sserialize::CellQueryResult::const_iterator it(src.begin()), end(src.end()); it != end; ++it) {
		uint32_t cellId = it.cellId();
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

double CQRDilator::distance(const sserialize::Static::spatial::GeoPoint& gp1, const sserialize::Static::spatial::GeoPoint& gp2) const {
	return std::abs<double>( sserialize::spatial::distanceTo(gp1.lat(), gp1.lon(), gp2.lat(), gp2.lon()) );
}

double CQRDilator::distance(const sserialize::Static::spatial::GeoPoint& gp, uint32_t cellId) const {
	return distance(gp, m_ci.at(cellId));
}

double CQRDilator::distance(uint32_t cellId1, uint32_t cellId2) const {
	return distance(m_ci.at(cellId1), m_ci.at(cellId2));
}


}//end namespace