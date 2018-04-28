#include "Benchmark.h"
#include <sserialize/strings/stringfunctions.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/stats/statfuncs.h>
#include <sserialize/stats/ProgressInfo.h>
#include <sserialize/iterator/TransformIterator.h>
#include <liboscar/AdvancedCellOpTree.h>
#include <sserialize/utility/exceptions.h>
#include <limits>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>

#include "DecelledACOT.h"

namespace oscarcmd {
	
const char * Benchmarker::Stats::meas_res_unit = "us";

Benchmarker::Config::Config(const std::string & str) :
Config()
{
	std::vector<std::string> splitted = sserialize::split< std::vector<std::string> >(str, ',', '\\');
	for(const std::string & splitString : splitted) {
		std::vector<std::string> realOpts = sserialize::split< std::vector<std::string> >(splitString, '=', '\\');
		if (realOpts.size() != 2) {
			throw sserialize::ConfigurationException("Benchmarker", "Invalid option:" + splitString);
		}
		if (realOpts[0] == "o") {
			outFileName = realOpts[1];
		}
		else if (realOpts[0] == "i") {
			completionStringsFileName = realOpts[1];
		}
		else if(realOpts[0] == "cc") {
			coldCache = sserialize::toBool(realOpts[1]);
		}
		else if (realOpts[0] == "t") {
			if (realOpts[1] == "geocell") {
				ct = CT_GEOCELL;
			}
			else if (realOpts[1] == "treedgeocell") {
				ct = CT_GEOCELL_TREED;
			}
			else if (realOpts[1] == "items") {
				ct = CT_ITEMS;
			}
			else if (realOpts[1] == "decelled") {
				ct = CT_GEOCELL_DECELLED;
			}
			else {
				throw sserialize::ConfigurationException("Benchmarker", "invalid completer type: " + realOpts[1]);
			}
		}
		else if (realOpts[0] == "tc") {
			threadCount = std::stoi(realOpts[1]);
			if (!threadCount) {
				threadCount = std::thread::hardware_concurrency();
			}
		}
		else if (realOpts[0] == "ghsg") {
			if (realOpts[1] == "mem" || realOpts[1] == "memory" || realOpts[1] == "cached") {
				ghsgt = sserialize::spatial::GeoHierarchySubGraph::T_IN_MEMORY;
			}
			else if (realOpts[1] == "pass-through" || realOpts[1] == "pass") {
				ghsgt = sserialize::spatial::GeoHierarchySubGraph::T_PASS_THROUGH;
			}
			else {
				throw std::runtime_error("Benchmarker::Config: invalid GeoHierarchySubGraph type: " + realOpts[1]);
				return;
			}
		}
		else if (realOpts[0] == "subset") {
			computeSubSet = sserialize::toBool(realOpts[1]);
		}
		else if (realOpts[0] == "items") {
			computeItems = sserialize::toBool(realOpts[1]);
		}
		else {
			throw std::runtime_error("oscarcmd::Benchmarker::Config: unknown option: " + splitString);
			return;
		}
	}
	if (ct == CT_GEOCELL_DECELLED) {
		computeSubSet = false;
		computeItems = true;
	}
	if (completionStringsFileName.empty()) {
		throw sserialize::ConfigurationException("Benchmarker", "No input file name set");
	}
	if (outFileName.empty()) {
		throw sserialize::ConfigurationException("Benchmarker", "No output file name set");
	}
}

void Benchmarker::doGeocellBench() {
	if (!m_completer.textSearch().hasSearch(liboscar::TextSearch::Type::GEOCELL)) {
		throw std::runtime_error("No support fo clustered completion available");
		return;
	}
	std::ofstream rawOutFile;
	rawOutFile.open(config.outFileName + ".raw");
	if (!rawOutFile.is_open()) {
		throw std::runtime_error("Could not open file " + config.outFileName + ".raw");
		return;
	}
	
	std::ofstream statsOutFile;
	statsOutFile.open(config.outFileName + ".stats");
	if (!statsOutFile.is_open()) {
		throw std::runtime_error("Could not open file " + config.outFileName + ".stats");
		return;
	}

	int fd = -1;
	if (config.coldCache) {
		fd = ::open("/proc/sys/vm/drop_caches", O_WRONLY);
		if (fd < 0) {
			throw std::runtime_error("Could not open proc to drop caches");
		}
	}
	
	sserialize::Static::CellTextCompleter cmp;
	if (m_completer.textSearch().hasSearch(liboscar::TextSearch::Type::GEOCELL)) {
		cmp = m_completer.textSearch().get<liboscar::TextSearch::Type::GEOCELL>();
	}
	else if (m_completer.textSearch().hasSearch(liboscar::TextSearch::Type::OOMGEOCELL)) {
		cmp = m_completer.textSearch().get<liboscar::TextSearch::Type::OOMGEOCELL>();
	}
	
	sserialize::Static::ItemIndexStore indexStore = m_completer.indexStore();
	sserialize::Static::spatial::GeoHierarchy gh = m_completer.store().geoHierarchy();
	sserialize::Static::CQRDilator cqrd(m_completer.store().cellCenterOfMass(), m_completer.store().cellGraph());
	liboscar::CQRFromPolygon cqrfp(m_completer.store(), indexStore);
	sserialize::spatial::GeoHierarchySubGraph ghs(gh, indexStore, config.ghsgt);
	liboscar::CQRFromComplexSpatialQuery csq(ghs, cqrfp);
	liboscar::AdvancedCellOpTree opTree(cmp, cqrd, csq, ghs);
	oscar_cmd::DecelledACOT dOpTree(opTree);
	
	std::vector<Stats> stats;
	sserialize::CellQueryResult cqr;
	
	sserialize::ProgressInfo pinfo;
	pinfo.begin(m_strs.size(), "Processing");
	auto totalStart = std::chrono::high_resolution_clock::now();
	for(std::size_t i(0), s(m_strs.size()); i < s; ++i) {
		const std::string & str  = m_strs[i];
		Stats stat;
		if (config.coldCache) {
			::sync();
			::sleep(5);
			if (2 != ::write(fd, "3\n", 2)) {
				throw std::runtime_error("Benchmarker: could not drop caches");
			}
			::sync();
			::sleep(5);
		}
		opTree.parse(str);

		auto start = std::chrono::high_resolution_clock::now();

		if (config.ct == Config::CT_GEOCELL_TREED) {
			cqr = opTree.calc<sserialize::TreedCellQueryResult>(config.threadCount).toCQR(config.threadCount);
		}
		else if (config.ct == Config::CT_GEOCELL) {
			cqr = opTree.calc<sserialize::CellQueryResult>(config.threadCount);
		}
		else if (config.ct == Config::CT_GEOCELL_DECELLED) {
			dOpTree.clear();
			dOpTree.prepare();
			dOpTree.flaten(config.threadCount);
		}
		auto stop = std::chrono::high_resolution_clock::now();
		stat.cqr = std::chrono::duration_cast<Stats::meas_res>(stop-start);
		
		start = std::chrono::high_resolution_clock::now();
		if (config.computeSubSet) {
			auto subSet = ghs.subSet(cqr, false, config.threadCount);
		}
		stop = std::chrono::high_resolution_clock::now();
		stat.subgraph = std::chrono::duration_cast<Stats::meas_res>(stop-start);
		
		if (config.computeItems) {
			if (cqr.flags() & sserialize::CellQueryResult::FF_CELL_LOCAL_ITEM_IDS) {
				start = std::chrono::high_resolution_clock::now();
				cqr = cqr.toGlobalItemIds(config.threadCount);
				stop = std::chrono::high_resolution_clock::now();
				
				stat.toGlobalIds = std::chrono::duration_cast<Stats::meas_res>(stop-start);	
			}
			
			sserialize::ItemIndex items;
			
			start = std::chrono::high_resolution_clock::now();
			switch (config.ct) {
			case Config::CT_GEOCELL:
			case Config::CT_GEOCELL_TREED:
				items = cqr.flaten(config.threadCount);
				break;
			case Config::CT_GEOCELL_DECELLED:
				items = dOpTree.execute();
				break;
			default:
				break;
			};
			stop = std::chrono::high_resolution_clock::now();
		
			stat.flaten = std::chrono::duration_cast<Stats::meas_res>(stop-start);	
			stat.itemCount = items.size();
		}
		
		stat.cellCount = cqr.cellCount();
		
		stats.push_back(stat);
		
		pinfo(i);
	}
	auto totalStop = std::chrono::high_resolution_clock::now();
	pinfo.end();
	
	if (!stats.size()) {
		return;
	}
	
	rawOutFile << "Query id; cqr time [" << Stats::meas_res_unit << "];"
				<< "subgraph time[" << Stats::meas_res_unit << "];"
				<< "toGlobalIds time[" << Stats::meas_res_unit << "];"
				<< "flaten time[" << Stats::meas_res_unit << "];"
				<< "cell count; item count\n";
	for(uint32_t i(0), s(stats.size()); i < s; ++i) {
		const Stats & stat = stats[i];
		rawOutFile << i << ';' <<
		stat.cqr.count() << ';'
		<< stat.subgraph.count() << ';'
		<< stat.toGlobalIds.count() << ';'
		<< stat.flaten.count() << ';'
		<< stat.cellCount << ';'
		<< stat.itemCount << '\n';
	}
	
	Stats min(stats.front()), max(stats.front()), mean(stats.front());
	std::vector<Stats::meas_res> cqr_median, subgraph_median, toGlobalIds_median, flaten_median;
	
	cqr_median.emplace_back(stats.front().cqr);
	subgraph_median.emplace_back(stats.front().subgraph);
	toGlobalIds_median.emplace_back(stats.front().toGlobalIds);
	flaten_median.emplace_back(stats.front().flaten);
	
	for(uint32_t i(1), s(stats.size()); i < s; ++i) {
		const Stats & stat = stats[i];
		min.cqr = std::min(stat.cqr, min.cqr);
		min.subgraph = std::min(stat.subgraph, min.subgraph);
		min.toGlobalIds = std::min(stat.toGlobalIds, min.toGlobalIds);
		min.flaten = std::min(stat.flaten, min.flaten);
		
		max.cqr = std::max(stat.cqr, max.cqr);
		max.subgraph = std::max(stat.subgraph, max.subgraph);
		max.toGlobalIds = std::max(stat.toGlobalIds, max.toGlobalIds);
		max.flaten = std::max(stat.flaten, max.flaten);
		
		mean.cqr += stat.cqr;
		mean.subgraph += stat.subgraph;
		mean.toGlobalIds += stat.toGlobalIds;
		mean.flaten += stat.flaten;
		
		cqr_median.emplace_back(stat.cqr);
		subgraph_median.emplace_back(stat.subgraph);
		toGlobalIds_median.emplace_back(stat.toGlobalIds);
		flaten_median.emplace_back(stat.flaten);
	}
	
	std::sort(cqr_median.begin(), cqr_median.end());
	std::sort(subgraph_median.begin(), subgraph_median.end());
	std::sort(toGlobalIds_median.begin(), toGlobalIds_median.end());
	std::sort(flaten_median.begin(), flaten_median.end());
	
	statsOutFile << "total [" << Stats::meas_res_unit << "]: " << std::chrono::duration_cast<Stats::meas_res>(totalStop-totalStart).count() << '\n';
	
	statsOutFile << "cqr::total[" << Stats::meas_res_unit << "]: " << std::accumulate(cqr_median.begin(), cqr_median.end(), Stats::meas_res(0)) .count() << '\n';
	statsOutFile << "cqr::min[" << Stats::meas_res_unit << "]: " << min.cqr.count() << '\n';
	statsOutFile << "cqr::max[" << Stats::meas_res_unit << "]: " << max.cqr.count() << '\n';
	statsOutFile << "cqr::mean[" << Stats::meas_res_unit << "]: " << mean.cqr.count()/stats.size() << '\n';
	statsOutFile << "cqr::median[" << Stats::meas_res_unit << "]: " << cqr_median.at(stats.size()/2).count() << '\n';
	
	statsOutFile << "subgraph::total[" << Stats::meas_res_unit << "]: " << std::accumulate(subgraph_median.begin(), subgraph_median.end(), Stats::meas_res(0)) .count() << '\n';
	statsOutFile << "subgraph::min[" << Stats::meas_res_unit << "]: " << min.subgraph.count() << '\n';
	statsOutFile << "subgraph::max[" << Stats::meas_res_unit << "]: " << max.subgraph.count() << '\n';
	statsOutFile << "subgraph::mean[" << Stats::meas_res_unit << "]: " << mean.subgraph.count()/stats.size() << '\n';
	statsOutFile << "subgraph::median[" << Stats::meas_res_unit << "]: " << subgraph_median.at(stats.size()/2).count() << '\n';

	statsOutFile << "toGlobalIds::total[" << Stats::meas_res_unit << "]: " << std::accumulate(toGlobalIds_median.begin(), toGlobalIds_median.end(), Stats::meas_res(0)) .count() << '\n';
	statsOutFile << "toGlobalIds::min[" << Stats::meas_res_unit << "]: " << min.toGlobalIds.count() << '\n';
	statsOutFile << "toGlobalIds::max[" << Stats::meas_res_unit << "]: " << max.toGlobalIds.count() << '\n';
	statsOutFile << "toGlobalIds::mean[" << Stats::meas_res_unit << "]: " << mean.toGlobalIds.count()/stats.size() << '\n';
	statsOutFile << "toGlobalIds::median[" << Stats::meas_res_unit << "]: " << toGlobalIds_median.at(stats.size()/2).count() << '\n';
	
	statsOutFile << "flaten::total[" << Stats::meas_res_unit << "]: " << std::accumulate(flaten_median.begin(), flaten_median.end(), Stats::meas_res(0)) .count() << '\n';
	statsOutFile << "flaten::min[" << Stats::meas_res_unit << "]: " << min.flaten.count() << '\n';
	statsOutFile << "flaten::max[" << Stats::meas_res_unit << "]: " << max.flaten.count() << '\n';
	statsOutFile << "flaten::mean[" << Stats::meas_res_unit << "]: " << mean.flaten.count()/stats.size() << '\n';
	statsOutFile << "flaten::median[" << Stats::meas_res_unit << "]: " << flaten_median.at(stats.size()/2).count() << '\n';

}

Benchmarker::Benchmarker(liboscar::Static::OsmCompleter & completer, const Config & config) :
m_completer(completer),
config(config)
{
	{
		sserialize::MmappedMemory<char> mm(config.completionStringsFileName);
		const char * dBegin = mm.data();
		const char * dEnd = dBegin+mm.size();
		sserialize::split<const char *, sserialize::OneValueSet<uint32_t>, sserialize::OneValueSet<uint32_t>, std::back_insert_iterator<std::vector<std::string> > >(
			dBegin, dEnd, sserialize::OneValueSet<uint32_t>('\n'), sserialize::OneValueSet<uint32_t>(0xFFFFFFFF), std::back_insert_iterator<std::vector<std::string> >(m_strs)
			);
	}

}

void Benchmarker::execute() {
	switch (config.ct) {
	case Config::CT_GEOCELL:
	case Config::CT_GEOCELL_TREED:
	case Config::CT_GEOCELL_DECELLED:
		doGeocellBench();
		break;
	default:
		break;
	}
}

}//end namespace
