#include "AreaExtractor.h"
#include "helpers.h"
#include "common.h"

namespace oscar_create {

generics::RCPtr<osmpbf::AbstractTagFilter>
AreaExtractor::nameFilter(const std::string & keysDefiningRegionsFile, const std::string & keyValuesDefiningRegionsFile) {
	std::unordered_set<std::string> keys;
	std::unordered_map<std::string, std::unordered_set<std::string> > keyValues; 
	std::insert_iterator< std::unordered_set<std::string> > keyInsertIt(keys, keys.end());
	readKeysFromFile< std::insert_iterator< std::unordered_set<std::string> > >(keysDefiningRegionsFile, keyInsertIt);
	readKeyValuesFromFile(keyValuesDefiningRegionsFile, keyValues);

	generics::RCPtr<osmpbf::AbstractTagFilter> filter;
	if (!keys.size() && !keyValues.size()) {
		filter.reset(new osmpbf::ConstantReturnFilter(false));
		std::cout << "AreaExtractor::nameFilter: Warning: name-filter is empty" << std::endl;
		return filter;
	}
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

}//end namespace