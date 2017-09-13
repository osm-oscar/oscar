#include "CellCreator.h"
#include <sserialize/Static/GeoWay.h>
#include <sserialize/utility/printers.h>
#include <sserialize/mt/ThreadPool.h>
#include "OsmKeyValueObjectStore.h"

inline std::size_t hashCellId(const std::vector<uint32_t> & v) {
	std::size_t seed = 0;
	for(std::vector<uint32_t>::const_iterator it(v.begin()), end(v.end()); it != end; ++it) {
		::hash_combine(seed, *it);
	}
	return seed;
}


namespace oscar_create {

CellCreator::FlatCellMap::FlatCellMap(uint32_t cellCount) :
m_cellCount(cellCount),
m_cellItemEntries(sserialize::MM_FILEBASED),
m_cellBoundaries(cellCount)
{}

CellCreator::FlatCellMap::~FlatCellMap() {}

CellCreator::FlatCellMap& CellCreator::FlatCellMap::operator=(CellCreator::FlatCellMap&& other) {
	if (this == &other) {
		return *this;
	}
	std::unique_lock<std::mutex>(m_cellItemLock);
	std::unique_lock<std::mutex>(other.m_cellItemLock);

	using std::swap;
	m_cellCount = std::move(other.m_cellCount);
	m_cellItemEntries = std::move(other.m_cellItemEntries);
	m_cellBoundaries = std::move(other.m_cellBoundaries);
	
	return *this;
}

void CellCreator::FlatCellMap::insert(uint32_t cellId, uint32_t itemId, const sserialize::spatial::GeoRect & gr) {
	SSERIALIZE_CHEAP_ASSERT_SMALLER(cellId, m_cellCount);
	std::unique_lock<std::mutex> cILock(m_cellItemLock);
	m_cellItemEntries.emplace_back(cellId, itemId);
	sserialize::spatial::GeoRect & cb = m_cellBoundaries.at(cellId);
	//snap boundary here. All points inserted into the cellmap are snapped later anyway
	//It's easier to do this here instead of doing it in every call-site
	cb.enlarge(gr.snapped());
}

CellCreator::CellCreator() {}
CellCreator::~CellCreator() {}

void CellCreator::createCellList(FlatCellMap & cellMap, osmtools::OsmTriangulationRegionStore & trs, oscar_create::CellCreator::CellListType& cellList, std::vector<uint32_t> & newToOldCellId) {
	sserialize::MMVector<uint32_t> m_cellIdData(std::move(trs.regionLists()));
	sserialize::MMVector<uint32_t> m_cellItems(sserialize::MM_FAST_FILEBASED);
	sserialize::MMVector<CellListType::Cell> m_d(sserialize::MM_PROGRAM_MEMORY);
	
	{
		std::sort(cellMap.m_cellItemEntries.begin(), cellMap.m_cellItemEntries.end(),
			[](const FlatCellMap::CellItemEntry & a, const FlatCellMap::CellItemEntry & b) {
				return a.cellId == b.cellId ? a.itemId < b.itemId : a.cellId < b.cellId;
			}
		);

		//remove empty cells and remap cellids, list will still be ordered as new cellids are created in ascending order
		std::vector<uint32_t> oldToNewCellId(trs.cellCount(), 0xFFFFFFFF);
		newToOldCellId.clear();
		for(FlatCellMap::CellItemEntry & x : cellMap.m_cellItemEntries) {
			if (oldToNewCellId.at(x.cellId) == 0xFFFFFFFF) {
				oldToNewCellId[x.cellId] = (uint32_t) newToOldCellId.size();
				newToOldCellId.push_back(x.cellId);
			}
			x.cellId = oldToNewCellId.at(x.cellId);
		}
		std::cout << "Pruned " << oldToNewCellId.size()-newToOldCellId.size() << " empty cells" << std::endl;

		m_d.resize(newToOldCellId.size());
		m_cellItems.reserve(cellMap.m_cellItemEntries.size());

		//copy the cell-list information, itemsets are not available yet, store cells with incorrect boundary
		std::unordered_set<uint32_t> cellsWithMissingBoundary; //stores old cellids
		for(uint32_t oldCellId : newToOldCellId) {
			//the internal points of this list are wrong, but offset and size are correct
			const osmtools::OsmTriangulationRegionStore::RegionList & rl = trs.regions(oldCellId);
			sserialize::spatial::GeoRect & cr = cellMap.m_cellBoundaries.at(oldCellId);
			CellListType::Cell & c = m_d.at(oldToNewCellId.at(oldCellId)); 
			c = CellListType::Cell(m_cellIdData.begin()+rl.offset(), rl.size(), 0, 0, cr);
			SSERIALIZE_EXPENSIVE_ASSERT(sserialize::is_strong_monotone_ascending(c.parentsBegin(), c.parentsEnd()));
			if (!cr.valid()) {
				cellsWithMissingBoundary.insert(oldCellId);
			}
			else {
				SSERIALIZE_NORMAL_ASSERT(cr.isSnapped());
			}
		}
		
		//fix cell boundaries, cellIds here are the old ids
		for(osmtools::OsmTriangulationRegionStore::Finite_faces_iterator fh(trs.finite_faces_begin()), end(trs.finite_faces_end()); fh != end; ++fh) {
			uint32_t fhId = trs.cellId(fh);
			if (cellsWithMissingBoundary.count(fhId)) {
				sserialize::MinMax<double> latInfo, lonInfo;
				for(int i(0); i < 3; ++i) {
					auto p(fh->vertex(0)->point());
					latInfo.update(CGAL::to_double(p.x()));
					lonInfo.update(CGAL::to_double(p.y()));
				}
				auto & cell = m_d.at(oldToNewCellId.at(fhId));
				cell.boundary().enlarge(sserialize::spatial::GeoRect(latInfo.min(), latInfo.max(), lonInfo.min(), lonInfo.max()));
				SSERIALIZE_NORMAL_ASSERT(cell.boundary().isSnapped());
			}
		}
	}
	
	{//populate the cell items store
		uint32_t cellId = 0;
		uint32_t cellItemSize = 0;
		uint32_t * cellItemsBegin = m_cellItems.end();
		
		auto finFunc = [&m_d, &cellId, &cellItemSize, &cellItemsBegin]() {
			CellListType::Cell & c = m_d.at(cellId);
			c = CellListType::Cell(c.parentsBegin(), c.parentsSize(), cellItemsBegin, cellItemSize, c.boundary());
			cellItemsBegin += cellItemSize;
			cellItemSize = 0;
			++cellId;
		};
		
		
		for(const FlatCellMap::CellItemEntry & x  : cellMap.m_cellItemEntries) {
			SSERIALIZE_CHEAP_ASSERT_SMALLER(x.cellId, m_d.size());
			if (x.cellId > cellId) { //new cellId, finalize previous
				SSERIALIZE_CHEAP_ASSERT(cellItemSize);
				finFunc();
				SSERIALIZE_CHEAP_ASSERT_EQUAL(x.cellId, cellId);
			}
			m_cellItems.push_back(x.itemId);
			++cellItemSize;
		}
		finFunc();
	}
	{
		std::vector<uint32_t> sortedCellId2newCellId;
		sortedCellId2newCellId.reserve(newToOldCellId.size());
		for(uint32_t i(0), s((uint32_t) newToOldCellId.size()); i < s; ++i) {
			sortedCellId2newCellId.push_back(i);
		}
		
		//TODO:look into speeding this up by doing it only once with a reorder pass afterwards
		//since permutation can be described by at most n transpositions
		std::sort(sortedCellId2newCellId.begin(), sortedCellId2newCellId.end(), [&m_d](uint32_t a, uint32_t b) {
			return m_d.at(a).parentsSize() < m_d.at(b).parentsSize();
		});
		
		std::sort(m_d.begin(), m_d.end(), [](const CellListType::Cell & a, const CellListType::Cell & b) {
			return a.parentsSize() < b.parentsSize();
		});
		//update the new2OldCellId map
		std::vector<uint32_t> tmp(sortedCellId2newCellId.size());
		for(uint32_t i(0), s((uint32_t) sortedCellId2newCellId.size()); i < s; ++i) {
			tmp.at(i) = newToOldCellId.at( sortedCellId2newCellId.at(i) );
		}
		SSERIALIZE_CHEAP_ASSERT_EQUAL(newToOldCellId.size(), m_d.size());
		newToOldCellId = std::move(tmp);
		SSERIALIZE_CHEAP_ASSERT_EQUAL(newToOldCellId.size(), m_d.size());
	}
	
	cellList = CellListType(std::move(m_cellIdData), std::move(m_cellItems), std::move(m_d));
	
	cellMap = FlatCellMap(0);
}

struct ChildrenOfGeoRegion {
	union {
		struct {
			uint32_t * m_begin;
			uint32_t * m_end;
		} p;
		struct {
			uint64_t beginOffset;
			uint64_t size;
		} o;
	} u;
	ChildrenOfGeoRegion(uint32_t * begin, uint32_t * end) { u.p.m_begin = begin; u.p.m_end = end;}
	uint32_t * begin() { return u.p.m_begin; }
	uint32_t * end() { return u.p.m_end; }
	const uint32_t * begin() const { return u.p.m_begin; }
	const uint32_t * end() const { return u.p.m_end; }
	const uint32_t * cbegin() const { return u.p.m_begin; }
	const uint32_t * cend() const { return u.p.m_end; }
	uint32_t size() const { return (uint32_t) (end()-begin()); }
};

//BUG?: on plankton with planet: geoRegionCellSplit had a non-initialized entry which means that the cell is empty (this should have been filtered before?)

void CellCreator::createGeoHierarchy(FlatCellList& cellList, uint32_t geoRegionCount, sserialize::spatial::GeoHierarchy& gh) {
	typedef sserialize::CFLArray< sserialize::MMVector<uint32_t> > CellsOfGeoRegion;
	typedef sserialize::MMVector< CellsOfGeoRegion > GeoRegionCellSplitListType;
	sserialize::ProgressInfo info;
	std::atomic<uint32_t> processedItems(0);
	std::cout << "CellCreator: creating GeoHierarchy for " << geoRegionCount << " regions out of " << cellList.size() << " cells" << std::endl;
	
	sserialize::MMVector<uint32_t> geoRegionGraphData(sserialize::MM_FILEBASED);
	sserialize::MMVector<ChildrenOfGeoRegion> geoRegionGraph(sserialize::MM_FAST_FILEBASED, geoRegionCount, ChildrenOfGeoRegion(0, 0));

	sserialize::MMVector<uint32_t> geoRegionCellSplitData(sserialize::MM_FAST_FILEBASED);
	GeoRegionCellSplitListType geoRegionCellSplit(sserialize::MM_FAST_FILEBASED);

	sserialize::MMVector< std::pair<uint32_t, uint32_t> > parents(sserialize::MM_FAST_FILEBASED); // (regionId, parentId)
	
	{//calculate geoRegionCellSplit
		//geoRegionCellSplit contains the list of cells for the regions, the cell lists are sorted
		sserialize::MMVector< std::pair<uint32_t, uint32_t> > tmp(sserialize::MM_FILEBASED); //(regionId, cellId)
		for(uint32_t i = 0, s = cellList.size(); i < s; ++i) {
			for(auto pit(cellList.at(i).parentsBegin()), pend(cellList.at(i).parentsEnd()); pit != pend; ++pit) {
				tmp.emplace_back(*pit, i);
			}
		}
		std::sort(tmp.begin(), tmp.end());
		geoRegionCellSplitData.reserve(tmp.size());
		geoRegionCellSplit.resize(geoRegionCount, CellsOfGeoRegion(&geoRegionCellSplitData, 0, 0));
		uint32_t prevRegionId = 0;
		sserialize::UByteArrayAdapter::OffsetType prevRegionOffset = 0;
		for(auto & x : tmp) {
			if (x.first != prevRegionId) {
				CellsOfGeoRegion & cr = geoRegionCellSplit.at(prevRegionId);
				cr = CellsOfGeoRegion(&geoRegionCellSplitData, prevRegionOffset, geoRegionCellSplitData.size()-prevRegionOffset);
				prevRegionId = x.first;
				prevRegionOffset = geoRegionCellSplitData.size();
			}
			geoRegionCellSplitData.push_back(x.second);
		}
		geoRegionCellSplit.back() = CellsOfGeoRegion(&geoRegionCellSplitData, prevRegionOffset, geoRegionCellSplitData.size()-prevRegionOffset);
	}
	SSERIALIZE_CHEAP_ASSERT_EQUAL(geoRegionCellSplit.size(), geoRegionCount);
	
	{//calculate geoRegionGraph
		auto geoRegionCellSizeComparer = [&geoRegionCellSplit](uint32_t x, uint32_t y) {
										if (geoRegionCellSplit.at(x).size() == geoRegionCellSplit.at(y).size())
											return (x < y);
										else
											return geoRegionCellSplit.at(x).size() < geoRegionCellSplit.at(y).size();
									};
		
		processedItems = 0;
		std::mutex lck;
		info.begin(geoRegionCount, "Creating GeoRegionGraph");
		{
			std::atomic<uint32_t> regionIt(0);
			sserialize::ThreadPool::execute([&]() {
				while (true) {
					uint32_t i = regionIt.fetch_add(1);
					
					if (i >= geoRegionCount) {
						return;
					}
					
					const auto & myCellList = geoRegionCellSplit.at(i);
					SSERIALIZE_EXPENSIVE_ASSERT(std::is_sorted(myCellList.begin(), myCellList.end()));
					std::set<uint32_t, decltype(geoRegionCellSizeComparer)> tmpChildRegions(geoRegionCellSizeComparer);
					std::unordered_set<uint32_t> checkedCandidateRegions;
					checkedCandidateRegions.insert(i);
					for(auto cit(myCellList.begin()), cend(myCellList.end()); cit != cend; ++cit) {
						for(auto pit(cellList.at(*cit).parentsBegin()), pend(cellList.at(*cit).parentsEnd()); pit != pend; ++pit) {
							if (!checkedCandidateRegions.count(*pit)) {
								if (sserialize::subset_of(geoRegionCellSplit.at(*pit).cbegin(), geoRegionCellSplit.at(*pit).cend(), myCellList.cbegin(), myCellList.cend())) {
									tmpChildRegions.insert(*pit);
								}
								else {
									checkedCandidateRegions.insert(*pit);
								}
							}
						}
					}
					//now remove all children that are not direct descendents
					//the children are sorted according to the size of their cell-list (in ascending order!)
					//start with the largest one and remove all subsets
					std::vector<uint32_t> myChildren;
					while (tmpChildRegions.size()) {
						uint32_t directChild = *tmpChildRegions.rbegin();
						myChildren.push_back(directChild);
						tmpChildRegions.erase(directChild);
						const auto & directChildCellList = geoRegionCellSplit.at(directChild); 
						for(std::set<uint32_t>::iterator it(tmpChildRegions.begin()), end(tmpChildRegions.end()); it != end;) {
							if (sserialize::subset_of(geoRegionCellSplit.at(*it).cbegin(), geoRegionCellSplit.at(*it).cend(), directChildCellList.cbegin(), directChildCellList.cend())) {
								it = tmpChildRegions.erase(it);
							}
							else {
								++it;
							}
						}
					}
					//sort the children since they are now sorted according to their cell count
					std::sort(myChildren.begin(), myChildren.end());
					ChildrenOfGeoRegion & me = geoRegionGraph.at(i);
					me.u.o.size = myChildren.size();
					{
						std::unique_lock<std::mutex> l(lck);
						me.u.o.beginOffset = geoRegionGraphData.size();
						geoRegionGraphData.push_back(myChildren.begin(), myChildren.end());
					}
					{
						++processedItems;
						info(processedItems);
					}
				}
			});
		}
		for(ChildrenOfGeoRegion & x : geoRegionGraph) {
			auto off = x.u.o.beginOffset;
			auto size = x.u.o.size;
			x.u.p.m_begin = geoRegionGraphData.begin()+off;
			x.u.p.m_end = x.u.p.m_begin + size;
		}
		info.end();
	}//end calculate geoRegionGraph
	SSERIALIZE_CHEAP_ASSERT_EQUAL(geoRegionGraph.size(), geoRegionCount);
	
	{//calculate the parents
		for(uint32_t i = 0, s = (uint32_t) geoRegionGraph.size(); i < s; ++i) {
			if (geoRegionGraph.at(i).size()) {
				for(auto childId : geoRegionGraph.at(i)) {
					parents.emplace_back(childId, i);
				}
			}
		}
		std::sort(parents.begin(), parents.end());
	}
	
	std::cout << "Found " << cellList.size() << " different cells" << std::endl;
	if (cellList.size()) {
		std::cout << "Maximum cell-split: " << std::max_element(geoRegionCellSplit.begin(), geoRegionCellSplit.end(), [] (const CellsOfGeoRegion & x, const CellsOfGeoRegion & y) {return x.size() < y.size();})->size() << std::endl;
		std::cout << "Maximum children: " << std::max_element(geoRegionGraph.begin(), geoRegionGraph.end(), [] (const ChildrenOfGeoRegion & x, const ChildrenOfGeoRegion & y) {return x.size() < y.size();})->size() << std::endl;
		std::cout << "Constructing Hierarchy" << std::endl;
		gh.cells().swap(cellList);
		gh.regions().clear();
		gh.regions().regionDescriptions().reserve(geoRegionGraph.size());
		auto parentsIt = parents.begin();
		for(uint32_t i = 0, s = (uint32_t) geoRegionGraph.size(); i < s; ++i) {
			sserialize::OffsetType off = gh.regions().regionData().size();
			uint32_t parentsSize = 0;
			gh.regions().regionData().push_back(geoRegionGraph.at(i).cbegin(), geoRegionGraph.at(i).cend());//children
			while(parentsIt->first == i && parentsIt != parents.end()) {
				gh.regions().regionData().push_back(parentsIt->second);
				++parentsSize;
				++parentsIt;
			}
			gh.regions().regionData().push_back(geoRegionCellSplit.at(i).cbegin(), geoRegionCellSplit.at(i).cend());
		
			gh.regions().regionDescriptions().emplace_back(&(gh.regions().regionData()), off, geoRegionGraph.at(i).size(), parentsSize, geoRegionCellSplit.at(i).size(), 0);
			sserialize::spatial::GeoHierarchy::Region & r = gh.regions().regionDescriptions().back();
			
			r.storeId = i;
			r.ghId = 0xFFFFFFFF; //BUG: fix order
// 			r.type = polystore.regions().at(i)->type();
// 			r.boundary = polystore.regions().at(i)->boundary();

// 			if (polystore.values().at(i).osmId == 1107905) {
// 				std::cout << "Reached Neugereut: id=" << i << std::endl;
// 				std::cout << "Cells: " << r.cells() << std::endl;
// 			}
		}
#ifdef SSERIALIZE_EXPENSIVE_ASSERT_ENABLED
		for(sserialize::spatial::GeoHierarchy::Region & r : gh.regions()) {
			for(uint32_t cellId : r.cells()) {
				const sserialize::spatial::GeoHierarchy::Cell & c = gh.cell(cellId);
				SSERIALIZE_EXPENSIVE_ASSERT( std::find(c.parentsBegin(), c.parentsEnd(), r.storeId) != c.parentsEnd());
			}
		}
#endif
		{//now sort the regions according to their cell.size. This makes sure that region.ghId > region.children[].ghId
			typedef sserialize::spatial::GeoHierarchy::Region MyRegion;
			std::sort(gh.regions().begin(), gh.regions().end(), [](const MyRegion & r1, const MyRegion & r2 ) {
				return (r1.cellsSize() == r2.cellsSize() ? r1.cells() < r2.cells() : r1.cellsSize() < r2.cellsSize());
			});
			std::vector<uint32_t> storeIdToGhId(gh.regions().size(), 0);
			for(uint32_t i = 0, s(gh.regions().size()); i < s; ++i) {
				sserialize::spatial::GeoHierarchy::Region & r = gh.regions()[i];
				r.ghId = i;
				storeIdToGhId[r.storeId] = r.ghId;
			}
			//now remap the children in the regions:
			for(MyRegion & r : gh.regions()) {
				for(uint32_t & x : r.parents()) {
					x = storeIdToGhId.at(x);
				}
				for(uint32_t & x : r.children()) {
					x = storeIdToGhId.at(x);
				}
				std::sort(r.childrenBegin(), r.childrenEnd());
				std::sort(r.parentsBegin(), r.parentsEnd());
			}
			//we have to remap the cell data first since multiple cells may point to the same data since they share the same regions
			for(uint32_t & x : gh.cells().cellRegionLists()) {
				x = storeIdToGhId.at(x);
			}
			for(sserialize::spatial::GeoHierarchy::Cell & c : gh.cells()) {
				std::sort(c.parentsBegin(), c.parentsEnd());
			}
		}
	}
}

}//end namespace