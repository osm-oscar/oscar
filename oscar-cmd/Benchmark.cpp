#include "Benchmark.h"
#include <sserialize/strings/stringfunctions.h>
#include <sserialize/algorithm/utilfuncs.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/stats/statfuncs.h>
#include <sserialize/stats/ProgressInfo.h>
#include <liboscar/CellOpTree.h>
#include <limits>
#include <algorithm>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>

namespace oscarcmd {

	
void BenchmarkStats::printLatexStats(std::ostream & out) {
	std::vector<long int> elapsedTimes; elapsedTimes.reserve(tm.size());
	std::vector<long int> elapsedMeanTimesPerCompletion; elapsedMeanTimesPerCompletion.reserve(tm.size());
	for(std::size_t i = 0; i < tm.size(); ++i) {
		elapsedTimes.push_back( tm[i].elapsedTime() );
		elapsedMeanTimesPerCompletion.push_back( elapsedTimes.back() / updateCount[i] );
	}


	std::size_t minPosTotal = std::min_element(elapsedTimes.begin(), elapsedTimes.end()) - elapsedTimes.begin();
	std::size_t maxPosTotal = std::max_element(elapsedTimes.begin(), elapsedTimes.end()) - elapsedTimes.begin();
// 	std::size_t minPosSingle = std::min_element(elapsedMeanTimesPerCompletion.begin(), elapsedMeanTimesPerCompletion.end()) - elapsedMeanTimesPerCompletion.begin();
// 	std::size_t maxPosSingle = std::max_element(elapsedMeanTimesPerCompletion.begin(), elapsedMeanTimesPerCompletion.end()) - elapsedMeanTimesPerCompletion.begin();

	out << "\\multicolumn{2}{c}{Allgemeine Informationen}\\\\\\hline" << std::endl;
	out << "Anzahl voller Simulationen\t&" << elapsedTimes.size() << "\\\\" << std::endl;
	out << "Kummulierte Anzahl an Komplettierungen&" << std::accumulate(updateCount.begin(), updateCount.end(), 0) << "\\\\" << std::endl;

	out << "\\multicolumn{2}{c}{Komplettierungszeiten für eine volle Simulation}\\\\\\hline" << std::endl;
	out << "Minimum\t\t&" << elapsedTimes[minPosTotal] << "\\\\" << std::endl; 
	out << "Maximum\t\t&" << elapsedTimes[maxPosTotal] << "\\\\" << std::endl;
	out << "Mittelwert\t\t&" << sserialize::statistics::mean(elapsedTimes.begin(), elapsedTimes.end(), 0.0) << std::endl;
	out << "Median\t\t&" << sserialize::statistics::median(elapsedTimes.begin(), elapsedTimes.end(), 0.0) << std::endl;
	out << "Standardabweichung\t&" << sserialize::statistics::stddev(elapsedTimes.begin(), elapsedTimes.end(), 0.0) << std::endl;

	out << "\\multicolumn{2}{c}{Komplettierungszeiten für eine einzelne Anfrage}\\\\\\hline" << std::endl;
	out << "Minimum\t\t&" << elapsedMeanTimesPerCompletion[minPosTotal] << "\\\\" << std::endl; 
	out << "Maximum\t\t&" << elapsedMeanTimesPerCompletion[maxPosTotal] << "\\\\" << std::endl;
	out << "Mittelwert\t\t&" << sserialize::statistics::mean(elapsedMeanTimesPerCompletion.begin(), elapsedMeanTimesPerCompletion.end(), 0.0) << std::endl;
	out << "Median\t\t&" << sserialize::statistics::median(elapsedMeanTimesPerCompletion.begin(), elapsedMeanTimesPerCompletion.end(), 0.0) << std::endl;
	out << "Standardabweichung\t&" << sserialize::statistics::stddev(elapsedMeanTimesPerCompletion.begin(), elapsedMeanTimesPerCompletion.end(), 0.0) << std::endl;

}

void  BenchmarkStats::printStats(std::ostream & out) {
	std::vector<long int> elapsedTimes; elapsedTimes.reserve(tm.size());
	std::vector<long int> elapsedMeanTimesPerCompletion; elapsedMeanTimesPerCompletion.reserve(tm.size());
	std::vector< std::vector<long int> > singleElapsedTimes; singleElapsedTimes.resize(singleTm.size());
	
	std::vector<long int> minSingleTimes; minSingleTimes.reserve(tm.size());
	std::vector<long int> maxSingleTimes; maxSingleTimes.reserve(tm.size());
	std::vector<long int> allSingleTimes;
	for(std::size_t i = 0; i < tm.size(); ++i) {
		elapsedTimes.push_back( tm[i].elapsedTime() );
		elapsedMeanTimesPerCompletion.push_back( elapsedTimes.back() / updateCount[i] );
	}
	for(std::size_t i = 0; i < singleTm.size(); ++i) {
		for(std::size_t j = 0; j < singleTm[i].size(); ++j) {
			singleElapsedTimes[i].push_back( singleTm[i][j].elapsedTime() );
			allSingleTimes.push_back(singleElapsedTimes[i].back());
		}
	}

	for(std::size_t i = 0; i < singleElapsedTimes.size(); ++i) {
		minSingleTimes.push_back( std::min_element(singleElapsedTimes[i].begin(), singleElapsedTimes[i].end()).operator*() );
		maxSingleTimes.push_back( std::max_element(singleElapsedTimes[i].begin(), singleElapsedTimes[i].end()).operator*() );
	}

	if (elapsedTimes.size()) {
		out << "Cumulated Timing stats" << std::endl;
		printStatsFromVector(out, elapsedTimes);
	}
	if (elapsedMeanTimesPerCompletion.size()) {
		out << "Timing stats per completion (mean)" << std::endl;
		printStatsFromVector(out, elapsedMeanTimesPerCompletion);
	}
	
	if (minSingleTimes.size()) {
		out << "Minimum Single Timing stats" << std::endl;
		printStatsFromVector(out, minSingleTimes);
	}
	if (maxSingleTimes.size()) {
		out << "Maximum Single Timing stats" << std::endl;
		printStatsFromVector(out, maxSingleTimes);
	}
	if (allSingleTimes.size()) {
		out << "All Single Timing stats" << std::endl;
		printStatsFromVector(out, maxSingleTimes, false);
	}
}

void BenchmarkStats::printRawStats(std::ostream & out) {
	std::vector< std::vector<long int> > singleElapsedTimes; singleElapsedTimes.resize(singleTm.size());
	for(std::size_t i = 0; i < singleTm.size(); ++i) {
		for(std::size_t j = 0; j < singleTm[i].size(); ++j) {
			singleElapsedTimes[i].push_back( singleTm[i][j].elapsedTime() );
		}
	}
	for(std::size_t i = 0; i < singleElapsedTimes.size(); ++i) {
		std::size_t j = 0;
		while (true) {
			out << singleElapsedTimes[i][j];
			++j;
			if (j < singleElapsedTimes[i].size())
				out << ";";
			else
				break;
		}
		out << std::endl;
	}
}

Benchmarker::Config::Config(const std::string str) :
ct(CT_INVALID),
coldCache(false)
{
	std::vector<std::string> splitted = sserialize::split< std::vector<std::string> >(str, ',', '\\');
	for(const std::string & splitString : splitted) {
		std::vector<std::string> realOpts = sserialize::split< std::vector<std::string> >(splitString, '=', '\\');
		if (realOpts.size() != 2)
			continue;
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
			else if (realOpts[1] == "items") {
				ct = CT_ITEMS;
			}
			else {
				throw std::runtime_error("Benchmarker::Config: invalid completer type: " + realOpts[1]);
				return;
			}
		}
		else {
			throw std::runtime_error("oscarcmd::Benchmarker::Config: unknown option: " + splitString);
			return;
		}
	}
}

void Benchmarker::doGeocellBench(const std::vector<std::string> & strs, bool coldCache, std::ostream & out) {
	if (!m_completer.textSearch().hasSearch(liboscar::TextSearch::Type::GEOCELL)) {
		throw std::runtime_error("No support fo clustered completion available");
		return;
	}
	//out << "#Query;Time for set operations;Time for subgraph creation;Number of cells;Number of items\n";
	sserialize::Static::CellTextCompleter cmp( m_completer.textSearch().get<liboscar::TextSearch::Type::GEOCELL>() );
	sserialize::TimeMeasurer tm;
	long int cqrTime, subsetTime;
	int fd = -1;
	if (coldCache) {
		fd = ::open("/proc/sys/vm/drop_caches", O_WRONLY);
		if (fd < 0) {
			throw std::runtime_error("Could not open proc for dropping caches");
		}
	}
	
	
	for(const std::string & str : strs) {
		liboscar::CellOpTree<sserialize::CellQueryResult> opTree(cmp, true);
		if (coldCache) {
			::sync();
			sleep(5);
			if (2 != ::write(fd, "3\n", 2)) {
				throw std::runtime_error("Benchmarker: could not drop caches");
			}
			::sync();
			sleep(5);
		}
		opTree.parse(str);
		tm.begin();
		sserialize::CellQueryResult r(opTree.calc());
		tm.end();
		cqrTime = tm.elapsedUseconds();
		tm.begin();
		sserialize::Static::spatial::GeoHierarchy::SubSet subSet = m_completer.store().geoHierarchy().subSet(r, false);
		tm.end();
		subsetTime = tm.elapsedUseconds();
		out << str << ";" << cqrTime << ";" << subsetTime << ";" << r.cellCount() << ";" << r.flaten().size() << "\n";
	}
}


void Benchmarker::benchmark(const Benchmarker::Config & config) {
	std::vector<std::string> strs;
	{
		sserialize::MmappedMemory<char> mm(config.completionStringsFileName);
		const char * dBegin = mm.data();
		const char * dEnd = dBegin+mm.size();
		sserialize::split<const char *, sserialize::OneValueSet<uint32_t>, sserialize::OneValueSet<uint32_t>, std::back_insert_iterator<std::vector<std::string> > >(
			dBegin, dEnd, sserialize::OneValueSet<uint32_t>('\n'), sserialize::OneValueSet<uint32_t>(0xFFFFFFFF), std::back_insert_iterator<std::vector<std::string> >(strs)
			);
	}
	std::ofstream outFile;
	outFile.open(config.outFileName);
	if (!outFile.is_open()) {
		throw std::runtime_error("Could not open file " + config.outFileName);
		return;
	}
	
	if (config.ct == Config::CT_GEOCELL) {
		doGeocellBench(strs, config.coldCache, outFile);
	}
}

}//end namespace