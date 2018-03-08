#ifndef OSCAR_CMD_BENCHMARK_H
#define OSCAR_CMD_BENCHMARK_H
#include <liboscar/StaticOsmCompleter.h>
#include <sserialize/stats/statfuncs.h>
#include <sserialize/stats/TimeMeasuerer.h>
#include <chrono>

namespace oscarcmd {

/**
  * A simple benchmark class
  * Config accepts the following config string:
  * i=inputFileName,o=outputFileName,t=(tgeocell|geocell|items),cc=(true|false),tc=<num>,ghsg=(mem|pass)
  */
class Benchmarker final {
public:
	struct Config {
		typedef enum {CT_INVALID, CT_ITEMS, CT_GEOCELL, CT_GEOCELL_TREED} CompleterType;
		CompleterType ct;
		bool coldCache;
		std::string completionStringsFileName;
		std::string outFileName;
		uint32_t threadCount;
		sserialize::spatial::GeoHierarchySubGraph::Type ghsgt;
		Config() : ct(CT_INVALID), threadCount(1), ghsgt(sserialize::spatial::GeoHierarchySubGraph::T_PASS_THROUGH) {}
		Config(const std::string & str);
	};
	
	struct Stats {
		using meas_res = std::chrono::microseconds;
		meas_res cqr;
		meas_res subgraph;
		meas_res flaten;
		uint32_t cellCount;
		uint32_t itemCount;
	};
	
private:
	liboscar::Static::OsmCompleter & m_completer;
	Config config;
	std::vector<std::string> m_strs;
private:
	void doGeocellBench();
public:
	Benchmarker(liboscar::Static::OsmCompleter & completer, const Config & config);
	~Benchmarker() {}
	void execute();
};


}

#endif
