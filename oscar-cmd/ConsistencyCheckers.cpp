#include "ConsistencyCheckers.h"
#include <sserialize/spatial/GeoRegion.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/vendor/utf8/checked.h>
#include <sserialize/utility/printers.h>

namespace oscarcmd {

bool ConsistencyChecker::checkIndex() {
	const sserialize::Static::ItemIndexStore & indexStore = cmp.indexStore();
	sserialize::ProgressInfo pinfo;
	uint32_t idxStoreSize = indexStore.size();
	bool allOk = true;
	pinfo.begin(indexStore.size(), "Checking index store");
	#pragma omp parallel for schedule(dynamic, 1)
	for(uint32_t i = 0;  i < idxStoreSize; ++i) {
		sserialize::ItemIndex idx(indexStore.at(i));
		if (idx.size() != indexStore.idxSize(i)) {
			allOk = false;
		}
		pinfo(i);
	}
	pinfo.end();
	return allOk;
}

bool ConsistencyChecker::checkGh() {
	const liboscar::Static::OsmKeyValueObjectStore& store = cmp.store();
	const sserialize::Static::ItemIndexStore & indexStore = cmp.indexStore();
	const sserialize::Static::spatial::GeoHierarchy & gh = store.geoHierarchy();
	uint32_t ghSize = gh.regionSize();
	sserialize::ProgressInfo pinfo;
	bool allOk = true;
	bool ok = true;
	pinfo.begin(gh.regionSize(), "ConsistencyChecker::GH: id-order");
	#pragma omp parallel for schedule(dynamic, 1)
	for(uint32_t i = 0; i < ghSize; ++i) {
		sserialize::Static::spatial::GeoHierarchy::Region r = gh.region(i);
		for (uint32_t j = 0, sj = r.childrenSize(); j < sj; ++j) {
			if (r.ghId() <= gh.region(j).ghId()) {
				ok = false;
			}
		}
		#pragma omp critical
		pinfo(i);
	}
	pinfo.end();
	allOk = allOk && ok;
	if (!ok) {
		std::cout << "Region-DAG has unsorted ids" << std::endl;
	}
	
	pinfo.begin(gh.regionSize(), "ConsistencyChecker::GH: cell region containment");
	ok = true;
	#pragma omp parallel for schedule(dynamic, 1)
	for(uint32_t i = 0; i < ghSize; ++i) {
		sserialize::Static::spatial::GeoHierarchy::Region r = gh.region(i);
		sserialize::ItemIndex cellIdx(indexStore.at(r.cellIndexPtr()));
		sserialize::spatial::GeoRect rb(r.boundary());
		for(uint32_t x : cellIdx) {
			if (!rb.contains(gh.cellBoundary(x))) {
				ok = false;
			}
		}
		#pragma omp critical
		pinfo(i);
	}
	pinfo.end();
	allOk = allOk && ok;
	if (!ok) {
		std::cout << "At least one region has cells with non-contained boundary" << std::endl;
	}
	
	if (gh.hasRegionItems()) {
		pinfo.begin(gh.regionSize(), "ConsistencyChecker::GH: indexes");
		#pragma omp parallel for schedule(dynamic, 1)
		for(uint32_t i = 0; i < ghSize; ++i) {
			sserialize::Static::spatial::GeoHierarchy::Region r = gh.region(i);
			sserialize::ItemIndex cellIdx = indexStore.at( r.cellIndexPtr() );
			std::vector<sserialize::ItemIndex> cellsItemIdcs;
			for(uint32_t j = 0, js = cellIdx.size(); j < js; ++j) {
				sserialize::Static::spatial::GeoHierarchy::Cell c = gh.cell(cellIdx.at(j));
				cellsItemIdcs.push_back( indexStore.at( c.itemPtr() ) );
			}
			sserialize::ItemIndex cellsItems = sserialize::ItemIndex::unite(cellsItemIdcs);
			sserialize::ItemIndex regionsItems = indexStore.at( r.itemsPtr() );
			if (cellsItems != regionsItems) {
				#pragma omp critical
				{
					std::cout << "Error in region " << i << ": merged items from cells don't match regions items." << std::endl;
					if (debug) {
						sserialize::ItemIndex symDif = cellsItems ^ regionsItems;
						std::cout << "symdiff index: " << symDif << std::endl;
					}
				}
				allOk = false;
			}
			#pragma omp critical
			pinfo(i);
		}
		pinfo.end();
	}
	else {
		std::cout << "Cannot check index since GeoHierarchy does not have region item info" << std::endl;
	}
	
	pinfo.begin(gh.regionSize(), "ConsistencyChecker::GH: store/gh region boundaries");
	#pragma omp parallel for schedule(dynamic, 1)
	for(uint32_t i = 0; i < ghSize; ++i) {
		sserialize::Static::spatial::GeoHierarchy::Region r = gh.region(i);
		uint32_t storeId = r.storeId();
		if (store.geoShape(storeId).boundary() !=  r.boundary()) {
			#pragma omp critical
			{
				std::cout << "Boundary of region " << i << " differs" << std::endl;
			}
			allOk = false;
		}
	}
	pinfo.end();
	
	uint32_t regionsWithInvalidItems = 0;
	uint32_t invalidItemsInRegions = 0;
	uint32_t itemsWithUnkownConsistency = 0;
	std::vector<std::pair<uint32_t, uint32_t>> brokenRegionItemInclusion;
	pinfo.begin(gh.regionSize(), "ConsistencyChecker::GH: inclusion");
	#pragma omp parallel for schedule(dynamic, 1)
	for(uint32_t i = 0; i < ghSize; ++i) {
		sserialize::Static::spatial::GeoHierarchy::Region r = gh.region(i);
	
		sserialize::Static::spatial::GeoShape rgs = store.geoShape(r.storeId());
		auto rgsr = rgs.get<sserialize::spatial::GS_REGION>();
		if (!rgsr) {
			std::cout << "Could not cast region " << i << " with storeId=" << r.storeId() << ", ghId=" << r.ghId() << " to a GeoRegion" << std::endl;
			allOk = false;
		}
		
		uint32_t invalidItemCount = 0;
		sserialize::ItemIndex regionsItems;
		if (r.hasItemsInfo()) {
			regionsItems = indexStore.at( r.itemsPtr() );
		}
		else {
			std::vector<sserialize::ItemIndex> cellItems;
			sserialize::ItemIndex cellItemIdx = indexStore.at( r.cellIndexPtr() );
			for(uint32_t cellIdxId : cellItemIdx) {
				cellItems.emplace_back( indexStore.at( cellIdxId ) );
			}
			regionsItems = sserialize::ItemIndex::unite(cellItems);
		}
		for(uint32_t itemId : regionsItems) {
			sserialize::Static::spatial::GeoShape igs = store.geoShape(itemId);
			bool ok = true;
			if (igs.type() == sserialize::spatial::GS_POINT) {
				ok = rgsr->contains( *igs.get<sserialize::spatial::GS_POINT>() );
			}
			else {
				try {
					ok = rgsr->intersects( *igs.get<sserialize::spatial::GS_REGION>() );
				}
				catch (const sserialize::UnimplementedFunctionException & e) {
					#pragma omp atomic
					itemsWithUnkownConsistency += 1;
				}
			}
			if (!ok) {
				++invalidItemCount;
				brokenRegionItemInclusion.emplace_back(i, itemId);
			}
		}
		if (invalidItemCount) {
			#pragma omp atomic
			invalidItemsInRegions += invalidItemCount;
			#pragma omp atomic
			++regionsWithInvalidItems;
		}
		#pragma omp critical
		pinfo(i);
	}
	pinfo.end();
	
	std::cout << "Found " << regionsWithInvalidItems << " regions having non-intersecting items\n";
	std::cout << "Found " << invalidItemsInRegions << " items having non-intersecting region\n";
	std::cout << "Found " << brokenRegionItemInclusion.size() << " broken region-item inclusion relations\n";
	if (debug) {
		std::cout << "Broken relations follow: " << std::endl;
		sserialize::operator<<(std::cout, brokenRegionItemInclusion) << std::endl;
	}
	return allOk;
}

bool ConsistencyChecker::checkTriangulation() {
	const liboscar::Static::OsmKeyValueObjectStore& store = cmp.store();
	if (!store.regionArrangement().tds().selfCheck()) {
		return false;
	}
	auto ra = store.regionArrangement();
	auto gr = ra.grid();
	auto tds = gr.tds();
	for(uint32_t i(0), s(tds.faceCount()); i < s; ++i) {
		auto f = tds.face(i);
		auto cn = f.centroid();
		if (gr.faceId(cn) != f.id()) {
			std::cout << "Triangulation returns wrong face" << std::endl;
			return false;
		}
		if (gr.faceHint(cn) < s) { //but a face is there, otheriwse the call above would have returned already
			std::cout << "Gridlocator returns invalid face hint" << std::endl;
			return false;
		}
	}
	return true;
}

bool ConsistencyChecker::checkStore() {
	const liboscar::Static::OsmKeyValueObjectStore & store = cmp.store();
	sserialize::ProgressInfo pinfo;
	sserialize::Static::StringTable::SizeType keyCount = store.keyStringTable().size();
	sserialize::Static::StringTable::SizeType valueCount = store.valueStringTable().size();
	uint32_t storeSize = store.size();
	uint32_t brokenItems = 0;
	pinfo.begin(store.size(), "ConsistencyChecker::Store: item creation");
	#pragma omp parallel for schedule(dynamic, 1)
	for(uint32_t i = 0;  i < storeSize; ++i) {
		liboscar::Static::OsmKeyValueObjectStore::Item item(store.at(i));
		for(uint32_t j(0), js(item.size()); j < js; ++j) {
			uint32_t keyId = item.keyId(j);
			uint32_t valueId = item.valueId(j);
			if (keyId >= keyCount || valueId >= valueCount) {
				#pragma omp atomic
				++brokenItems;
				break;
			}
			std::string key = item.key(j);
			std::string value = item.value(j);
			if (!utf8::is_valid(key.cbegin(), key.cend()) || !utf8::is_valid(value.cbegin(), value.cend())) {
				#pragma omp atomic
				++brokenItems;
				break;
			}
		}
		#pragma omp critical
		pinfo(i);
	}
	pinfo.end();
	if (brokenItems) {
		std::cout << "Found " << brokenItems << " broken items in store" << std::endl;
	}
	return brokenItems == 0;
}


bool ConsistencyChecker::checkCTC() {
	using CellTextCompleter = sserialize::Static::CellTextCompleter;
	if (!cmp.textSearch().hasSearch(liboscar::TextSearch::OOMGEOCELL)) {
		std::cout << "ConsistencyChecker: No geocell text search available" << std::endl;
		return true;
	}
	CellTextCompleter ctc = cmp.textSearch().get<liboscar::TextSearch::OOMGEOCELL>();
	auto indexStore = cmp.indexStore();
	auto gh = cmp.store().geoHierarchy();
	
	
	auto triePtr = ctc.trie().as<CellTextCompleter::FlatTrieType>();
	if (!triePtr) {
		std::cout << "No geocell completer with flat trie" << std::endl;
		return true;
	}
	
	auto trie = triePtr->trie();
	
	std::array<int, 4> checkTypes{{
		sserialize::StringCompleter::QT_EXACT,
		sserialize::StringCompleter::QT_PREFIX,
		sserialize::StringCompleter::QT_SUFFIX,
		sserialize::StringCompleter::QT_SUBSTRING
	}};
	
	uint32_t cellCount = gh.cellSize();
	
	bool ok = true;
	
	#pragma omp parallel for schedule(dynamic, 1)
	for(uint32_t i(0), s(trie.size()); i < s; ++i) {
		CellTextCompleter::Payload payload = trie.at(i);
		int types = payload.types();
		for(int ct : checkTypes) {
			if ((types & ct) == 0) {
				continue;
			}
			CellTextCompleter::Payload::Type td = payload.type(ct);
			sserialize::ItemIndex fmCells = indexStore.at(td.fmPtr());
			sserialize::ItemIndex pmCells = indexStore.at(td.pPtr());
			
			if (fmCells.size() && fmCells.back() >= cellCount) {
				ok = false;
				if (debug) {
					#pragma omp critical
					std::cout << "Broken ctc fm entry at " << i << " = " << trie.strAt(i) << std::endl;
				}
			}
			if (pmCells.size() && pmCells.back() >= cellCount) {
				ok = false;
				if (debug) {
					#pragma omp critical
					std::cout << "Broken ctc pm entry at " << i << " = " << trie.strAt(i) << std::endl;
				}
			}
			
			auto pmItemsPtrIt = td.pItemsPtrBegin();
			for(uint32_t pmCellId : pmCells) {
				sserialize::ItemIndex items = indexStore.at(*pmItemsPtrIt);
				sserialize::ItemIndex cellItems = indexStore.at( gh.cellItemsPtr(pmCellId) );
				sserialize::ItemIndex diff = items - cellItems;
				if (diff.size()) {
					ok = false;
					if (debug) {
						#pragma omp critical
						std::cout << "Broken ctc pm items at " << i << " = " << trie.strAt(i) << std::endl;
						std::cout << "Elements in index that are not in the cell: " << diff << std::endl;
					}
				}
				++pmItemsPtrIt;
			}
		}
	}
	return ok;
}

}//end namespace
