#include "MainHandler.h"
#include <cppcms/service.h>
#include <cppcms/applications_pool.h>
#include <iostream>
#include <liboscar/StaticOsmCompleter.h>
#include <sserialize/algorithm/hashspecializations.h>
#include "types.h"

/*

Config:
	dbfiles : [ {"name" : "name of db", "urlpath" : "path/for/url", "path" : "/full/path/to/db/folder"}]


query: ?q=<query-string>&limit=num

<searchresults querystring="135 pilkington, avenue birmingham">
	</place 
// 		boundingbox="52.548641204834,52.5488433837891,-1.81612110137939,-1.81592094898224" 
		at="52.5487429714954" lon="-1.81602098644987" 
		display_name="135, Pilkington Avenue, Wylde Green, City of Birmingham, West Midlands (county), B72, United Kingdom" 
	>
 </searchresults>

*/

bool initGhFilters(cppcms::json::value ghfilter, oscar_web::CompletionFileDataPtr dataPtr) {
	if (ghfilter.is_null()) {
		return true;
	}
	cppcms::json::array & filterArray = ghfilter.array();
	std::unordered_map<std::string, sserialize::spatial::GeoHierarchySubSetCreator> & ghSubSetCreators = dataPtr->ghSubSetCreators;
	for(const cppcms::json::value & v : filterArray) {
		if (v.type() != cppcms::json::is_object) {
			continue;
		}
		const cppcms::json::object & filterDef = v.object();
		
		std::string name = filterDef.at("name").str();
		
		const auto & store = dataPtr->completer->store();
		
		//keys
		std::unordered_set<uint32_t> keys;
		//key-values
		std::unordered_set< std::pair<uint32_t, uint32_t> > kv;
		
		if (filterDef.count("k")) {
			for(const cppcms::json::value & x : filterDef.at("k").array()) {
				const std::string & key = x.str();
				keys.insert( store.keyStringTable().find(key)) ;//don't care about npos
			}
		}
		
		if (filterDef.count("kv")) {
			for(const cppcms::json::object::value_type & x : filterDef.at("kv").object()) {
				const std::string & key = x.first;
				const std::string & value = x.second.str();
				kv.emplace(store.keyStringTable().find(key), store.valueStringTable().find(value));
			}
		}
		
		const auto & gh = store.geoHierarchy();
		std::vector<bool> validRegions(store.geoHierarchy().regionSize(), false);
		for(uint32_t regionId(0), regionIdEnd(gh.regionSize()); regionId < regionIdEnd; ++regionId) {
			uint32_t storeId = gh.ghIdToStoreId(regionId);
			auto item = store.at(storeId);
			for(uint32_t i(0), s(item.size()); i < s; ++i) {
				uint32_t keyId = item.keyId(i);
				if (keys.count(keyId) || kv.count(std::pair<uint32_t, uint32_t>(keyId, item.valueId(i)))) {
					validRegions.at(regionId) = true;
				}
			}
		}
		
		ghSubSetCreators.emplace(name,
			sserialize::spatial::GeoHierarchySubSetCreator(gh,
				[&validRegions](uint32_t regionId) {
					return validRegions.at(regionId);
				}
			)
		);
	}
	return true;
}

int main(int argc, char **argv) {
	cppcms::json::value dbfile;
	cppcms::service app(argc,argv);
	try {
		dbfile = app.settings().find("dbfile");
	}
	catch (cppcms::json::bad_value_cast & e) {
		std::cerr << "Failed to parse dbfile object." << std::endl;
		return -1;
	}
	
	oscar_web::CompletionFileDataPtr completionFileDataPtr(new oscar_web::CompletionFileData() );
	oscar_web::CompletionFileData & data = *completionFileDataPtr;
	try {
		data.path = dbfile.get<std::string>("path");
		data.name = dbfile.get<std::string>("name");
		
		data.logFilePath = dbfile.get<std::string>("logfile");
		data.limit = dbfile.get<uint32_t>("limit", 32);
		data.chunkLimit =dbfile.get<uint32_t>("chunklimit", 8);
		data.minStrLen = dbfile.get<uint32_t>("minStrLen", 3);
		data.fullSubSetLimit = dbfile.get<uint32_t>("fullsubsetlimit", 100);
		data.maxIndexDBReq = dbfile.get<uint32_t>("maxindexdbreq", 10);
		data.maxItemDBReq = dbfile.get<uint32_t>("maxitemdbreq", 10);
		data.textSearchers[liboscar::TextSearch::GEOCELL] = dbfile.get<uint32_t>("geocellcompleter", 0);
		data.textSearchers[liboscar::TextSearch::OOMGEOCELL] = dbfile.get<uint32_t>("geocellcompleter", 0);
		data.textSearchers[liboscar::TextSearch::ITEMS] = dbfile.get<uint32_t>("itemscompleter", 0);
		data.textSearchers[liboscar::TextSearch::GEOHIERARCHY] = dbfile.get<uint32_t>("geohcompleter", 0);
		data.geocompleter = dbfile.get<uint32_t>("geocompleter", 0);
		data.treedCQR = dbfile.get<bool>("treedCQR", false);
	}
	catch (cppcms::json::bad_value_cast & e) {
		std::cerr << "Incomplete dbfiles entry: " << e.what() << std::endl;
		return -1;
	}
	
	data.completer = oscar_web::OsmCompleter( new liboscar::Static::OsmCompleter() );
	data.log = std::shared_ptr<std::ofstream>(new std::ofstream() );
	data.completer->setAllFilesFromPrefix(data.path);

	try {
		data.completer->energize();
	}
	catch (const std::exception & e) {
		std::cout << "Failed to initialize completer from " << data.path << ": " << e.what() << std::endl;
		return -1;
	}
	
	try {
		initGhFilters(app.settings().find("ghfilters"), completionFileDataPtr);
	}
	catch (cppcms::json::bad_value_cast & e) {
		std::cerr << "Failed to parse ghfilters object." << std::endl;
		return -1;
	}
	
	if (data.textSearchers.size()) {
		for(const auto & x : data.textSearchers) {
			if(!data.completer->setTextSearcher((liboscar::TextSearch::Type)x.first, x.second)) {
				std::cout << "Failed to set selected completer: " << (uint32_t)x.first << ":" << (uint32_t)x.second << std::endl;
			}
		}
	}

	if (!data.completer->setGeoCompleter(data.geocompleter)) {
		std::cout << "Failed to set seleccted geo completer: " << data.geocompleter<< std::endl;
	}
	if (data.completer->textSearch().hasSearch(liboscar::TextSearch::GEOCELL)) {
		std::cout << "Selected geocell text completer: " << data.completer->textSearch().get<liboscar::TextSearch::GEOCELL>().getName() << std::endl;
	}
	if (data.completer->textSearch().hasSearch(liboscar::TextSearch::OOMGEOCELL)) {
		std::cout << "Selected oomgeocell text completer: " << data.completer->textSearch().get<liboscar::TextSearch::OOMGEOCELL>().getName() << std::endl;
	}
	if (data.completer->textSearch().hasSearch(liboscar::TextSearch::ITEMS)) {
		std::cout << "Selected items text completer: " << data.completer->textSearch().get<liboscar::TextSearch::ITEMS>().getName() << std::endl;
	}
	if (data.completer->textSearch().hasSearch(liboscar::TextSearch::GEOHIERARCHY)) {
		std::cout << "Selected geohierarchy text completer: " << data.completer->textSearch().get<liboscar::TextSearch::GEOHIERARCHY>().getName() << std::endl;
	}
	
	std::cout << "Selected Geo completer: " << data.completer->geoCompleter()->describe() << std::endl; 
	
	if (!data.logFilePath.empty()) {
		data.log->open(data.logFilePath, std::ios::out | std::ios::app);
	}

	
	try {
		app.applications_pool().mount(cppcms::applications_factory<oscar_web::MainHandler, oscar_web::CompletionFileDataPtr>(completionFileDataPtr));
		app.run();
	}
	catch(std::exception const &e) {
		std::cerr << e.what() << std::endl;
	}
}