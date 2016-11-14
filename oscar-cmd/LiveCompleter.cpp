#include <type_traits>
#include "LiveCompleter.h"
#include <sserialize/stats/TimeMeasuerer.h>

using namespace std;
using namespace liboscar;

namespace oscarcmd {

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
	void operator()(liboscar::Static::OsmItemSet &) const {}
	void operator()(liboscar::Static::OsmItemSetIterator &) const {}
};

struct CompSetUpdater {
	void operator()(liboscar::Static::OsmItemSet & compSet, const std::string & completionString) {
		compSet.update(completionString);
	}
	void operator()(liboscar::Static::OsmItemSetIterator & compSet, const std::string & completionString) {
		compSet.update(completionString);
	}
};

template<typename TItemSetType>
void printCompletionInfo(
const sserialize::TimeMeasurer & cT, const sserialize::TimeMeasurer & dT, 
const std::string & cs, TItemSetType & compSet, int printNResults) {
	printResults(compSet, printNResults);
	
	cout << "\nCompletion of (";
	cout << cs << ")";
	cout << " parsed as "; compSet.printTreeStructure(cout);
	cout << " took " << cT.elapsedMilliSeconds() << " mseconds" << endl;
	cout << "Data analysing took " << dT.elapsedMilliSeconds() << " mseconds" << endl;
	cout << "Found " << compSet.size() << " items" << std::endl;

}

template<typename T_INITIAL_COMPLETION_FUNC, typename T_UPDATE_FUNC = CompSetUpdater, typename T_DATA_ANALYSER = DummyDataAnalyser>
void completionBase(const vector<std::string> & completionStrings,
					T_INITIAL_COMPLETION_FUNC ic, int printNumResults, T_UPDATE_FUNC uf = T_UPDATE_FUNC(), T_DATA_ANALYSER * da = static_cast<T_DATA_ANALYSER*>(0)) {
	if (completionStrings.empty()) {
		return;
	}
	typedef typename std::result_of< T_INITIAL_COMPLETION_FUNC(const std::string&)>::type ItemSetType;
	sserialize::TimeMeasurer completionTime, dataAnalyseTime;
	
	std::vector<std::string>::const_iterator completionString = completionStrings.cbegin();
	completionTime.begin();
	ItemSetType compSet = ic(*completionString);
	completionTime.end();
	
	if (da) {
		dataAnalyseTime.begin();
		(*da)(compSet);
		dataAnalyseTime.end();
	}
	printCompletionInfo(completionTime, dataAnalyseTime, *completionString, compSet, printNumResults);
	
	++completionString;
	for(std::vector<std::string>::const_iterator end(completionStrings.end()); completionString != end; ++completionString) {
		completionTime.begin();
		uf(compSet, *completionString);
		completionTime.end();
		if (da) {
			dataAnalyseTime.begin();
			(*da)(compSet);
			dataAnalyseTime.end();
		}
		printCompletionInfo(completionTime, dataAnalyseTime, *completionString, compSet, printNumResults);
	}
}

void LiveCompletion::symDiffItemsCompleters(const std::string & str, uint32_t c1, uint32_t c2, int printNumResults) {
	uint32_t csc = m_completer.textSearch().selectedTextSearcher(liboscar::TextSearch::ITEMS);
	sserialize::TimeMeasurer tm;
	tm.begin();
	m_completer.setTextSearcher(liboscar::TextSearch::ITEMS, c1);
	sserialize::ItemIndex r1 = m_completer.complete(str).getIndex();
	m_completer.setTextSearcher(liboscar::TextSearch::ITEMS, c2);
	sserialize::ItemIndex r2 = m_completer.complete(str).getIndex();
	m_completer.setTextSearcher(liboscar::TextSearch::ITEMS, csc);
	sserialize::ItemIndex rf = r1 xor r2;
	tm.end();
	liboscar::Static::OsmItemSet itemSet(m_completer.store(), rf);
	printCompletionInfo(tm, tm, str, itemSet, printNumResults);
}

void LiveCompletion::doFullCompletion(const vector<std::string> & completionStrings, int printNumResults) {
	liboscar::Static::OsmCompleter * c = &m_completer;
	auto fn = [&c](const std::string & x){return c->complete(x);};
	completionBase(completionStrings, fn, printNumResults, CompSetUpdater());
}

void LiveCompletion::doSimpleCompletion(const std::vector<std::string> & completionStrings, int count, int minStrLen, int printNumResults) {
	liboscar::Static::OsmCompleter * c = &m_completer;
	auto fn = [&c, count, minStrLen](const std::string & x) {return c->simpleComplete(x, count, minStrLen);};
	completionBase(completionStrings, fn, printNumResults);
}

void LiveCompletion::doPartialCompletion(const std::vector<std::string> & completionStrings, int count, int printNumResults) {
	liboscar::Static::OsmCompleter * c = &m_completer;
	auto dataAnalyser = [count](liboscar::Static::OsmItemSetIterator & x) -> void { x.seek(count); };
	auto fn = [&c, count](const std::string & x) -> liboscar::Static::OsmItemSetIterator {return c->partialComplete(x);};
	completionBase(completionStrings, fn, printNumResults, CompSetUpdater(), &dataAnalyser);
}

void LiveCompletion::doClusteredComplete(const std::vector<std::string> & completionStrings, int printNumResults, bool treedCQR, uint32_t threadCount) {
	liboscar::Static::OsmCompleter * c = &m_completer;
	auto cf = [&c, treedCQR, threadCount](const std::string & x) -> liboscar::Static::OsmItemSet {
		sserialize::Static::spatial::GeoHierarchy::SubSet subSet = c->clusteredComplete(x, 0, treedCQR, threadCount);
		sserialize::ItemIndex retIdx( subSet.cqr().flaten() );
		return liboscar::Static::OsmItemSet(c->store(), retIdx);
	};
	auto uf = [&c, treedCQR, threadCount](liboscar::Static::OsmItemSet & s, const std::string & str) -> void {
		sserialize::Static::spatial::GeoHierarchy::SubSet subSet = c->clusteredComplete(str, 0, treedCQR, threadCount);
		sserialize::ItemIndex retIdx( subSet.cqr().flaten() );
		s = liboscar::Static::OsmItemSet(c->store(), retIdx);
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
