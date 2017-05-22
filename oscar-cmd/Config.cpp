#include "Config.h"
#include <iostream>
#include <sserialize/strings/unicode_case_functions.h>

namespace oscarcmd {

//helpers
PrintStatsSelection printStatsfromString(const std::string & istr) {
	std::string str = sserialize::unicode_to_lower(istr);
	
	if (str == "all")
		return PS_ALL;
	else if ("idxstore" == str || "idx" == str || "index" == str || "indexstore" == str) 
		return PS_IDX_STORE;
	else if("completer" == str)
		return PS_COMPLETER;
	else if("db" == str)
		return PS_DB;
	else if("gh" == str)
		return PS_GH;
	else if("geo" == str)
		return PS_GEO;
	else if("tag" == str)
		return PS_TAG;
	else
		return PS_NONE;
}

liboscar::TextSearch::Type textSearchTypeFromString(const std::string & str) {
	if (str == "items")
		return liboscar::TextSearch::ITEMS;
	else if (str == "geoh")
		return liboscar::TextSearch::GEOHIERARCHY;
	else if (str == "geocell")
		return liboscar::TextSearch::GEOCELL;
	else
		return liboscar::TextSearch::INVALID;
};

void Config::printHelp() {
	std::cout << "Oscar -- Help\n";
	std::cout <<
"\n \
--help \n \
--tempfileprefix path\tpath to temp files \n \
-l\tlist completers \n \
-v num\tverbosity option, print num results (-1 for all) \n \
-m string\tset the current completion string used in completions \n \
-t num\tnumber of threads to use (i.e. for treedcqr flattening) \n \
--create-completion-strings num\tcreate num completion strings from the store \n \
-ssc ts,ct\tselect the textsearch completer ts=(items|geoh|geocell),ct=num \n \
-sgc num\tselect geo completer \n \
-cfq list cqr from query\n \
-csp num\tpartial complete string and seek num items \n \
-css ms,mr\tsimple complete string with minStrLen=ms and maxResultSetSize=mr \n \
-csf\tfull complete string \n \
-csc\tgeocell complete string \n \
-csct\tgeocell with treed-cqr complete string \n \
-cip num\tinteractive partial complete and seek num items \n \
-cis ms,mr\tinteractive simple complete with minStrLen=ms and maxResultSetSize=mr \n \
-cif\tinteractive full complete \n \
-cfp num,file\tpartial complete strings from file and seek num items \n \
-cfs ms,mr,file\tsimple complete strings from file with minStrLen=ms and maxResultSetSize=mr \n \
-cff file\tfull complete strings from file \n \
-cfc file\tgeocell complete strings from file \n \
-cfct file\tgeocell complete strings from file with treed cqr \n \
--symdiff-items-completers c1,c2\tprint symmetric difference between completer c1 and completer c2 \n \
-ds which\tprint stats: all,idxstore,completer,db,geo,tag \n \
-dcs\tdump cell statistics \n \
-dpsdb file\tprint db paper stats, file sets the interesting tags \n \
-dpsgh file\tprint gh paper stats, file is outfile \n \
-dcns <num>,[file]\tprint cell neighbor distance stats \n \
-dctcs file\tprint ctc stats to file \n \
-dctcss (exact|prefix|suffix|substring)\tprint selective ctc storage stats \n \
-dcqrds str\tprint cqr data size for string str \n \
-dsf which,file\tprint stats to file: all,idxstore,completer,db,geo,tag \n \
-dss which,selector\tprint single stat \n \
-dx num\tdump index with id=num \n \
-di num\tdump item num \n \
--dump-all-items\tdump all items \n \
--dump-all-itemtags-with-inherited-tags k=keyfile,o=outfile\tdump all items \n \
-dks file\tdump key string table to file \n \
-dvs file\tdump value string table to file \n \
-dit file\tdump item tags as key:value to file \n \
-gh2sId ghId\tprint store id of region with id \n \
-s2ghId sId\tprint gh id of region with storeId id \n \
-dghrc ghId\tprint children regions of region with ghId (-1==root region) \n \
-dghci cellId\tdump items of cell id \n \
-dghri ghId\tdump items of region id \n \
-dghcp cellId\tdump parents of cell id \n \
-dghp path\tdump gh path \n \
-dgh file\tdump gh as dot file to file \n \
-dghr storeid\tdump gh region \n \
-dghc id\tdump gh cell \n \
--check type\tcheck type=(index|store|gh|tds) \n \
--benchmark\tdo a benchmark \n \
--mlock index,kvstore,textsearch\tlock memory of specified file\n \
--munlock index,kvstore,textsearch\tunlock memory of specified file\n";
}

int Config::parseSingleArg(int argc, char ** argv, int & i, int & printNumResults, int & threadCount, std::string & completionString) {
	std::string arg(argv[i]);
	if (arg == "--help") {
		printHelp();
		return PRT_HELP;
	}
	else if (arg == "--tempfileprefix" && i+1 < argc) {
		sserialize::UByteArrayAdapter::setTempFilePrefix( std::string(argv[i+1]) );
		i++;
	}
	else if (arg == "-l") {
		workItems.emplace_back(WorkItem::LIST_COMPLETERS);
	}
	
	//Now the state full opts
	else if (arg == "-v" && i+1 < argc) {
		printNumResults = atoi(argv[i+1]);
		++i;
	}
	else if (arg == "-m" && i+1 < argc) {
		completionString = std::string(argv[i+1]);
		i++;
	}
	else if (arg == "-t" && i+1 < argc) {
		threadCount = atoi(argv[i+1]);
		i++;
	}
	
	//data creation opts
	
	else if (arg == "--create-completion-strings" && i+1 < argc) {
		std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(std::string(argv[i+1]),',');
		uint32_t count = atoi(tmp.at(1).c_str());
		workItems.emplace_back(WorkItem::CREATE_COMPLETION_STRINGS, new WD_CreateCompletionStrings(tmp.at(0), count) );
		i++;
	}
	
	//completion selection opts
	
	else if (arg == "-ssc" && i+1 < argc) {
		std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(std::string(argv[i+1]), ',');
		liboscar::TextSearch::Type ts = textSearchTypeFromString(tmp.at(0));
		int tsc = atoi(tmp.at(1).c_str());
		workItems.emplace_back(Config::WorkItem::SELECT_TEXT_COMPLETER, new WD_SelectTextCompleter(ts, tsc));
		i += 1;
	}
	else if (arg == "-sgc" && i+1 < argc) {
		int sc = atoi(argv[i+1]); 
		workItems.emplace_back(Config::WorkItem::SELECT_GEO_COMPLETER, new WD_SelectGeoCompleter(sc));
		i++;
	}
	
	//completion with string
	else if (arg == "-csp" && i+1 < argc) {
		workItems.emplace_back(Config::WorkItem::COMPLETE_STRING_PARTIAL, new WD_CompleteStringPartial(completionString, printNumResults, atoi(argv[i+1])));
		++i;
	}
	else if (arg == "-css" && i+1 < argc) {
		std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(std::string(argv[i+1]), ',');
		uint32_t minStrLen = atoi(tmp.at(0).c_str());
		uint32_t maxResultSetSize = atoi(tmp.at(1).c_str());
		workItems.emplace_back(Config::WorkItem::COMPLETE_STRING_SIMPLE, new WD_CompleteStringSimple(completionString, printNumResults, minStrLen, maxResultSetSize));
		++i;
	}
	else if (arg == "-csf") {
		workItems.emplace_back(Config::WorkItem::COMPLETE_STRING_FULL, new WD_CompleteStringFull(completionString, printNumResults));
	}
	else if (arg == "-csc") {
		workItems.emplace_back(Config::WorkItem::COMPLETE_STRING_CLUSTERED, new WD_CompleteStringClustered(completionString, printNumResults, threadCount));
	}
	else if (arg == "-csct") {
		workItems.emplace_back(Config::WorkItem::COMPLETE_STRING_CLUSTERED_TREED_CQR, new WD_CompleteStringClusteredTreedCqr(completionString, printNumResults, threadCount));
	}
	else if (arg == "-cfq") {
		workItems.emplace_back(Config::WorkItem::CELLS_FROM_QUERY, new WD_CellsFromQuery(completionString, threadCount));
	}
	else if (arg == "-cifq" && i+1 < argc) {
		workItems.emplace_back(Config::WorkItem::CELL_IMAGE_FROM_QUERY, new WD_CellImageFromQuery(completionString, std::string(argv[i+1])));
		++i;
	}
	//interactive complete
	
	else if (arg == "-cip" && i+1 < argc) {
		workItems.emplace_back(Config::WorkItem::INTERACTIVE_PARTIAL, new WD_InteractivePartial(printNumResults, atoi(argv[i+1])));
		++i;
	}
	else if (arg == "-cis" && i+1 < argc) {
		std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(std::string(argv[i+1]), ',');
		uint32_t minStrLen = atoi(tmp.at(0).c_str());
		uint32_t maxResultSetSize = atoi(tmp.at(1).c_str());
		workItems.emplace_back(Config::WorkItem::INTERACTIVE_SIMPLE, new WD_InteractiveSimple(printNumResults, minStrLen, maxResultSetSize));
		++i;
	}
	else if (arg == "-cif" && i+1 < argc) {
		workItems.emplace_back(Config::WorkItem::INTERACTIVE_FULL, new WD_InteractiveFull( sserialize::toBool(std::string(argv[i+1])) ) );
		++i;
	}
	
	//completion from file
	else if (arg == "-cfp" && i+1 < argc) {
		std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(std::string(argv[i+1]), ',');
		workItems.emplace_back(Config::WorkItem::COMPLETE_FROM_FILE_PARTIAL, new WD_CompleteFromFilePartial(printNumResults, tmp.at(1), atoi(tmp.at(0).c_str())));
		++i;
	}
	else if (arg == "-cfs" && i+1 < argc) {
		std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(std::string(argv[i+1]), ',');
		uint32_t minStrLen = atoi(tmp.at(0).c_str());
		uint32_t maxResultSetSize = atoi(tmp.at(1).c_str());
		workItems.emplace_back(Config::WorkItem::INTERACTIVE_SIMPLE, new WD_CompleteFromFileSimple(printNumResults, tmp.at(2), minStrLen, maxResultSetSize));
		++i;
	}
	else if (arg == "-cff" && i+1 < argc) {
		workItems.emplace_back(Config::WorkItem::COMPLETE_FROM_FILE_FULL, new WD_CompleteFromFileFull(printNumResults, std::string(argv[i+1])));
		++i;
	}
	else if (arg == "-cfc" && i+1 < argc) {
		workItems.emplace_back(Config::WorkItem::COMPLETE_FROM_FILE_CLUSTERED, new WD_CompleteFromFileClustered(printNumResults, std::string(argv[i+1]), threadCount));
		++i;
	}
	else if (arg == "-cfct" && i+1 < argc) {
		workItems.emplace_back(Config::WorkItem::COMPLETE_FROM_FILE_CLUSTERED_TREED_CQR, new WD_CompleteFromFileClusteredTreedCqr(printNumResults, std::string(argv[i+1]), threadCount));
		++i;
	}
	
	//special completions
	else if (arg == "--symdiff-items-completers" && i+1 < argc) {
		std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(std::string(argv[i+1]), ',');
		uint32_t c1 = atoi(tmp.at(0).c_str());
		uint32_t c2 = atoi(tmp.at(1).c_str());
		workItems.emplace_back(Config::WorkItem::SYMDIFF_ITEMS_COMPLETERS, new WD_SymDiffItemsCompleters(printNumResults, completionString, c1, c2));
		++i;
	}

	//statistic opts
	
	else if (arg == "-ds" && i+1 < argc) {
		PrintStatsSelection p = printStatsfromString( std::string(argv[i+1]) );
		workItems.emplace_back(WorkItem::PRINT_STATS, new WD_PrintStats(p) );
		i++;
	}
	else if (arg == "-dcs") {
		workItems.emplace_back(WorkItem::PRINT_CELL_STATS, new WD_PrintCellStats());
	}
	else if (arg == "-dpsdb" && i+1 < argc) {
		workItems.emplace_back(WorkItem::PRINT_PAPER_STATS_DB, new WD_PrintPaperStatsDb(std::string(argv[i+1])) );
		i++;
	}
	else if (arg == "-dcns" && i+1 < argc) {
		workItems.emplace_back(WorkItem::PRINT_CELL_NEIGHBOR_STATS, new WD_PrintCellNeighborStats(std::string(argv[i+1])) );
		i++;
	}
	else if (arg == "-dpsgh" && i+1 < argc) {
		workItems.emplace_back(WorkItem::PRINT_PAPER_STATS_GH, new WD_PrintPaperStatsGh(std::string(argv[i+1])) );
		i++;
	}
	else if (arg == "-dctcs" && i+1 < argc) {
		workItems.emplace_back(WorkItem::PRINT_CTC_STATS, new WD_PrintCTCStorageStats(std::string(argv[i+1])) );
		i++;
	}
	else if (arg == "-dctcss" && i+1 < argc) {
		workItems.emplace_back(WorkItem::PRINT_CTC_SELECTIVE_STATS, new WD_PrintCTCSelectiveStorageStats(std::string(argv[i+1])) );
		i++;
	}
	else if (arg == "-dcqrds" && i+1 < argc) {
		workItems.emplace_back(WorkItem::PRINT_CQR_DATA_SIZE, new WD_PrintCQRDataSize(std::string(argv[i+1])) );
		i++;
	}
	else if (arg == "-dsf" && i+1 < argc) {
		std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(std::string(argv[i+1]), ',');
		PrintStatsSelection p = printStatsfromString(tmp.at(0));
		workItems.emplace_back(WorkItem::PRINT_STATS, new WD_PrintStats(p,  tmp.at(1)) );
		++i;
	}
	else if (arg == "-dss" && i+1 < argc) {
		std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(std::string(argv[i+1]), ',');
		PrintStatsSelection p = printStatsfromString(tmp.at(0));
		uint32_t psub = atoi(tmp.at(1).c_str());
		workItems.emplace_back(WorkItem::PRINT_STATS_SINGLE, new WD_PrintStatsSingle(p, psub) );
		i += 1;
	}
	
	//some info opts
	else if (arg == "-gh2sId" && i+1 < argc) {
		workItems.emplace_back(WorkItem::GH_ID_2_STORE_ID, new WD_GhId2StoreId(atoi(argv[i+1])) );
		++i;
	}
	else if (arg == "-s2ghId" && i+1 < argc) {
		workItems.emplace_back(WorkItem::STORE_ID_2_GH_ID, new WD_StoreId2GhId(atoi(argv[i+1])) );
		++i;
	}
	
	//Dump opts
	else if (arg == "-dx" && i+1 < argc) {
		workItems.emplace_back(WorkItem::DUMP_INDEX, new WD_DumpIndex(atoi(argv[i+1])) );
		++i;
	}
	else if (arg == "-di" && i+1 < argc) {
		workItems.emplace_back(WorkItem::DUMP_ITEM, new WD_DumpItem(atoi(argv[i+1])) );
		++i;
	}
	else if (arg == "--dump-all-items") {
		workItems.emplace_back(WorkItem::DUMP_ALL_ITEMS, new WD_DumpAllItems(false) );
	}
	else if (arg == "--dump-all-itemtags-with-inherited-tags" && i+1 < argc) {
		std::vector<std::string> tmp = sserialize::split< std::vector<std::string> >(std::string(argv[i+1]), ',');
		if (tmp.size() != 2 || tmp.front().size() <= 2 || tmp.back().size() <= 2) {
			std::cout << "Not enough options given for --dump-all-item-tags-with-inherited-tags" << std::endl;
			return PRT_HELP;
		}
		std::string tagFileName, outFileName;
		for(const std::string & s : tmp) {
			if (s.at(0) == 'k') {
				tagFileName.insert(tagFileName.end(), s.begin()+2, s.end());
			}
			else if (s.at(0) == 'o') {
				outFileName.insert(outFileName.end(), s.begin()+2, s.end());
			}
		}
		workItems.emplace_back(WorkItem::DUMP_ALL_ITEM_TAGS_WITH_INHERITED_TAGS, new WD_DumpAllItemTagsWithInheritedTags(tagFileName, outFileName) );
		i += 1;
	}
	else if (arg == "-dks" && i+1 < argc) {
		workItems.emplace_back(WorkItem::DUMP_KEY_STRING_TABLE, new WD_DumpKeyStringTable(argv[i+1]) );
		++i;
	}
	else if (arg == "-dvs" && i+1 < argc) {
		workItems.emplace_back(WorkItem::DUMP_VALUE_STRING_TABLE, new WD_DumpValueStringTable(argv[i+1]) );
		++i;
	}
	else if (arg == "-dit" && i+1 < argc) {
		workItems.emplace_back(WorkItem::DUMP_ITEM_TAGS, new WD_DumpItemTags(argv[i+1]) );
		++i;
	}
	else if (arg == "-dghcp" && i+1 < argc) {
		workItems.emplace_back(WorkItem::DUMP_GH_CELL_PARENTS, new WD_DumpGhCellParents(atoi(argv[i+1])) );
		++i;
	}
	else if (arg == "-dghci" && i+1 < argc) {
		workItems.emplace_back(WorkItem::DUMP_GH_CELL_ITEMS, new WD_DumpGhCellItems(atoi(argv[i+1])) );
		++i;
	}
	else if (arg == "-dghri" && i+1 < argc) {
		workItems.emplace_back(WorkItem::DUMP_GH_REGION_ITEMS, new WD_DumpGhRegionItems(atoi(argv[i+1])) );
		++i;
	}
	else if (arg == "-dghrc" && i+1 < argc) {
		int tmp = atoi(argv[i+1]);
		uint32_t regionId = 0xFFFFFFFF;
		if (tmp >= 0) {
			regionId = tmp;
		}
		workItems.emplace_back(WorkItem::DUMP_GH_REGION_CHILDREN, new WD_DumpGhRegionChildren(regionId));
		++i;
	}	
	else if (arg == "-dghp" && i+1 < argc) {
		std::vector<std::string> strPath = sserialize::split< std::vector<std::string> >(argv[i+1], ';');
		std::vector<uint32_t> tmp;
		sserialize::transform<decltype(tmp)>(strPath.begin(), strPath.end(), [](const std::string & x) -> uint32_t {return atoi(x.c_str());});
		workItems.emplace_back(WorkItem::DUMP_GH_PATH, new WD_DumpGhPath(tmp) );
		++i;
	}
	else if (arg == "-dgh" && i+1 < argc) {
		workItems.emplace_back(WorkItem::DUMP_GH, new WD_DumpGh(argv[i+1]) );
		++i;
	}
	else if (arg == "-dghr" && i+1 < argc) {
		workItems.emplace_back(WorkItem::DUMP_GH_REGION, new WD_DumpGhRegion( atoi(argv[i+1]) ) );
		++i;
	}
	else if (arg == "-dghc" && i+1 < argc) {
		workItems.emplace_back(WorkItem::DUMP_GH_CELL, new WD_DumpGhCell( atoi(argv[i+1]) ) );
		++i;
	}
	
	//misc
	
	else if (arg == "--check" && i+1 < argc) {
		workItems.emplace_back(WorkItem::CONSISTENCY_CHECK, new WD_ConsistencyCheck(std::string(argv[i+1])));
		++i;
	}
	else if (arg == "--benchmark" && i+1 < argc) {
		std::string cfgStr(argv[i+1]);
		Benchmarker::Config cfg(cfgStr);
		workItems.emplace_back(WorkItem::BENCHMARK, new WD_Benchmark(cfg));
		++i;
	}
	else if (arg == "--mlock" && i+1 < argc) {
		workItems.emplace_back(WorkItem::LOCK_MEMORY, new WD_LockMemory(std::string(argv[i+1])));
		++i;
	}
	else if (arg == "--munlock" && i+1 < argc) {
		workItems.emplace_back(WorkItem::UNLOCK_MEMORY, new WD_UnlockMemory(std::string(argv[i+1])));
		++i;
	}
	else if (arg.size() && arg[0] == '-') {
		std::cerr << "Unkown option: " << arg << std::endl;
		return PRT_HELP;
	}
	else {
		inFileName = arg;
	}
	return PRT_OK;
}

bool Config::fromCommandline(int argc, char ** argv) {
	//state variables
	int printNumResults = -1;
	std::string completionString;
	int threadCount = 1;
	if (argc > 1) {
		for(int i = 0; i < argc; i++) {
			try {
				int prt = parseSingleArg(argc, argv, i, printNumResults, threadCount, completionString);
				switch(prt) {
				case PRT_HELP:
					return true;
				case PRT_OK:
				default:
					break;
				}
			}//end try block
			catch(const std::exception & e) {
				std::cout << "An error occured while processing argument " << i << "=" << argv[i] << std::endl;
				std::cout << "The error was: " << e.what() << std::endl;
				return false;
			}
		}
	}
	else {
		std::cout << "Arguments given: " << std::endl;
		for (int i=0; i < argc; i++) {
			std::cout << argv[i];
		}
		std::cout << std::endl << "Need in filename" << std::endl;
		printHelp();
		return false;
	}
	return true;
}

bool Config::valid() {
	if (!sserialize::MmappedFile::isDirectory(inFileName.c_str())) {
		std::cout << "No directory given as path to the completion files" << std::endl;
		return false;
	}
	return true;
}


}//end namespace