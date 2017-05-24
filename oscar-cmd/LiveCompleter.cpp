#include <type_traits>
#include "LiveCompleter.h"
#include <sserialize/stats/TimeMeasuerer.h>
#include <sserialize/spatial/CellQueryResult.h>
#include <sserialize/spatial/TreedCQR.h>
#include <liboscar/AdvancedCellOpTree.h>
#include <liboscar/CQRFromComplexSpatialQuery.h>

using namespace std;
using namespace liboscar;

namespace oscarcmd {

void LiveCompletion::CompletionStats::reset() {
	parseTime.reset();
	calcTime.reset();
	tree2TCQRTime.reset();
	tcqr2CqrTime.reset();
	cqrRemoveEmptyTime.reset();
	idxTime.reset();
	subGraphTime.reset();
	analyzeTime.reset();
	query.clear();
}

template<typename TItemSet>
void printResults(TItemSet & compSet, int num) {
	for(uint32_t i(0), s(num < 0 ? compSet.size() : std::min<uint32_t>(compSet.size(), num)); i < s; ++i) {
		compSet.at(i).print(std::cout, false);
		std::cout << "\n";
	}
}

LiveCompletion::LiveCompletion(liboscar::Static::OsmCompleter & completer) : m_completer(completer) {}

LiveCompletion::~LiveCompletion() {}

struct DummyDataAnalyser {
	void operator()(liboscar::Static::OsmItemSet &, LiveCompletion::CompletionStats &) const {}
	void operator()(liboscar::Static::OsmItemSetIterator &, LiveCompletion::CompletionStats &) const {}
};

struct CompSetUpdater {
	void operator()(liboscar::Static::OsmItemSet & compSet, LiveCompletion::CompletionStats & cs) {
		cs.idxTime.begin();
		compSet.update(cs.query);
		cs.idxTime.end();
	}
	void operator()(liboscar::Static::OsmItemSetIterator & compSet, LiveCompletion::CompletionStats & cs) {
		cs.idxTime.begin();
		compSet.update(cs.query);
		cs.idxTime.end();
	}
};

template<typename TItemSetType>
void printCompletionInfo(const LiveCompletion::CompletionStats & cs, TItemSetType & compSet, int printNResults) {
	printResults(compSet, printNResults);
	cout << "\nCompletion of (" << cs.query << ")" << " parsed as ";
	compSet.printTreeStructure(cout);
	cout << " took: \n";
	cout << "\tParse: " << cs.parseTime.elapsedMilliSeconds() << " [ms]\n";
	cout << "\tCalc: " << cs.calcTime.elapsedMilliSeconds() << " [ms]\n";
	if (cs.tree2TCQRTime.elapsedTime()) {
		cout << "\tTree2TCQR: " << cs.tree2TCQRTime.elapsedMilliSeconds() << " [ms]\n";
	}
	if (cs.tcqr2CqrTime.elapsedTime()) {
		cout << "\tTCQR2CQR: " << cs.tcqr2CqrTime.elapsedMilliSeconds() << " [ms]\n";
	}
	if (cs.cqrRemoveEmptyTime.elapsedTime()) {
		cout << "\tCQRRemoveEmpty: " << cs.cqrRemoveEmptyTime.elapsedMilliSeconds() << " [ms]\n";
	}
	cout << "\tSubGraph: " << cs.subGraphTime.elapsedMilliSeconds() << " [ms]\n";
	cout << "\tIndex: " << cs.idxTime.elapsedMilliSeconds() << " [ms]\n";
	cout << "\tAnalysis: " << cs.analyzeTime.elapsedMilliSeconds() << "[ms]\n";
	cout << "Found " << compSet.size() << " items" << std::endl;
}

template<typename T_INITIAL_COMPLETION_FUNC, typename T_UPDATE_FUNC = CompSetUpdater, typename T_DATA_ANALYSER = DummyDataAnalyser>
void completionBase(const vector<std::string> & completionStrings,
					T_INITIAL_COMPLETION_FUNC ic, int printNumResults, T_UPDATE_FUNC uf = T_UPDATE_FUNC(), T_DATA_ANALYSER * da = static_cast<T_DATA_ANALYSER*>(0)) {
	if (completionStrings.empty()) {
		return;
	}
	typedef typename std::result_of< T_INITIAL_COMPLETION_FUNC(LiveCompletion::CompletionStats & cs)>::type ItemSetType;
	LiveCompletion::CompletionStats cs;
	
	std::vector<std::string>::const_iterator completionString = completionStrings.cbegin();
	cs.query = *completionString;
	ItemSetType compSet = ic(cs);
	
	if (da) {
		(*da)(compSet, cs);
	}
	printCompletionInfo(cs, compSet, printNumResults);
	
	++completionString;
	for(std::vector<std::string>::const_iterator end(completionStrings.end()); completionString != end; ++completionString) {
		cs.reset();
		cs.query = *completionString;
		
		uf(compSet, cs);
		if (da) {
			(*da)(compSet, cs);
		}
		printCompletionInfo(cs, compSet, printNumResults);
	}
}

void LiveCompletion::symDiffItemsCompleters(const std::string & str, uint32_t c1, uint32_t c2, int printNumResults) {
	uint32_t csc = m_completer.textSearch().selectedTextSearcher(liboscar::TextSearch::ITEMS);
	LiveCompletion::CompletionStats cs;
	cs.query = str;
	cs.idxTime.begin();
	m_completer.setTextSearcher(liboscar::TextSearch::ITEMS, c1);
	sserialize::ItemIndex r1 = m_completer.complete(str).getIndex();
	m_completer.setTextSearcher(liboscar::TextSearch::ITEMS, c2);
	sserialize::ItemIndex r2 = m_completer.complete(str).getIndex();
	m_completer.setTextSearcher(liboscar::TextSearch::ITEMS, csc);
	sserialize::ItemIndex rf = r1 xor r2;
	cs.idxTime.end();
	liboscar::Static::OsmItemSet itemSet(m_completer.store(), rf);
	printCompletionInfo(cs, itemSet, printNumResults);
}

void LiveCompletion::doFullCompletion(const vector<std::string> & completionStrings, int printNumResults) {
	liboscar::Static::OsmCompleter * c = &m_completer;
	auto fn = [&c](CompletionStats & cs) {
		cs.idxTime.begin();
		auto ret = c->complete(cs.query);
		cs.idxTime.end();
		return ret;
	};
	completionBase(completionStrings, fn, printNumResults, CompSetUpdater());
}

void LiveCompletion::doSimpleCompletion(const std::vector<std::string> & completionStrings, int count, int minStrLen, int printNumResults) {
	liboscar::Static::OsmCompleter * c = &m_completer;
	auto fn = [&c, count, minStrLen](LiveCompletion::CompletionStats & cs) {
		cs.idxTime.begin();
		auto ret = c->simpleComplete(cs.query, count, minStrLen);
		cs.idxTime.end();
		return ret;
	};
	completionBase(completionStrings, fn, printNumResults);
}

void LiveCompletion::doPartialCompletion(const std::vector<std::string> & completionStrings, int count, int printNumResults) {
	liboscar::Static::OsmCompleter * c = &m_completer;
	auto dataAnalyser = [count](liboscar::Static::OsmItemSetIterator & x, LiveCompletion::CompletionStats & cs) -> void {
		cs.analyzeTime.begin();
		x.seek(count);
		cs.analyzeTime.end();
	};
	auto fn = [&c, count](LiveCompletion::CompletionStats & cs) -> liboscar::Static::OsmItemSetIterator {
		cs.idxTime.begin();
		auto ret = c->partialComplete(cs.query);
		cs.idxTime.end();
		return ret;
	};
	completionBase(completionStrings, fn, printNumResults, CompSetUpdater(), &dataAnalyser);
}

void LiveCompletion::doClusteredComplete(const std::vector<std::string> & completionStrings, int printNumResults, bool treedCQR, uint32_t threadCount) {
	liboscar::Static::OsmCompleter * c = &m_completer;
	auto cf = [&c, treedCQR, threadCount](LiveCompletion::CompletionStats & cs) -> liboscar::Static::OsmItemSet {
		sserialize::ItemIndex retIdx;
	
		if (!c->textSearch().hasSearch(liboscar::TextSearch::Type::GEOCELL)) {
			throw sserialize::UnsupportedFeatureException("OsmCompleter::cqrComplete data has no CellTextCompleter");
		}
		sserialize::Static::CellTextCompleter cmp( c->textSearch().get<liboscar::TextSearch::Type::GEOCELL>() );
		sserialize::Static::CQRDilator cqrd(c->store().cellCenterOfMass(), c->store().cellGraph());
		CQRFromPolygon cqrfp(c->store(), c->indexStore());
		CQRFromComplexSpatialQuery csq(c->ghsg(), cqrfp);

		sserialize::CellQueryResult cqr;
		if (!treedCQR) {
			cs.parseTime.begin();
			AdvancedCellOpTree opTree(cmp, cqrd, csq, c->ghsg());
			opTree.parse(cs.query);
			cs.parseTime.end();
			cs.calcTime.begin();
			cqr = opTree.calc<sserialize::CellQueryResult>();
			cs.calcTime.end();
		}
		else {
			cs.parseTime.begin();
			AdvancedCellOpTree opTree(cmp, cqrd, csq, c->ghsg());
			opTree.parse(cs.query);
			cs.parseTime.end();
			
			cs.calcTime.begin();
			
			cs.tree2TCQRTime.begin();
			auto tcqr = opTree.calc<sserialize::TreedCellQueryResult>(threadCount);
			cs.tree2TCQRTime.end();
			
			cs.tcqr2CqrTime.begin();
			cqr = tcqr.toCQR(threadCount, true);
			cs.tcqr2CqrTime.end();
			
			cs.cqrRemoveEmptyTime.begin();
			cqr = cqr.removeEmpty();
			cs.cqrRemoveEmptyTime.end();
			
			cs.calcTime.end();
		}
		
		cs.subGraphTime.begin();
		c->ghsg().subSet(cqr, false);
		cs.subGraphTime.end();
		
		cs.idxTime.begin();
		retIdx = cqr.flaten();
		cs.idxTime.end();

		return liboscar::Static::OsmItemSet(c->store(), retIdx);
	};
	auto uf = [&cf](liboscar::Static::OsmItemSet & s, LiveCompletion::CompletionStats & cs) -> void {
		s = cf(cs);
	};
	completionBase(completionStrings, cf, printNumResults, uf);
}

void LiveCompletion::doFullCompletion(bool printStats) {
	sserialize::TimeMeasurer completionTime, dataAnalyseTime;
	
	//completed set stats
	uint32_t cset_positionCount;
	uint32_t cset_geoElementCount;

	char strBuffer[256];
	string compStr("");
	Static::OsmItemSet compSet = m_completer.complete(compStr);
	while (true) {
		cset_positionCount = 0;
		cset_geoElementCount = 0;
		cout << "Please enter the completion query (Return=quit)" << endl;
		cin.getline(strBuffer, 256);
		compStr = std::string(strBuffer);
		
		if (compStr.size() > 0) {
			
			completionTime.begin();
			compSet.update(compStr);
			completionTime.end();
			
			dataAnalyseTime.begin();
			for(uint32_t i = 0; i < compSet.size(); i++) {
				Static::OsmKeyValueObjectStore::Item item = compSet.at(i);
				cset_geoElementCount++;;
				for(uint32_t i = 0; i < item.geoPointCount(); i++) {
					sserialize::Static::spatial::GeoPoint geop =  item.geoPointAt(i);
					cset_positionCount++;
				}
			}
			dataAnalyseTime.end();

			cout << "Do you want to print the results?";
			cin.getline(strBuffer, 256);
			string doPrint(strBuffer);
			if (doPrint == "y") {
				std::cout << compSet;
			}
			
			if (printStats) {
				//stats
				cout << "Do you want to write the index to disk?";
				cin.getline(strBuffer, 256);
				string doWrite(strBuffer);
				if (doWrite == "y") {
					compSet.getIndex().dump(std::string("indexout-" + compStr).c_str());
				}
			}
			

			cout << endl << "Completion of (";
			cout << compStr << ")";
			cout << " parsed as "; compSet.printTreeStructure(cout);
			cout << " took " << (double)completionTime.elapsedTime()/1000 << " mseconds" << endl;
			cout << "Analysing " << compSet.size() << " items with " << cset_geoElementCount << " geo-elements and " << cset_positionCount << " geopoints took" << std::endl;
			std::cout << (double)dataAnalyseTime.elapsedTime() << " mseconds" << std::endl;
		}
		else {
			break;
		}
		
	}
}
	

	
void LiveCompletion::doSimpleCompletion(uint32_t count, uint32_t minStrLen, bool printStats) {
	sserialize::TimeMeasurer completionTime, dataAnalyseTime;
	
	//completed set stats
	uint32_t cset_positionCount;
	uint32_t cset_geoElementCount;

	char strBuffer[256];
	string compStr("");
	Static::OsmItemSet compSet = m_completer.simpleComplete(compStr, count, minStrLen);
	while (true) {
		cset_positionCount = 0;
		cset_geoElementCount = 0;
		cout << "Please enter the completion query (Return=quit)" << endl;
		cin.getline(strBuffer, 256);
		compStr = std::string(strBuffer);
		
		if (compStr.size() > 0) {
			
			completionTime.begin();
			compSet.update(compStr);
			completionTime.end();
			
			dataAnalyseTime.begin();
			for(uint32_t i = 0; i < compSet.size(); i++) {
				Static::OsmKeyValueObjectStore::Item item = compSet.at(i);
				cset_geoElementCount++;;
				for(uint32_t i = 0; i < item.geoPointCount(); i++) {
					sserialize::Static::spatial::GeoPoint geop =  item.geoPointAt(i);
					cset_positionCount++;
				}
			}
			dataAnalyseTime.end();

			cout << "Do you want to print the results?";
			cin.getline(strBuffer, 256);
			string doPrint(strBuffer);
			if (doPrint == "y") {
				std::cout << compSet;
			}
			
			if (printStats) {
				//stats
				cout << "Do you want to write the index to disk?";
				cin.getline(strBuffer, 256);
				string doWrite(strBuffer);
				if (doWrite == "y") {
					std::ofstream of;
					of.open(std::string("indexout-" + compStr));
					if (of.is_open()) {
						compSet.getIndex().dump(of);
					}
					else {
						std::cout << "Failed to open index" << std::endl;
					}
				}
			}
			

			cout << endl << "Completion of (";
			cout << compStr << ")";
			cout << " parsed as "; compSet.printTreeStructure(cout);
			cout << " took " << (double)completionTime.elapsedTime()/1000 << " mseconds" << endl;
			cout << "Analysing " << compSet.size() << " items with " << cset_geoElementCount << " geo-elements and " << cset_positionCount << " geopoints took" << std::endl;
			std::cout << (double)dataAnalyseTime.elapsedTime() << " mseconds" << std::endl;
		}
		else {
			break;
		}
		
	}
}
	
void LiveCompletion::doPartialCompletion(int count) {
	sserialize::TimeMeasurer completionTime, seekTime, dataAnalyseTime;
	
	//completed set stats
	uint32_t cset_positionCount;
	uint32_t cset_geoElementCount;

	char strBuffer[256];
	string compStr("");
	Static::OsmItemSetIterator compSet = m_completer.partialComplete(compStr, sserialize::spatial::GeoRect());
	while (true) {
		cset_positionCount = 0;
		cset_geoElementCount = 0;
		cout << "Please enter the completion query (Return=quit)" << endl;
		cin.getline(strBuffer, 256);
		compStr = std::string(strBuffer);
		
		if (compStr.size() > 0) {
			
			completionTime.begin();
			compSet.update(compStr);
			completionTime.end();
			
			seekTime.begin();
			compSet.at(count);
			seekTime.end();
			
			dataAnalyseTime.begin();
			count = std::max<int>(count-1,0);
			for(int i = 0; i < count && compSet.valid(); i++) {
				Static::OsmKeyValueObjectStore::Item item = compSet.at(i);
				cset_geoElementCount++;;
				for(uint32_t i = 0; i < item.geoPointCount(); i++) {
					sserialize::Static::spatial::GeoPoint geop =  item.geoPointAt(i);
					cset_positionCount++;
				}
			}
			dataAnalyseTime.end();

			cout << endl << "Completion of (";
			cout << compStr << ")";
			cout << " parsed as "; compSet.printTreeStructure(cout);
			cout << " took " << (double)completionTime.elapsedTime()/1000 << " mseconds" << endl;
			cout << "ItemSetIterator.maxSize() = " << compSet.maxSize() << std::endl;
			cout << "Seeking to " << compSet.cacheSize() << " items took";
			std::cout << (double)seekTime.elapsedTime()/1000 << " mseconds" << endl;

			cout << "Analysing items with " << cset_geoElementCount << " geo-elements and " << cset_positionCount << " geopoints took ";
			std::cout << (double)dataAnalyseTime.elapsedTime()/1000 << "mseconds" << std::endl;
			cout << "Do you want to print the results?";
			cin.getline(strBuffer, 256);
			string doPrint(strBuffer);
			if (doPrint == "y") {
				compSet.seekEnd();
				std::cout << compSet;
			}
		}
		else {
			break;
		}
		
	}
}

}//end namespace
