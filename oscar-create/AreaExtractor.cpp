#include "AreaExtractor.h"
#include "helpers.h"
#include "common.h"

namespace oscar_create {

inline BoundariesInfo boundariesInfoFromPrimitive(osmpbf::IPrimitive & prim) {
	BoundariesInfo bi;
	bi.osmIdType.id(prim.id());
	bi.osmIdType.type(oscar_create::toOsmItemType( prim.type() ));
	for(int i(0), s(prim.tagsSize()); i < s; ++i) {
		std::string wkstr = prim.key(i);
		if (wkstr == "admin_level") {
			bi.adminLevel = atoi(prim.value(i).c_str());
		}
	}
	return bi;
}

AreaExtractor::AreaExtractor() {}

AreaExtractor::~AreaExtractor() {}


generics::RCPtr<osmpbf::AbstractTagFilter>
AreaExtractor::nameFilter(const std::string & keysDefiningRegionsFile, const std::string & keyValuesDefiningRegionsFile) {
	std::unordered_set<std::string> keys;
	std::unordered_map<std::string, std::unordered_set<std::string> > keyValues; 
	std::insert_iterator< std::unordered_set<std::string> > keyInsertIt(keys, keys.end());
	readKeysFromFile< std::insert_iterator< std::unordered_set<std::string> > >(keysDefiningRegionsFile, keyInsertIt);
	readKeyValuesFromFile(keyValuesDefiningRegionsFile, keyValues);

	generics::RCPtr<osmpbf::AbstractTagFilter> filter;
	if (keys.size()) {
		std::string regexString = "(";
		for(const std::string & x : keys) {
			regexString += "(" + x + ")|";
		}
		regexString.back() = ')';
		
		filter.reset( new osmpbf::RegexKeyTagFilter(regexString) );
	}
	if (keyValues.size()) {
		osmpbf::MultiKeyMultiValueTagFilter * kvFilter = new osmpbf::MultiKeyMultiValueTagFilter();
		for(const auto & x : keyValues) {
			kvFilter->addValues(x.first, x.second.begin(), x.second.end());
		}
		if (filter.get()) {
			filter.reset(osmpbf::newOr(filter.get(), kvFilter) );
		}
		else {
			filter.reset( kvFilter );
		}
	}
	return filter;
}

bool AreaExtractor::extract(const std::string & inputFileName, const generics::RCPtr<osmpbf::AbstractTagFilter> & filter, OsmPolygonStore & polystore, uint32_t numThreads, uint32_t /*maxNodeTableSize*/) {
	auto ef = [&polystore](const std::shared_ptr<sserialize::spatial::GeoRegion> & region, osmpbf::IPrimitive & primitive) {
		polystore.push_back(*region, boundariesInfoFromPrimitive(primitive));
	};
	return m_ae.extract(inputFileName, ef, osmtools::AreaExtractor::ET_ALL_SPECIAL_BUT_BUILDINGS, filter, numThreads, "Region extraction");
}

bool AreaExtractor::extract(const std::string & file, oscar_create::OsmPolygonStore & polystore) {
	auto ef = [&polystore](const std::shared_ptr<sserialize::spatial::GeoRegion> & region, osmpbf::IPrimitive & primitive) {
		polystore.push_back(*region, boundariesInfoFromPrimitive(primitive));
	};
	return m_ae.extract(file, ef, osmtools::AreaExtractor::ET_ALL_SPECIAL_BUT_BUILDINGS);
}

}//end namespace