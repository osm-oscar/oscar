#ifndef OSCAR_CREATE_CELL_CREATOR_H
#define OSCAR_CREATE_CELL_CREATOR_H
#include <sserialize/algorithm/hashspecializations.h>
#include <sserialize/containers/MMVector.h>
#include <sserialize/spatial/GeoHierarchy.h>
#include <sserialize/containers/OADHashTable.h>
#include <osmtools/OsmTriangulationRegionStore.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace sserialize {
namespace spatial {
	class GeoHierarchy;
}}

//BUG:regions without items should be removed from the hierarchy

namespace oscar_create {

class CellCreator {
public:
	
	//Needs to map [regionid] -> [cellid, [itemsPtr, itemCount]]
	
	class FlatCellMap {
	private:
		friend class CellCreator;
	private:
		struct CellItemEntry {
			uint32_t cellId;
			uint32_t itemId;
			CellItemEntry() : CellItemEntry(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()) {}
			CellItemEntry(uint32_t cellId, uint32_t itemId) : cellId(cellId), itemId(itemId) {}
		};
		
		typedef sserialize::MMVector<CellItemEntry> CellItemEntryStorage;
	private:
		uint32_t m_cellCount;
		CellItemEntryStorage m_cellItemEntries;
		std::mutex m_cellItemLock;
		
		std::vector<sserialize::spatial::GeoRect> m_cellBoundaries;
		
	public:
		FlatCellMap(uint32_t cellCount);
		~FlatCellMap();
		FlatCellMap & operator=(FlatCellMap && other);
		///thread safe insert
		void insert(uint32_t cellId, uint32_t itemId, const sserialize::spatial::GeoRect & gr);
	};
	
	typedef sserialize::spatial::detail::geohierarchy::CellList FlatCellList;

	//a cell is identified by the list GeoRegion creating it by intersection with all items in the kvstore
	typedef std::vector<uint32_t> CellId;
	typedef std::shared_ptr<CellId> CellIdPtr;
	typedef std::unordered_set<CellIdPtr, sserialize::DerefHasher<CellIdPtr, CellId>, sserialize::DerefEq<CellIdPtr> > CellSetType;
	template<typename TValue> using CellValueMap = std::unordered_map<CellIdPtr, TValue, sserialize::DerefHasher<CellIdPtr, CellId>, sserialize::DerefEq<CellIdPtr> >;

// 	typedef std::unordered_map<CellIdPtr, std::vector<uint32_t>, sserialize::DerefHasher<CellIdPtr, CellId>, sserialize::DerefEq<CellIdPtr> > CellMapType;

	typedef FlatCellMap CellMapType;
	typedef FlatCellList CellListType;
	

public:
	CellCreator();
	virtual ~CellCreator();

	///@cellMap gets cleared during processing, afterwards multiple cells may point to the same region-lists
	///@trs gets partially cleared, but triangulation and cellids are the same, but the regionList is empty
	void createCellList(FlatCellMap & cellMap, osmtools::OsmTriangulationRegionStore & trs, oscar_create::CellCreator::CellListType& cellList, std::vector<uint32_t> & newToOldCellId);
	///@cellList gets cleared during processing, the id of a region in the GeoHierarchy is the position of the region in the polystore.regions() container
	///you have to set the appropriate region.type and region.boundary info afterwards
	void createGeoHierarchy(FlatCellList & cellList, uint32_t geoRegionCount, sserialize::spatial::GeoHierarchy & gh);
};


}//end namespace
#endif
