#ifndef OSCAR_CMD_LIVE_COMPLETER_H
#define OSCAR_CMD_LIVE_COMPLETER_H
#include <liboscar/StaticOsmCompleter.h>

namespace oscarcmd {

class LiveCompletion {
public:
	struct CompletionStats {
		sserialize::TimeMeasurer parseTime;
		sserialize::TimeMeasurer calcTime;
		sserialize::TimeMeasurer tree2TCQRTime;
		sserialize::TimeMeasurer tcqr2CqrTime;
		sserialize::TimeMeasurer cqrRemoveEmptyTime;
		sserialize::TimeMeasurer cqrLocal2GlobalIds;
		sserialize::TimeMeasurer idxTime;
		sserialize::TimeMeasurer subGraphTime;
		sserialize::TimeMeasurer analyzeTime;
		std::string query;
		void reset();
	};
public:
	LiveCompletion(liboscar::Static::OsmCompleter & completer);
	~LiveCompletion();

	void symDiffItemsCompleters(const std::string & str, uint32_t c1, uint32_t c2, int printNumResults);

	void doFullCompletion(const std::vector<std::string> & completionStrings, int printNumResults);
	void doSimpleCompletion(const std::vector< std::string > & completionStrings, int count, int minStrLen, int printNumResults);
	void doPartialCompletion(const std::vector<std::string> & completionStrings, int count, int printNumResults);
	void doClusteredComplete(const std::vector<std::string> & completionStrings, int printNumResults, bool treedCQR, uint32_t threadCount);
	void doDecelledComplete(const std::vector<std::string> & completionStrings, int printNumResults, uint32_t threadCount);

	void doFullCompletion(bool printStats);
	void doSimpleCompletion(uint32_t count, uint32_t minStrLen, bool printStats);
	void doPartialCompletion(int count);
private:
	liboscar::Static::OsmCompleter & m_completer;
};


}//end namespace

#endif
