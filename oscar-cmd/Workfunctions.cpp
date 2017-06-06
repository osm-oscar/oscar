#include "Workfunctions.h"
#include <unordered_set>
#include "Config.h"
#include "ConsistencyCheckers.h"
#include "GeoHierarchyPrinter.h"
#include "LiveCompleter.h"
#include "CairoRenderer.h"
#include <sserialize/strings/unicode_case_functions.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/stats/statfuncs.h>
#include <sserialize/spatial/LatLonCalculations.h>

namespace oscarcmd {

std::string statsFileNameSuffix(uint32_t which) {
	switch (which) {
	case PS_ALL:
		return "all";
	case PS_IDX_STORE:
		return "idxstore";
	case PS_ITEM_COMPLETER:
		return "itemcomp";
	case PS_HIERARCHY_COMPLETER:
		return "hierachycomp";
	case PS_GEOCELL_COMPLETER:
		return "geocellcomp";
	case PS_COMPLETER:
		return "comp";
	case PS_DB:
		return "db";
	case PS_GEO:
		return "geo";
	case PS_TAG:
		return "tag";
	case PS_GH:
		return "gh";
	case PS_NONE:
		return "none";
	default:
		return "mix";
	}
}

void doPrintStats(std::ostream & out, liboscar::Static::OsmCompleter & completer, uint32_t which) {
	if (which & S_DB) {
		completer.store().printStats(out);
		out << '\n';
	}
	if (which & PS_IDX_STORE) {
		completer.indexStore().printStats(out);
		out << '\n';
	}
	if (which & PS_ITEM_COMPLETER) {
		for(uint32_t i(0), s(completer.textSearch().size(liboscar::TextSearch::ITEMS)); i < s; ++i) {
			completer.textSearch().get<liboscar::TextSearch::ITEMS>(i).printStats(out);
			out << '\n';
		}
	}
	if (which & PS_HIERARCHY_COMPLETER) {
		for(uint32_t i(0), s(completer.textSearch().size(liboscar::TextSearch::GEOHIERARCHY)); i < s; ++i) {
			completer.textSearch().get<liboscar::TextSearch::GEOHIERARCHY>(i).printStats(out);
			out << '\n';
		}
	}
	if (which & PS_GEOCELL_COMPLETER) {
		for(uint32_t i(0), s(completer.textSearch().size(liboscar::TextSearch::GEOCELL)); i < s; ++i) {
			completer.textSearch().get<liboscar::TextSearch::GEOCELL>(i).printStats(out);
			out << '\n';
		}
	}
	if (which & PS_GEO) {
		out << completer.geoCompleter()->describe();
		out << '\n';
	}
	if (which & PS_TAG) {
		completer.tagStore().printStats(out);
		out << '\n';
	}
	if (which & PS_GH) {
		completer.store().geoHierarchy().printStats(out, completer.indexStore());
		out << '\n';
	}
}

void Worker::listCompleters() {
	if (completer.textSearch().hasSearch(liboscar::TextSearch::ITEMS)) {
		std::cout << "Items completers:\n"; 
		for(uint32_t i(0), s(completer.textSearch().size(liboscar::TextSearch::ITEMS)); i < s; ++i) {
			std::cout << "\t" << i << "=" << completer.textSearch().get<liboscar::TextSearch::ITEMS>(i).getName() << "\n";
		}
	}
	if (completer.textSearch().hasSearch(liboscar::TextSearch::GEOHIERARCHY)) {
		std::cout << "GeoHierarchy completers:\n"; 
		for(uint32_t i(0), s(completer.textSearch().size(liboscar::TextSearch::GEOHIERARCHY)); i < s; ++i) {
			std::cout << "\t" << i << "=" << completer.textSearch().get<liboscar::TextSearch::GEOHIERARCHY>(i).getName() << "\n";
		}
	}
	if (completer.textSearch().hasSearch(liboscar::TextSearch::GEOCELL)) {
		std::cout << "GeoCell completers:\n"; 
		for(uint32_t i(0), s(completer.textSearch().size(liboscar::TextSearch::GEOCELL)); i < s; ++i) {
			std::cout << "\t" << i << "=" << completer.textSearch().get<liboscar::TextSearch::GEOCELL>(i).trie().getName() << "\n";
		}
	}
	std::cout << std::flush;
}


void Worker::printStats(const WD_PrintStats & data) {
	if (data.printStats != PS_NONE) {
		if (data.fileName.size()) {
			std::ofstream of;
			of.open(data.fileName + "." + statsFileNameSuffix(data.printStats) + ".stats");
			if (!of.is_open()) {
				throw std::runtime_error("Failed to open " + data.fileName + "stats");
			}
			else {
				doPrintStats(of, completer, data.printStats);
				of.close();
			}
		}
		else {
			doPrintStats(*data.out, completer, data.printStats);
		}
	}
}

void Worker::printCQRDataSize(const WD_PrintCQRDataSize& data) {
	std::string qstr = data.value;
	const sserialize::Static::spatial::GeoHierarchy & gh = completer.store().geoHierarchy();
	const sserialize::Static::ItemIndexStore & idxStore = completer.indexStore();
	sserialize::Static::CellTextCompleter cmp( completer.textSearch().get<liboscar::TextSearch::Type::GEOCELL>() );
	sserialize::StringCompleter::QuerryType qt = sserialize::StringCompleter::normalize(qstr);
	sserialize::Static::CellTextCompleter::Payload::Type t = cmp.typeFromCompletion(qstr, qt);
	sserialize::ItemIndex fmIdx = idxStore.at( t.fmPtr() );
	sserialize::ItemIndex pmIdx = idxStore.at( t.pPtr() );
	sserialize::CellQueryResult r(fmIdx, pmIdx, t.pItemsPtrBegin(), gh, idxStore);
	std::vector<sserialize::UByteArrayAdapter::SizeType> idxDataSizes;
	std::vector<uint32_t> idxSizes;
	idxDataSizes.reserve(r.cellCount());
	idxSizes.reserve(r.cellCount());
	sserialize::UByteArrayAdapter::OffsetType idxDataSize = 0;
	sserialize::UByteArrayAdapter::OffsetType idxDataSizeForGhCellIdx = 0;
	for(sserialize::CellQueryResult::const_iterator it(r.begin()), end(r.end()); it != end; ++it) {
		const sserialize::ItemIndex & idx = *it;
		idxDataSize += idx.getSizeInBytes();
		idxDataSizes.push_back(idx.getSizeInBytes());
		idxSizes.push_back(idx.size());
		idxDataSizeForGhCellIdx += idxStore.dataSize( gh.cellItemsPtr( it.cellId() ) );
	}
	std::sort(idxDataSizes.begin(), idxDataSizes.end());
	std::sort(idxSizes.begin(), idxSizes.end());
	sserialize::UByteArrayAdapter::OffsetType totalDataSize = 0;
	totalDataSize = idxDataSize+fmIdx.getSizeInBytes() + pmIdx.getSizeInBytes();
	totalDataSize += t.data().data().size();
	
	std::cout << "Index data size for cqr: " << idxDataSize << std::endl;
	std::cout << "Index data size of gh-cell idexes: " << idxDataSizeForGhCellIdx << std::endl;
	std::cout << "Cell match index size: " << fmIdx.getSizeInBytes() + pmIdx.getSizeInBytes() << std::endl;
	std::cout << "Total data size: " << totalDataSize << std::endl;
	std::cout << "Idx data sizes:\n";
	std::cout << "\tmean:" << sserialize::statistics::mean(idxDataSizes.begin(), idxDataSizes.end(), (double)0.0) << "\n";
	std::cout << "\tmedian: " << idxDataSizes.at(idxDataSizes.size()/2) << "\n";
	std::cout << "\tmin: " << idxDataSizes.front() << "\n";
	std::cout << "\tmax: " << idxDataSizes.back() << "\n";
	std::cout << "Idx sizes:\n";
	std::cout << "\tmean:" << sserialize::statistics::mean(idxSizes.begin(), idxSizes.end(), (double)0.0) << "\n";
	std::cout << "\tmedian: " << idxSizes.at(idxSizes.size()/2) << "\n";
	std::cout << "\tmin: " << idxSizes.front() << "\n";
	std::cout << "\tmax: " << idxSizes.back() << "\n";
	std::cout << std::flush;
}

void Worker::printCTCSelectiveStorageStats(const WD_PrintCTCSelectiveStorageStats& data) {
	typedef sserialize::Static::CellTextCompleter CTC;
	sserialize::StringCompleter::QuerryType qt = sserialize::StringCompleter::QT_NONE;
	if (data.value == "exact") {
		qt = sserialize::StringCompleter::QT_EXACT;
	}
	else if (data.value == "prefix") {
		qt = sserialize::StringCompleter::QT_PREFIX;
	}
	else if (data.value == "suffix") {
		qt = sserialize::StringCompleter::QT_SUFFIX;
	}
	else if (data.value == "substring") {
		qt = sserialize::StringCompleter::QT_SUBSTRING;
	}
	else {
		std::cout << "Invalid type specified" << std::endl;
		return;
	}
	const sserialize::Static::ItemIndexStore & idxStore = completer.indexStore();
	auto ft = completer.textSearch().get<liboscar::TextSearch::GEOCELL>().trie().as<CTC::FlatTrieType>();
	if (ft) {
		std::unordered_set<uint32_t> idxIds;
		sserialize::UByteArrayAdapter::OffsetType treeStorageSize = 0;
		sserialize::ProgressInfo pinfo;
		const CTC::FlatTrieType::TrieType & ftp = ft->trie();
		pinfo.begin(ftp.size(), "Gathering CTC storage stats");
		for(uint32_t i=0, s(ftp.size()); i < s; ++i) {
			CTC::Payload p( ftp.at(i) );
			CTC::Payload::Type t( p.type(qt) );
			if (t.valid()) {
				uint32_t pIdxSize = idxStore.idxSize(t.pPtr());
				treeStorageSize += p.typeData(qt).size();
				
				idxIds.insert(t.fmPtr());
				idxIds.insert(t.pPtr());
				auto it = t.pItemsPtrBegin();
				for(uint32_t i(0); i < pIdxSize; ++i, ++it) {
					idxIds.insert(*it);
				}
			}
			pinfo(i);
		}
		pinfo.end();
		std::cout << "Number of different indexes for the tree-data: " << idxIds.size() << "\n";
		for(uint32_t i(0), s(completer.store().geoHierarchy().cellSize()); i < s; ++i) {
			idxIds.insert(completer.store().geoHierarchy().cellItemsPtr(i));
		}
		sserialize::UByteArrayAdapter::OffsetType idxDataSize = 0;
		for(uint32_t x : idxIds) {
			idxDataSize += idxStore.dataSize(x);
		}
		idxIds.clear();

		std::cout << "Tree value data size: " << treeStorageSize << "\n";
		std::cout << "Index data size: " << idxDataSize << "\n";
		std::cout << "Total minimum data size: " << sserialize::prettyFormatSize(treeStorageSize+idxDataSize) << std::endl;
	}
	else {
		std::cout << "CTC is unsupported" << std::endl;
	}
}

void Worker::printCTCStorageStats(const WD_PrintCTCStorageStats & data) {
	if (!completer.textSearch().hasSearch(liboscar::TextSearch::GEOCELL))
		return;

	std::ofstream outfile;
	outfile.open(data.value);
	if (!outfile.is_open()) {
		throw std::runtime_error("Failed to open file: " + data.value);
	}

	typedef sserialize::Static::CellTextCompleter CTC;
	struct Stats {
		std::unordered_map<uint32_t, std::vector<sserialize::UByteArrayAdapter::OffsetType> > index;
		std::vector<sserialize::UByteArrayAdapter::OffsetType> tree;
		std::unordered_set<uint32_t> fmPmIndexIds;
		Stats() : tree(1024, 0) {
			index[sserialize::StringCompleter::QT_EXACT] = tree;
			index[sserialize::StringCompleter::QT_PREFIX] = tree;
			index[sserialize::StringCompleter::QT_SUFFIX] = tree;
			index[sserialize::StringCompleter::QT_SUBSTRING] = tree;
		};
	} stats;
	const sserialize::Static::ItemIndexStore & idxStore = completer.indexStore();
	auto ft = completer.textSearch().get<liboscar::TextSearch::GEOCELL>().trie().as<CTC::FlatTrieType>();
	if (ft) {
		std::cout << "Gathering CTC storage stats" << std::endl;
		struct MyRec {
			const sserialize::Static::ItemIndexStore & idxStore;
			Stats & stats;
			const CTC::FlatTrieType::TrieType & ftp;
			uint32_t longestString;
			void calc(const sserialize::Static::UnicodeTrie::FlatTrieBase::Node & node, int strlen) {
				uint32_t nodeId = node.id();
				
				stats.tree.at(strlen+1) += ftp.payloads().dataSize(nodeId);
				
				CTC::Payload p( ftp.at(nodeId) );
				for(auto & x : stats.index) {
					if (p.types() & x.first) {
						CTC::Payload::Type t( p.type((sserialize::StringCompleter::QuerryType) x.first) );
						stats.fmPmIndexIds.insert(t.fmPtr());
						stats.fmPmIndexIds.insert(t.pPtr());
						sserialize::UByteArrayAdapter::OffsetType tmp = 0;
						tmp += idxStore.dataSize(t.fmPtr()) + idxStore.dataSize(t.pPtr());
						auto it = t.pItemsPtrBegin();
						for(uint32_t i(0), s(idxStore.idxSize(t.pPtr())); i < s; ++i, ++it) {
							tmp += idxStore.dataSize(*it);
						}
						x.second.at(strlen+1) += tmp;
					}
				}
				longestString = std::max<uint32_t>(longestString, strlen+1);
				
				uint32_t nodePathStrLen = ftp.strSize(nodeId);
				for(auto child : node) {
					calc(child, nodePathStrLen);
				}
			}
		};
		MyRec myrec(
			{
				.idxStore = idxStore,
				.stats = stats,
				.ftp = ft->trie(),
				.longestString = 0
			}
		);
		myrec.calc(myrec.ftp.root(), -1);
		for(uint32_t i(0); i <= myrec.longestString; ++i) {
			outfile << stats.tree.at(i) << ";";
			outfile << stats.index.at(sserialize::StringCompleter::QT_EXACT).at(i) << ";";
			outfile << stats.index.at(sserialize::StringCompleter::QT_PREFIX).at(i) << ";";
			outfile << stats.index.at(sserialize::StringCompleter::QT_SUFFIX).at(i) << ";";
			outfile << stats.index.at(sserialize::StringCompleter::QT_SUBSTRING).at(i) << "\n";
		}
		sserialize::UByteArrayAdapter::OffsetType fmPmIndexDataSize = 0;
		sserialize::UByteArrayAdapter::OffsetType cellIndexSizes = 0;
		for(uint32_t x : stats.fmPmIndexIds) {
			fmPmIndexDataSize += completer.indexStore().dataSize(x);
		}
		for(uint32_t i(0), s(completer.store().geoHierarchy().cellSize()); i < s; ++i) {
			stats.fmPmIndexIds.insert(completer.store().geoHierarchy().cellItemsPtr(i));
		}
		
		for(uint32_t x : stats.fmPmIndexIds) {
			cellIndexSizes += completer.indexStore().dataSize(x);
		}
		std::cout << "Data size of fm and pm cell indexes: " << fmPmIndexDataSize << std::endl;
		std::cout << "With gh cell indexes: " << cellIndexSizes << std::endl;
	}
	else {
		std::cout << "CTC is unsupported" << std::endl;
	}
}

void Worker::printPaperStatsDb(const WD_PrintPaperStatsDb & data) {

	std::ifstream file;
	file.open(data.value);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open key file: " + data.value);
	}

	uint64_t rawValueSizes = 0;
	uint64_t flattenedValueSizes = 0;
	
	std::unordered_set<uint32_t> keyIds;
	std::unordered_set<uint32_t> regionValueIds;
		
	while (!file.eof()) {
		std::string key;
		std::getline(file, key);
		if (key.size()) {
			keyIds.insert( sserialize::narrow_check<uint32_t>( completer.store().keyStringTable().find(key) ));
		}
	}
	file.close();

	auto vst = completer.store().valueStringTable();
	auto gh = completer.store().geoHierarchy();
	auto idxStore = completer.indexStore();
	auto cellPtrs = gh.cellPtrs();
	sserialize::ProgressInfo pinfo;
	pinfo.begin(completer.store().size(), "Analyzing");
	for(uint32_t i(0), s(completer.store().size()); i < s; ++i) {
		regionValueIds.clear();
		auto item = completer.store().at(i);
		for(uint32_t j(0), js(item.size()); j < js; ++j) {
			if (keyIds.count(item.keyId(j))) {
				rawValueSizes += vst.strSize(item.valueId(j));
			}
		}
		for(auto cellId : item.cells()) {
			for(uint32_t j(gh.cellParentsBegin((uint32_t) cellId)), js(gh.cellParentsEnd((uint32_t) cellId)); j < js; ++j) {
				uint32_t regionId = gh.ghIdToStoreId(cellPtrs.at(j));
				auto ritem = completer.store().at(regionId);
				for(uint32_t k(0), ks(ritem.size()); k < ks; ++k) {
					if (keyIds.count(ritem.keyId(k))) {
						regionValueIds.insert(ritem.valueId(k));
					}
				}
			}
		}
		for(uint32_t x : regionValueIds) {
			flattenedValueSizes += vst.strSize(x);
		}
		pinfo(i);
	}
	pinfo.end();
	
	std::cout << "item value size for selected keys: " << rawValueSizes << "\n";
	std::cout << "inherited value size for selected keys: " << flattenedValueSizes << "\n";
	std::cout << "inherited+item value size for selected keys: " << flattenedValueSizes+rawValueSizes << "\n";
}

void Worker::printPaperStatsGh(const WD_PrintPaperStatsGh& data) {
	std::ofstream outFile;
	outFile.open(data.value);
	if (!outFile.is_open()) {
		throw std::runtime_error("Could not open outfile");
		return;
	}
	const sserialize::Static::spatial::GeoHierarchy & gh = completer.store().geoHierarchy();
	std::vector< std::pair<uint32_t, uint32_t> > cellItemSizes(gh.cellSize());
	for(uint32_t i(0), s(gh.cellSize()); i < s; ++i) {
		uint32_t cellItemCount = gh.cellItemsCount(i);
		auto & x = cellItemSizes[i];
		x.first = cellItemCount;
		x.second = i;
	}
	std::sort(cellItemSizes.begin(), cellItemSizes.end());
	for(auto x : cellItemSizes) {
		outFile << x.first << ";" << x.second << "\n";
	}
	outFile.close();
}

void Worker::printCellNeighborStats(const WD_PrintCellNeighborStats& data) {
	const auto & gh = completer.store().geoHierarchy();
	const auto & ra = completer.store().regionArrangement();
	const auto & cg = completer.store().cellGraph();
	const auto & cm = completer.store().cellCenterOfMass();
	const auto & tds = ra.tds();

	std::ofstream outFile;
	uint32_t maxFactor;
	{
		std::vector<std::string> tmp( sserialize::split< std::vector<std::string> >(data.value, (uint32_t)',') );
		if (tmp.size() == 0) {
			throw std::runtime_error("Invalid config option for cell neighbor stats");
		}
		maxFactor = std::atoi(tmp.at(0).c_str());
		if (tmp.size() >= 2) {
			outFile.open(tmp.at(1));
			if (!outFile.is_open()) {
				throw std::runtime_error("Could not open file " + tmp.at(1) + " to write the cell neighbor stats");
			}
		}
	}
	
	//number of neighbors a cell has within a radius of first*cellDiameter
	std::map<uint32_t, std::vector<uint32_t> > cnCount;
	
	for(uint32_t i(1); i < maxFactor; i *= 2) {
		cnCount[i] = std::vector<uint32_t>(cg.size(), 0);
	}
	
	uint32_t cgSize = cg.size();
	sserialize::ProgressInfo pinfo;
	
	pinfo.begin(cgSize);
	uint32_t pCounter = 0;
	#pragma omp parallel for schedule(dynamic, 1)
	for(uint32_t cellId = 0; cellId < cgSize; ++cellId) {
		sserialize::spatial::GeoPoint cellCM( cm.at(cellId) );
		sserialize::spatial::GeoRect cellRect( gh.cellBoundary(cellId) );
		auto ccm = cm.at(cellId);
		double cellDiag = cellRect.diagInM();
		double maxDist = cellDiag * maxFactor;
		
		std::unordered_set<uint32_t> neighborCells;
		tds.explore(ra.faceIdFromCellId(cellId), [&neighborCells, &tds, &ccm, &ra, &maxDist](const sserialize::Static::spatial::Triangulation::Face & face) -> bool {
			neighborCells.insert(ra.cellIdFromFaceId(face.id()));
			auto fc = face.centroid();
			return std::abs<double>( sserialize::spatial::distanceTo(fc.lat(), fc.lon(), ccm.lat(), ccm.lon()) ) < maxDist;
		});
		//now sort the cells by their distance
		std::vector<double> cellDistances;
		cellDistances.reserve(neighborCells.size());
		for(uint32_t ncId : neighborCells) {
			auto nccm = cm.at(ncId);
			cellDistances.push_back(
				std::abs<double>(
					sserialize::spatial::distanceTo(nccm.lat(), nccm.lon(), ccm.lat(), ccm.lon())
				)
			);
		}
		std::sort(cellDistances.begin(), cellDistances.end());
		
		std::map<uint32_t, std::vector<uint32_t> >::iterator dIt(cnCount.begin()), dEnd(cnCount.end());
		std::vector<double>::const_iterator cdIt(cellDistances.begin()), cdEnd(cellDistances.end());
		uint32_t count = 0;
		for(; dIt != dEnd && cdIt != cdEnd; ) {
			if (*cdIt/cellDiag < dIt->first) {
				++count;
				++cdIt;
			}
			else { //flush
				dIt->second.at(cellId) = count;
				++dIt;
			}
		}
		#pragma omp atomic
		++pCounter;
		#pragma omp critical
		{
			pinfo(pCounter);
		}
	}
	pinfo.end();
	
	struct StatsEntry {
		uint32_t factor;
		uint32_t min;
		uint32_t max;
		uint32_t mean;
		uint32_t median;
	};
	std::vector<StatsEntry> stats;
	
	for(const auto & x : cnCount) {
		if (!x.second.size()) {
			continue;
		}
		StatsEntry se;
		
		se.factor = x.first;
		se.min = *std::min_element(x.second.begin(), x.second.end());
		se.max = *std::max_element(x.second.begin(), x.second.end());
		se.mean = sserialize::statistics::mean(x.second.begin(), x.second.end(), 0);
		se.median = sserialize::statistics::median(x.second.begin(), x.second.end(), 0);
		
		stats.push_back(se);
	}

	std::ostream & out = (outFile.is_open() ? outFile : std::cout);
	char sep;
#define PRINT_STATS(__DESC, __VAR) \
	out << __DESC << ":"; \
	sep = ' '; \
	for(const StatsEntry & se : stats) { \
		out << sep << se.__VAR; \
		sep = ','; \
	} \
	out << std::endl; \

	PRINT_STATS("Factor", factor)
	PRINT_STATS("Min", min)
	PRINT_STATS("Max", max)
	PRINT_STATS("Mean", mean)
	PRINT_STATS("Median", median)
#undef PRINT_STATS
}

void Worker::printCellStats(const WD_PrintCellStats& /*data*/) {
	const sserialize::Static::spatial::GeoHierarchy & gh = completer.store().geoHierarchy();
	const sserialize::Static::ItemIndexStore & idxStore = completer.indexStore();
	std::vector<uint32_t> cellDirectParents;
	std::vector<uint32_t> cellParents;
	std::vector<uint32_t> regionExclusiveCells;
	std::vector<uint32_t> regionCellCounts;
	for(uint32_t i(0), s(gh.cellSize()); i < s; ++i) {
		cellDirectParents.emplace_back(gh.cellDirectParentsEnd(i) - gh.cellParentsBegin(i));
		cellParents.emplace_back(gh.cellParentsSize(i));
	}
	for(uint32_t i(0), s(gh.regionSize()); i < s; ++i) {
		uint32_t cIdxPtr = gh.regionCellIdxPtr(i);
		uint32_t recIdxPtr = gh.regionExclusiveCellIdxPtr(i);
		regionExclusiveCells.emplace_back(idxStore.idxSize(recIdxPtr));
		regionCellCounts.emplace_back(idxStore.idxSize(cIdxPtr));
	}
	std::cout << "Cell direct parents counts:" << std::endl;
	sserialize::statistics::StatPrinting::print(std::cout, cellDirectParents.begin(), cellDirectParents.end());
	std::cout << "Cell parents counts:" << std::endl;
	sserialize::statistics::StatPrinting::print(std::cout, cellParents.begin(), cellParents.end());
	std::cout << "Region exclusive cell counts: " << std::endl;
	sserialize::statistics::StatPrinting::print(std::cout, regionExclusiveCells.begin(), regionExclusiveCells.end());
	std::cout << "Region cell counts: " << std::endl;
	sserialize::statistics::StatPrinting::print(std::cout, regionCellCounts.begin(), regionCellCounts.end());

}

void Worker::dumpAllItemTagsWithInheritedTags(const WD_DumpAllItemTagsWithInheritedTags& data) {

	std::unordered_set<uint32_t> keyIds;
	std::unordered_set<uint32_t> regionValueIds;
	{
		std::ifstream file;
		file.open(data.keyfile);
		if (!file.is_open()) {
			throw std::runtime_error("Failed to open key file: " + data.keyfile);
		}
		while (!file.eof()) {
			std::string key;
			std::getline(file, key);
			if (key.size()) {
				keyIds.insert( sserialize::narrow_check<uint32_t>( completer.store().keyStringTable().find(key) ) );
			}
		}
		file.close();
	}
	std::ofstream file;
	file.open(data.outfile);

	auto vst = completer.store().valueStringTable();
	auto gh = completer.store().geoHierarchy();
	auto idxStore = completer.indexStore();
	auto cellPtrs = gh.cellPtrs();
	sserialize::ProgressInfo pinfo;
	pinfo.begin(completer.store().size(), "Writing");
	for(uint32_t i(0), s(completer.store().size()); i < s; ++i) {
		file << "itemid:" << i;
		regionValueIds.clear();
		auto item = completer.store().at(i);
		for(auto cellId : item.cells()) {
			for(uint32_t j(gh.cellParentsBegin((uint32_t) cellId)), js(gh.cellParentsEnd((uint32_t) cellId)); j < js; ++j) {
				uint32_t regionId = gh.ghIdToStoreId(cellPtrs.at(j));
				auto ritem = completer.store().at(regionId);
				for(uint32_t k(0), ks(ritem.size()); k < ks; ++k) {
					if (keyIds.count(ritem.keyId(k))) {
						regionValueIds.insert(ritem.valueId(k));
					}
				}
			}
		}
		for(uint32_t x : regionValueIds) {
			file << vst.at(x);
		}
		for(uint32_t j(0), js(item.size()); j < js; ++j) {
			if (keyIds.count(item.keyId(j))) {
				file << item.value(j);
			}
		}
		file.put('\0');
		pinfo(i);
	}
	pinfo.end();
	
}

void Worker::selectTextCompleter(WD_SelectTextCompleter & d) {
	if(!completer.setTextSearcher((liboscar::TextSearch::Type)d.first, d.second)) {
		std::cout << "Failed to set selected completer: " << (uint32_t)d.first << ":" << (uint32_t)d.second << std::endl;
	}
}

void Worker::selectGeoCompleter(WD_SelectGeoCompleter & d) {
	if (!completer.setGeoCompleter(d.value)) {
		std::cout << "Failed to set seleccted geo completer: " << d.value << std::endl;
	}
}

void Worker::lockMemory(WD_LockMemory& d) {
	auto fc = liboscar::fileConfigFromString(d.value);
	if (fc != liboscar::FC_INVALID) {
		auto data = completer.data(fc);
		data.advice(sserialize::UByteArrayAdapter::AT_LOCK, data.size());
	}
}

void Worker::unlockMemory(WD_UnlockMemory& d) {
	auto fc = liboscar::fileConfigFromString(d.value);
	if (fc != liboscar::FC_INVALID) {
		auto data = completer.data(fc);
		data.advice(sserialize::UByteArrayAdapter::AT_UNLOCK, data.size());
	}
}

void Worker::printSelectedTextCompleter() {
	if (completer.textSearch().hasSearch(liboscar::TextSearch::ITEMS) ) {
		std::cout << "Selected items text completer: " << completer.textSearch().get<liboscar::TextSearch::ITEMS>().getName() << std::endl;
	}
	if (completer.textSearch().hasSearch(liboscar::TextSearch::GEOHIERARCHY)) {
		std::cout << "Selected geohierarchy text completer: " << completer.textSearch().get<liboscar::TextSearch::GEOHIERARCHY>().getName() << std::endl;
	}
}

void Worker::printSelectedGeoCompleter() {
	std::cout << "Selected Geo completer: " << completer.geoCompleter()->describe() << std::endl; 
}

void Worker::consistencyCheck(WD_ConsistencyCheck & d) {
	if (d.value == "index") {
		if (!ConsistencyChecker::checkIndex(completer.indexStore())) {
			std::cout << "Index Store is BROKEN" << std::endl;
		}
		else {
			std::cout << "Index Store is OK" << std::endl;
		}
	}
	else if (d.value == "store") {
		if (!ConsistencyChecker::checkStore(completer.store())) {
			std::cout << "Store is BROKEN" << std::endl;
		}
		else {
			std::cout << "Store is OK" << std::endl;
		}
	}
	else if (d.value == "gh") {
		if (!ConsistencyChecker::checkGh(completer.store(), completer.indexStore())) {
			std::cout << "GeoHierarchy is BROKEN" << std::endl;
		}
		else {
			std::cout << "GeoHierarchy is OK" << std::endl;
		}
	}
	else if (d.value == "tds") {
		if (!ConsistencyChecker::checkTriangulation(completer.store())) {
			std::cout << "Triangulation is BROKEN" << std::endl;
		}
		else {
			std::cout << "Triangulation is OK" << std::endl;
		}
	}
}

void Worker::ghId2StoreId(WD_GhId2StoreId& d) {
	std::cout << "GhId2storeId: " << d.value << "->" << completer.store().geoHierarchy().ghIdToStoreId(d.value) << std::endl;
}

void Worker::storeId2GhId(WD_GhId2StoreId& d) {
	std::cout << "StoreId2ghId: " << d.value << "->" << completer.store().geoHierarchy().storeIdToGhId(d.value) << std::endl;
}

void Worker::dumpIndex(WD_DumpIndex& d) {
	std::cout << completer.indexStore().at(d.value) << std::endl;
}

void Worker::dumpItem(WD_DumpItem& d) {
	completer.store().at(d.value).print(std::cout, true);
	std::cout << std::endl;
}

void Worker::dumpAllItems(WD_DumpAllItems& d) {
	for(uint32_t i(0), s(completer.store().size()); i < s; ++i) {
		completer.store().at(i).print(std::cout, d.value);
		std::cout << std::endl;
	}
}

void Worker::dumpGhRegion(WD_DumpGhRegion& d) {
	std::cout << completer.store().geoHierarchy().regionFromStoreId(d.value) << std::endl;
}

void Worker::dumpGhCell(WD_DumpGhCell& d) {
	std::cout << completer.store().geoHierarchy().cell(d.value) << std::endl;
}

void Worker::dumpGhCellParents(WD_DumpGhCellParents& d) {
	const sserialize::Static::spatial::GeoHierarchy & gh = completer.store().geoHierarchy();
	sserialize::Static::spatial::GeoHierarchy::Cell c = gh.cell(d.value);
	for(uint32_t i(0), s(c.parentsSize()); i < s; ++i) {
		uint32_t parentGhId = c.parent(i);
		uint32_t parentStoreId = gh.ghIdToStoreId(parentGhId);
		completer.store().at(parentStoreId).print(std::cout, false);
		std::cout << "\n";
	}
}

void Worker::dumpGhCellItems(WD_DumpGhCellItems& d) {
	sserialize::ItemIndex cItems = completer.indexStore().at(completer.store().geoHierarchy().cell(d.value).itemPtr());
	for(uint32_t i : cItems) {
		completer.store().at(i).print(std::cout, false);
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

void Worker::dumpGhRegionChildren(WD_DumpGhRegionChildren& d) {
	sserialize::Static::spatial::GeoHierarchy::Region r;
	if (d.value == 0xFFFFFFFF) {
		r = completer.store().geoHierarchy().rootRegion();
		std::cout << "Root-region has the following children: " << std::endl;
	}
	else {
		r = completer.store().geoHierarchy().region(d.value);
		completer.store().at(r.storeId()).print(std::cout, false);
		std::cout << "\nhas the following children:" << std::endl;
	}
	
	for(uint32_t i(0), s(r.childrenSize()); i < s; ++i) {
		uint32_t childRegionId = r.child(i);
		uint32_t childStoreId = completer.store().geoHierarchy().ghIdToStoreId(childRegionId);
		completer.store().at(childStoreId).print(std::cout, false);
		std::cout << std::endl;
	}
}

void Worker::dumpGhRegionItems(WD_DumpGhRegionItems& d) {
	sserialize::ItemIndex rItems = completer.indexStore().at(completer.store().geoHierarchy().region(d.value).itemsPtr());
	for(uint32_t i : rItems) {
		completer.store().at(i).print(std::cout, false);
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

void Worker::dumpGh(WD_DumpGh & d) {
	std::ofstream outPrintFile;
	outPrintFile.open("children" + d.value);
	if (outPrintFile.is_open()) {
		GeoHierarchyPrinter printer;
		std::stringstream tmp;
		printer.printChildrenWithNames(tmp, completer.store());
		outPrintFile << tmp.rdbuf();
		outPrintFile.close();
	}
	else {
		throw std::runtime_error("Failed to open printfile: children " + d.value);
	}
	outPrintFile.open("parents" + d.value);
	if (outPrintFile.is_open()) {
		GeoHierarchyPrinter printer;
		std::stringstream tmp;
		printer.printParentsWithNames(tmp, completer.store());
		outPrintFile << tmp.rdbuf();
		outPrintFile.close();
	}
	else {
		throw std::runtime_error("Failed to open printfile: parents " + d.value);
	}
}

void Worker::dumpGhPath(WD_DumpGhPath & d) {
	sserialize::Static::spatial::GeoHierarchy::Region r = completer.store().geoHierarchy().rootRegion();
	for(std::vector<uint32_t>::const_iterator it(d.value.begin()), end(d.value.end()); it != end; ++it) {
		if (*it < r.childrenSize()) {
			r = completer.store().geoHierarchy().region(r.child(*it));
		}
		else {
			throw std::runtime_error("DumpGhPath: invalid path");
		}
		std::cout << "Region: " << std::endl;
		std::cout << "ghId=" << r.ghId() << std::endl;
		std::cout << "storeId=" << r.storeId() << std::endl;
		std::cout << "children.size=" << r.childrenSize() << std::endl;
		std::cout << "parents.size=" << r.parentsSize() << std::endl;
		std::cout << "cellIndex.size=" << completer.indexStore().at(r.cellIndexPtr()).size() << std::endl;
	}
}

void Worker::dumpKeyStringTable(WD_DumpKeyStringTable & d) {
	std::ofstream dksOut;
	dksOut.open(d.value.c_str());
	if (dksOut.is_open()) {
		for(auto x : completer.store().keyStringTable()) {
			dksOut << x << std::endl;
		}
		dksOut.close();
	}
}

void Worker::dumpValueStringTable(WD_DumpValueStringTable& d) {
	std::ofstream dvsOut;
	dvsOut.open(d.value.c_str());
	if (dvsOut.is_open()) {
		for(auto x : completer.store().valueStringTable()) {
			dvsOut << x << std::endl;
		}
		dvsOut.close();
	}
}

void Worker::dumpItemTags(WD_DumpItemTags& d) {
	std::ofstream ditOut;
	ditOut.open(d.value.c_str());
	if (ditOut.is_open()) {
		const liboscar::Static::OsmKeyValueObjectStore & store = completer.store();
		liboscar::Static::OsmKeyValueObjectStore::Item item;
		std::string tag;
		sserialize::ProgressInfo pinfo;
		pinfo.begin(store.size(), "Dumping item tags");
		for(uint32_t i(0), s(store.size()); i < s; ++i) {
			item = store.at(i);
			for(uint32_t j(0), js(item.size()); j < js; ++j) {
				tag = item.key(j) + ":" + item.value(j);
				std::replace(tag.begin(), tag.end(), '\n', '_');
				ditOut << tag << '\n';
			}
			pinfo(i);
		}
		pinfo.end();
		ditOut.close();
	}
}

void Worker::interactivePartial(WD_InteractivePartial & d) {
	LiveCompletion liveCompleter(completer);
	liveCompleter.doPartialCompletion(d.seek);
}

void Worker::interactiveSimple(WD_InteractiveSimple & d) {
	LiveCompletion liveCompleter(completer);
	liveCompleter.doSimpleCompletion(d.maxResultSetSize, d.minStrLen, d.printNumResults);
}

void Worker::interactiveFull(WD_InteractiveFull& d) {
	LiveCompletion liveCompleter(completer);
	liveCompleter.doFullCompletion(d.value);
}

inline std::string escapeString(const std::set<char> & escapeChars, const std::string & istr) {
	std::string ostr = "";
	for(std::string::const_iterator it = istr.begin(); it != istr.end(); ++it) {
		if (escapeChars.count(*it) > 0) {
			ostr += '\\';
		}
		ostr += *it;
	}
	return ostr;
}


template<typename T_OUTPUT_ITERATOR>
void createCompletionStrings2(const liboscar::Static::OsmKeyValueObjectStore & db, sserialize::StringCompleter::QuerryType qt, uint32_t count, T_OUTPUT_ITERATOR out) {
	std::string escapeStr = "-+/\\^$[]() ";
	sserialize::AsciiCharEscaper escaper(escapeStr.begin(), escapeStr.end());

	if (qt & sserialize::StringCompleter::QT_SUBSTRING) {
		for(uint32_t i = 0; i < count; i++) {
			uint32_t itemId = ((double)rand()/RAND_MAX)*db.size();
			liboscar::Static::OsmKeyValueObjectStore::Item item( db.at(itemId) );
			std::string str = "";
			for(uint32_t j = 0; j < item.strCount(); j++) {
				std::string s = item.strAt(j);
				if (s.size()) {
					if (*(s.begin()) == '\"') {
						s.erase(0,1);
						if (s.size() > 0 && *(s.rbegin()) == '\"') {
							s.erase(s.size()-1, 1);
						}
					}
					str += escaper.escape(s) + " ";
				}
			}
			if (qt & sserialize::StringCompleter::QT_CASE_INSENSITIVE) {
				str = sserialize::unicode_to_lower(str);
			}
			if (str.size() > 3) {
				*out = str;
				++out;
			}
		}
	}
}

void Worker::createCompletionStrings(WD_CreateCompletionStrings & d) {
	if (d.outFileName.empty()) {
		throw std::runtime_error("Create completion strings: out file not specified");
	}
	std::ofstream of;
	of.open(d.outFileName);
	if (!of.is_open()) {
		throw std::runtime_error("Create completion strings: unable to open outfile=" + d.outFileName);
	}
	std::unordered_set<std::string> result;
	sserialize::StringCompleter::QuerryType qt = (sserialize::StringCompleter::QuerryType) (sserialize::StringCompleter::QT_SUBSTRING | sserialize::StringCompleter::QT_CASE_INSENSITIVE);
	std::insert_iterator< std::unordered_set<std::string> > resInsertIt(result, result.end());
	createCompletionStrings2(completer.store(), qt, d.count, resInsertIt);
	
	for(const auto & cmpStr : result) {
		if (d.simulate) {
			std::string s = "";
			for(auto cmpStrChar : cmpStr) {
				s += cmpStrChar;
				of << s << std::endl;
			}
		}
		else {
			of << cmpStr << std::endl;
		}
	}
}

template<typename T_OUTPUT_ITERATOR>
void readCompletionStringsFromFile(const std::string & fileName, T_OUTPUT_ITERATOR out) {
	std::string tmp;
	std::ifstream inFile;
	inFile.open(fileName);
	if (!inFile.is_open()) {
		throw std::runtime_error("Failed to read completion strings from " + fileName);
	}
	while (!inFile.eof()) {
		std::getline(inFile, tmp);
		*out = tmp;
		++out;
	}
	inFile.close();
}

void Worker::cellsFromQuery(WD_CellsFromQuery& d) {
	if (!completer.textSearch().hasSearch(liboscar::TextSearch::GEOCELL)) {
		throw sserialize::UnsupportedFeatureException("Data has no geocell text search");
	}
	auto cqr = completer.cqrComplete(d.value, false, d.threadCount);
	auto tcqr = completer.cqrComplete(d.value, true, d.threadCount);
	
	if (cqr != tcqr) {
		throw sserialize::BugException("cqr and tcqr differ");
	}
	
	for(uint32_t i(0), s(cqr.cellCount()); i < s; ++i) {
		std::cout << cqr.cellId(i) << ':' << (cqr.fullMatch(i) ? 'f' : 'p') << '\n';
	}
}

void Worker::cellImageFromQuery(WD_CellImageFromQuery& d) {
	if (!completer.textSearch().hasSearch(liboscar::TextSearch::GEOCELL)) {
		throw sserialize::UnsupportedFeatureException("Data has no geocell text search");
	}
	std::string outFile;
	uint32_t latpix(0);
	bool heatMap  = false;
	sserialize::spatial::GeoRect bounds;
	{
		std::vector<std::string> tmp( sserialize::split< std::vector<std::string> >(d.cfg, ';') );
		if (tmp.size() < 2) {
			throw sserialize::ConfigurationException("cellImageFromQuery", "Invalid config");
		}

		if (tmp.size() >= 2) {
			outFile = tmp.at(0);
			latpix = ::atoi(tmp.at(1).c_str());
		}
		if (tmp.size() >= 3) {
			if (tmp.at(2) == "heat") {
				heatMap = true;
			}
			else if(tmp.at(2) == "cells") {
				heatMap = false;
			}
		}
		if (tmp.size() >= 4) {
			bounds = sserialize::spatial::GeoRect(tmp.at(3), true);
		}
	}
	
	if (latpix == 0) {
		std::cout << "Will not create image with size of 0 pixels" << std::endl;
		return;
	}
	
	auto cqr = completer.cqrComplete(d.query);
	
	if (!cqr.cellCount()) {
		std::cout << "Empty query result. Will not create image." << std::endl;
	}
	
	const sserialize::Static::spatial::GeoHierarchy & gh = completer.store().geoHierarchy();
	const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement & ra = completer.store().regionArrangement();
	
	uint32_t largestCell = cqr.idxSize(0);
	for(uint32_t i(1), s(cqr.cellCount()); i < s; ++i) {
		largestCell = std::max<uint32_t>(largestCell, cqr.idxSize(i));
	}
	
	if (!bounds.valid()) {
		auto boundsCalculator = [&bounds](const sserialize::Static::spatial::Triangulation::Face & f) -> void {
			for(int j=0; j < 3; ++j) {
				bounds.enlarge(f.point(j).boundary());
			}
		};
		bounds.enlarge(gh.cellBoundary(cqr.cellId(0)));
		for(uint32_t i(1), s(cqr.cellCount()); i < s; ++i) {
			largestCell = std::max<uint32_t>(largestCell, cqr.idxSize(i));
			ra.cfGraph(cqr.cellId(i)).visitCB(boundsCalculator);
		}
	}
	SSERIALIZE_CHEAP_ASSERT(bounds.valid());
	
	CairoRenderer cr;
	CairoRenderer::Color color(255, 0, 0, 255);
	cr.init(bounds, latpix);

	auto drawVisitor = [&cr, &color](const sserialize::Static::spatial::Triangulation::Face & f) -> void {
		cr.draw(f.point(0), f.point(1), f.point(2), color);
	};
	
	for(uint32_t i(0), s(cqr.cellCount()); i < s; ++i) {
		uint32_t cellId = cqr.cellId(i);
		if (heatMap) {
			color.setRGB(256 - (double)cqr.idxSize(i) / largestCell * 255);
		}
		else {
			color.randomize();
		}
		ra.cfGraph(cellId).visitCB(drawVisitor);
	}
	
	cr.toPng(outFile);
}

void Worker::completeStringPartial(WD_CompleteStringPartial & d) {
	std::vector<std::string> strs(1, d.str);
	LiveCompletion liveCompleter(completer);
	liveCompleter.doPartialCompletion(strs, d.seek, d.printNumResults);
}

void Worker::completeStringSimple(WD_CompleteStringSimple& d) {
	std::vector<std::string> strs(1, d.str);
	LiveCompletion liveCompleter(completer);
	liveCompleter.doSimpleCompletion(strs, d.maxResultSetSize, d.minStrLen, d.printNumResults);
}

void Worker::completeStringFull(WD_CompleteStringFull& d) {
	std::vector<std::string> strs(1, d.str);
	LiveCompletion liveCompleter(completer);
	liveCompleter.doFullCompletion(strs, d.printNumResults);
}

void Worker::completeStringClustered(WD_CompleteStringClustered & d, bool treedCQR) {
	std::vector<std::string> strs(1, d.str);
	LiveCompletion liveCompleter(completer);
	liveCompleter.doClusteredComplete(strs, d.printNumResults, treedCQR, d.threadCount);
}

void Worker::completeFromFilePartial(WD_CompleteFromFilePartial & d) {
	std::vector<std::string> strs;
	readCompletionStringsFromFile(d.fileName, std::back_insert_iterator< std::vector<std::string> >(strs));
	LiveCompletion liveCompleter(completer);
	liveCompleter.doPartialCompletion(strs, d.seek, d.printNumResults);
}

void Worker::completeFromFileSimple(WD_CompleteFromFileSimple& d) {
	std::vector<std::string> strs;
	readCompletionStringsFromFile(d.fileName, std::back_insert_iterator< std::vector<std::string> >(strs));
	LiveCompletion liveCompleter(completer);
	liveCompleter.doSimpleCompletion(strs, d.maxResultSetSize, d.minStrLen, d.printNumResults);
}

void Worker::completeFromFileFull(WD_CompleteFromFileFull& d) {
	std::vector<std::string> strs;
	readCompletionStringsFromFile(d.fileName, std::back_insert_iterator< std::vector<std::string> >(strs));
	LiveCompletion liveCompleter(completer);
	liveCompleter.doFullCompletion(strs, d.printNumResults);
}

void Worker::completeFromFileClustered(WD_CompleteFromFileClustered & d, bool treedCQR) {
	std::vector<std::string> strs;
	readCompletionStringsFromFile(d.fileName, std::back_insert_iterator< std::vector<std::string> >(strs));
	LiveCompletion liveCompleter(completer);
	liveCompleter.doClusteredComplete(strs, d.printNumResults, treedCQR, d.threadCount);
}

void Worker::symDiffCompleters(WD_SymDiffItemsCompleters& d) {
	LiveCompletion lc(completer);
	lc.symDiffItemsCompleters(d.str, d.completer1, d.completer2,d.printNumResults);
}

void Worker::benchmark(WD_Benchmark & d) {
	Benchmarker bm(completer);
	bm.benchmark(d.config);
}

}//end namespace