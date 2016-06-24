#ifndef OSCAR_CMD_WORK_FUNCTIONS_H
#define OSCAR_CMD_WORK_FUNCTIONS_H
#include <ostream>
#include "Config.h"


namespace liboscar {
namespace Static {
	class OsmCompleter;

}}//end liboscar namespace

namespace oscarcmd {

struct Worker {

	liboscar::Static::OsmCompleter completer;
	
	void listCompleters();
	
	void printPaperStatsDb(const WD_PrintPaperStatsDb & data);
	void printPaperStatsGh(const WD_PrintPaperStatsGh & data);
	void printCellNeighborStats(const WD_PrintCellNeighborStats & data);
	void dumpAllItemTagsWithInheritedTags(const WD_DumpAllItemTagsWithInheritedTags & data);
	void printCTCSelectiveStorageStats(const WD_PrintCTCSelectiveStorageStats & data);
	void printCTCStorageStats(const WD_PrintCTCStorageStats & data);
	void printStats(const WD_PrintStats & data);
	void printStatsSingle(const WD_PrintStatsSingle & data);
	void printCellStats(const WD_PrintCellStats & data);
	void printCQRDataSize(const WD_PrintCQRDataSize & data);
	void consistencyCheck(oscarcmd::WD_ConsistencyCheck& d);

	void selectTextCompleter(WD_SelectTextCompleter & d);
	void selectGeoCompleter(WD_SelectGeoCompleter & d);
	void printSelectedTextCompleter();
	void printSelectedGeoCompleter();
	
	void ghId2StoreId(WD_GhId2StoreId & d);
	void storeId2GhId(WD_StoreId2GhId & d);
	
	void dumpIndex(WD_DumpIndex& d);
	void dumpItem(WD_DumpItem & d);
	void dumpAllItems(WD_DumpAllItems & d);
	void dumpGhRegion(WD_DumpGhRegion & d);
	void dumpGhCell(WD_DumpGhCell & d);
	void dumpGhCellParents(WD_DumpGhCellParents & d);
	void dumpGhCellItems(WD_DumpGhCellItems & d);
	void dumpGhRegionChildren(WD_DumpGhRegionChildren & d);
	void dumpGhRegionItems(WD_DumpGhRegionItems & d);
	void dumpGh(WD_DumpGh & d);
	void dumpGhPath(oscarcmd::WD_DumpGhPath& d);
	void dumpKeyStringTable(WD_DumpKeyStringTable& d);
	void dumpValueStringTable(WD_DumpValueStringTable& d);
	void dumpItemTags(WD_DumpItemTags & d);

	void interactivePartial(WD_InteractivePartial & d);
	void interactiveSimple(WD_InteractiveSimple & d);
	void interactiveFull(WD_InteractiveFull & d);

	void cellsFromQuery(WD_CellsFromQuery & d);

	void completeStringPartial(WD_CompleteStringPartial & d);
	void completeStringSimple(WD_CompleteStringSimple& d);
	void completeStringFull(WD_CompleteStringFull& d);
	void completeStringClustered(WD_CompleteStringClustered & d, bool treedCQR);
	
	void completeFromFilePartial(WD_CompleteFromFilePartial & d);
	void completeFromFileSimple(WD_CompleteFromFileSimple& d);
	void completeFromFileFull(WD_CompleteFromFileFull& d);
	void completeFromFileClustered(WD_CompleteFromFileClustered & d, bool treedCQR);
	
	void symDiffCompleters(WD_SymDiffItemsCompleters & d);
	
	void createCompletionStrings(WD_CreateCompletionStrings & d);
	
	void benchmark(WD_Benchmark & d);
};

}//end namespace oscarcmd::wf


#endif