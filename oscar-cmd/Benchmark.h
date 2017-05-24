#ifndef OSCAR_CMD_BENCHMARK_H
#define OSCAR_CMD_BENCHMARK_H
#include <liboscar/StaticOsmCompleter.h>
#include <sserialize/stats/statfuncs.h>
#include <sserialize/stats/TimeMeasuerer.h>


namespace oscarcmd {

struct BenchmarkStats {
	BenchmarkStats() {}
	///resize tm to count
	BenchmarkStats(std::size_t count) {
		tm.resize(count);
		singleTm.resize(count);
		updateCount.resize(count, 0);
		finalCompletionString.resize(count);
	}
	std::vector<std::string> finalCompletionString;
	std::vector< sserialize::TimeMeasurer > tm;
	std::vector< std::vector<sserialize::TimeMeasurer> > singleTm;
	std::vector<uint32_t> updateCount;

	template<typename TValue>
	void printStatsFromVector(std::ostream & out, std::vector<TValue> src, bool withExtra = true) {
		std::size_t minPos = std::min_element(src.begin(), src.end()) - src.begin();
		std::size_t maxPos = std::max_element(src.begin(), src.end()) - src.begin();
	
		out << "Min: " << src[minPos] << std::endl;
		if (withExtra) {
			out << "Min-Query: " << finalCompletionString[minPos] << std::endl;
			out << "Min-UpdateCount: " << updateCount[minPos] << std::endl;
		}
		out << "Max: " << src[maxPos] << std::endl;
		if (withExtra) {
			out << "Max-Query: " << finalCompletionString[maxPos] << std::endl;
			out << "Max-UpdateCount: " << updateCount[maxPos] << std::endl;
		}
		out << "Mean: " << sserialize::statistics::mean(src.begin(), src.end(), 0.0) << std::endl;
		out << "StdDev: " << sserialize::statistics::stddev(src.begin(), src.end(), 0.0) << std::endl;
		std::sort(src.begin(), src.end());
		out << "Median: " << src.at(src.size()/2) << std::endl;
	}
	
	void printLatexStats(std::ostream & out);
	void printStats(std::ostream & out);
	void printRawStats(std::ostream & out);
};

/**
  * A simple benchmark class
  * Config accepts the following config string:
  * i=inputFileName,o=outputFileName,t=(tgeocell|geocell|items),cc=(true|false)
  */
class Benchmarker final {
public:
	struct Config {
		typedef enum {CT_INVALID, CT_ITEMS, CT_GEOCELL, CT_GEOCELL_TREED} CompleterType;
		CompleterType ct;
		bool coldCache;
		std::string completionStringsFileName;
		std::string outFileName;
		Config() : ct(CT_INVALID) {}
		Config(const std::string str);
	};
private:
	liboscar::Static::OsmCompleter & m_completer;
private:
	void doGeocellBench(const std::vector< std::string >& strs, bool coldCache, bool treedCQR, std::ostream& out);
public:
	Benchmarker(liboscar::Static::OsmCompleter & completer) : m_completer(completer) {}
	~Benchmarker() {}
	void benchmark(const Config & config);
};


}

#endif