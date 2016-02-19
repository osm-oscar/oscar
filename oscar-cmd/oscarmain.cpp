#include <iostream>
#include <sstream>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <cstdlib> 
#include <fstream>

#include <sserialize/vendor/utf8.h>
#include <sserialize/strings/stringfunctions.h>
#include <sserialize/strings/unicode_case_functions.h>
#include <sserialize/storage/MmappedFile.h>

#include "CompletionStringCreators.h"
#include "Config.h"
#include "Workfunctions.h"

using namespace oscarcmd;



int main(int argc, char **argv) {
	std::srand ( 0 );
	std::cout.precision(10);
	Config config;
	if (!config.fromCommandline(argc, argv)) {
		return 1;
	}
#ifdef __ANDROID__
	std::cout << "Parsed the commandline" << std::endl;
#endif
	
	Worker worker;
	liboscar::Static::OsmCompleter & osmCompleter = worker.completer;
	osmCompleter.setAllFilesFromPrefix(config.inFileName);


#ifdef __ANDROID__
	std::cout << "Trying to energize" << std::endl;
#endif
	
	try {
		osmCompleter.energize();
	}
	catch (const std::exception & e) {
		std::cerr << "Error occured: " << e.what() << std::endl;
		return 1;
	}
#ifdef __ANDROID__
	std::cout << "Doing the work" << std::endl;
#endif
	bool allOk = true;
	for(std::size_t i(0), s(config.workItems.size()); i < s; ++i) {
		Config::WorkItem & workItem = config.workItems[i];
// 		try {
			switch (workItem.type) {
			case Config::WorkItem::SELECT_TEXT_COMPLETER:
				worker.selectTextCompleter(*workItem.data->as<WD_SelectTextCompleter>());
				break;
			case Config::WorkItem::SELECT_GEO_COMPLETER:
				worker.selectGeoCompleter(*workItem.data->as<WD_SelectGeoCompleter>());
			case Config::WorkItem::PRINT_SELECTED_TEXT_COMPLETER:
				worker.printSelectedTextCompleter();
				break;
			case Config::WorkItem::PRINT_SELECTED_GEO_COMPLETER:
				worker.printSelectedGeoCompleter();
				break;
			case Config::WorkItem::GH_ID_2_STORE_ID:
				worker.ghId2StoreId(*workItem.data->as<WD_GhId2StoreId>());
				break;
			case Config::WorkItem::DUMP_INDEX:
				worker.dumpIndex(*workItem.data->as<WD_DumpIndex>());
				break;
			case Config::WorkItem::DUMP_ITEM:
				worker.dumpItem(*workItem.data->as<WD_DumpItem>());
				break;
			case Config::WorkItem::DUMP_ALL_ITEMS:
				worker.dumpAllItems(*workItem.data->as<WD_DumpAllItems>());
				break;
			case Config::WorkItem::DUMP_ALL_ITEM_TAGS_WITH_INHERITED_TAGS:
				worker.dumpAllItemTagsWithInheritedTags(*workItem.data->as<WD_DumpAllItemTagsWithInheritedTags>());
				break;
			case Config::WorkItem::DUMP_GH_REGION:
				worker.dumpGhRegion(*workItem.data->as<WD_DumpGhRegion>());
				break;
			case Config::WorkItem::DUMP_GH_CELL:
				worker.dumpGhCell(*workItem.data->as<WD_DumpGhCell>());
				break;
			case Config::WorkItem::DUMP_GH_CELL_PARENTS:
				worker.dumpGhCellParents(*workItem.data->as<WD_DumpGhCellParents>());
				break;
			case Config::WorkItem::DUMP_GH_CELL_ITEMS:
				worker.dumpGhCellItems(*workItem.data->as<WD_DumpGhCellItems>());
				break;
			case Config::WorkItem::DUMP_GH_REGION_CHILDREN:
				worker.dumpGhRegionChildren(*workItem.data->as<WD_DumpGhRegionChildren>());
				break;
			case Config::WorkItem::DUMP_GH_REGION_ITEMS:
				worker.dumpGhRegionItems(*workItem.data->as<WD_DumpGhRegionItems>());
				break;
			case Config::WorkItem::DUMP_GH:
				worker.dumpGh(*workItem.data->as<WD_DumpGh>());
				break;
			case Config::WorkItem::DUMP_KEY_STRING_TABLE:
				worker.dumpKeyStringTable(*workItem.data->as<WD_DumpKeyStringTable>());
				break;
			case Config::WorkItem::DUMP_VALUE_STRING_TABLE:
				worker.dumpValueStringTable(*workItem.data->as<WD_DumpValueStringTable>());
				break;
			case Config::WorkItem::DUMP_ITEM_TAGS:
				worker.dumpItemTags(*workItem.data->as<WD_DumpItemTags>());
				break;
			case Config::WorkItem::PRINT_STATS:
				worker.printStats(*workItem.data->as<WD_PrintStats>());
				break;
			case Config::WorkItem::PRINT_CELL_NEIGHBOR_STATS:
				worker.printCellNeighborStats(*workItem.data->as<WD_PrintCellNeighborStats>());
				break;
			case Config::WorkItem::PRINT_PAPER_STATS_DB:
				worker.printPaperStatsDb(*workItem.data->as<WD_PrintPaperStatsDb>());
				break;
			case Config::WorkItem::PRINT_PAPER_STATS_GH:
				worker.printPaperStatsGh(*workItem.data->as<WD_PrintPaperStatsGh>());
				break;
			case Config::WorkItem::PRINT_CTC_STATS:
				worker.printCTCStorageStats(*workItem.data->as<WD_PrintCTCStorageStats>());
				break;
			case Config::WorkItem::PRINT_CTC_SELECTIVE_STATS:
				worker.printCTCSelectiveStorageStats(*workItem.data->as<WD_PrintCTCSelectiveStorageStats>());
				break;
			case Config::WorkItem::PRINT_CQR_DATA_SIZE:
				worker.printCQRDataSize(*workItem.data->as<WD_PrintCQRDataSize>());
				break;
			case Config::WorkItem::LIST_COMPLETERS:
				worker.listCompleters();
				break;
			case Config::WorkItem::INTERACTIVE_PARTIAL:
				worker.interactivePartial(*workItem.data->as<WD_InteractivePartial>());
				break;
			case Config::WorkItem::INTERACTIVE_SIMPLE:
				worker.interactiveSimple(*workItem.data->as<WD_InteractiveSimple>());
				break;
			case Config::WorkItem::INTERACTIVE_FULL:
				worker.interactiveFull(*workItem.data->as<WD_InteractiveFull>());
				break;
			case Config::WorkItem::COMPLETE_STRING_PARTIAL:
				worker.completeStringPartial(*workItem.data->as<WD_CompleteStringPartial>());
				break;
			case Config::WorkItem::COMPLETE_STRING_SIMPLE:
				worker.completeStringSimple(*workItem.data->as<WD_CompleteStringSimple>());
				break;
			case Config::WorkItem::COMPLETE_STRING_FULL:
				worker.completeStringFull(*workItem.data->as<WD_CompleteStringFull>());
				break;
			case Config::WorkItem::COMPLETE_STRING_CLUSTERED:
				worker.completeStringClustered(*workItem.data->as<WD_CompleteStringClustered>(), false);
				break;
			case Config::WorkItem::COMPLETE_STRING_CLUSTERED_TREED_CQR:
				worker.completeStringClustered(*workItem.data->as<WD_CompleteStringClustered>(), true);
				break;
			case Config::WorkItem::COMPLETE_FROM_FILE_PARTIAL:
				worker.completeFromFilePartial(*workItem.data->as<WD_CompleteFromFilePartial>());
				break;
			case Config::WorkItem::COMPLETE_FROM_FILE_SIMPLE:
				worker.completeFromFileSimple(*workItem.data->as<WD_CompleteFromFileSimple>());
				break;
			case Config::WorkItem::COMPLETE_FROM_FILE_FULL:
				worker.completeFromFileFull(*workItem.data->as<WD_CompleteFromFileFull>());
				break;
			case Config::WorkItem::COMPLETE_FROM_FILE_CLUSTERED:
				worker.completeFromFileClustered(*workItem.data->as<WD_CompleteFromFileClustered>(), false);
				break;
			case Config::WorkItem::COMPLETE_FROM_FILE_CLUSTERED_TREED_CQR:
				worker.completeFromFileClustered(*workItem.data->as<WD_CompleteFromFileClustered>(), true);
				break;
			case Config::WorkItem::SYMDIFF_ITEMS_COMPLETERS:
				worker.symDiffCompleters(*workItem.data->as<WD_SymDiffItemsCompleters>());
			case Config::WorkItem::CONSISTENCY_CHECK:
				worker.consistencyCheck(*workItem.data->as<WD_ConsistencyCheck>());
				break;
			case Config::WorkItem::CREATE_COMPLETION_STRINGS:
				worker.createCompletionStrings(*workItem.data->as<WD_CreateCompletionStrings>());
				break;
			case Config::WorkItem::BENCHMARK:
				worker.benchmark(*workItem.data->as<WD_Benchmark>());
				break;
			default:
				throw std::runtime_error("Bad operation. Unkown work item.");
			};
// 		}
// 		catch(std::exception & e) {
// 			std::cerr << "Error occured: " << e.what() << std::endl;
// 			allOk = false;
// 		}
	}
	return (allOk ? 0 : 1);
}