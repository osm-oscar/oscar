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
  * i=inputFileName,o=outputFileName,t=(tgeocell|geocell|items),cc=(true|false),tc=<num>,ghsg=(mem|pass),subset=(true|false),items(true|false)
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
		bool computeSubSet;
		bool computeItems;
		Config() :
		ct(CT_INVALID),
		coldCache(false),
		threadCount(1),
		ghsgt(sserialize::spatial::GeoHierarchySubGraph::T_PASS_THROUGH),
		computeSubSet(true),
		computeItems(true)
		{}
		Config(const std::string & str);
	};
	
	struct Stats {
		static const char * meas_res_unit;
		using meas_res = std::chrono::microseconds;
		meas_res cqr;
		meas_res subgraph;
		meas_res toGlobalIds;
		meas_res flaten;
		uint32_t cellCount = 0;
		uint32_t itemCount = 0;
		Stats() : cqr(0), subgraph(0), toGlobalIds(0), flaten(0) {}
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
