#ifndef OSCAR_CREATE_AREA_EXTRACTOR_H
#define OSCAR_CREATE_AREA_EXTRACTOR_H
#include <osmpbf/filter.h>

namespace oscar_create {

struct AreaExtractor {
	static generics::RCPtr<osmpbf::AbstractTagFilter> nameFilter(const std::string & keysDefiningRegionsFile, const std::string & keyValuesDefiningRegionsFile);
};

}


#endif
