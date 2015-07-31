#ifndef OSCAR_CREATE_AREA_EXTRACTOR_H
#define OSCAR_CREATE_AREA_EXTRACTOR_H
#include <osmtools/AreaExtractor.h>
#include "OsmPolygonStore.h"

namespace oscar_create {

class AreaExtractor {
	osmtools::AreaExtractor m_ae;
public:
	AreaExtractor();
	virtual ~AreaExtractor();
	bool extract(const std::string & file, const generics::RCPtr<osmpbf::AbstractTagFilter> & regionFilter, oscar_create::OsmPolygonStore & polystore, uint32_t numThreads, uint32_t maxNodeTableSize);
	bool extract(const std::string & file, oscar_create::OsmPolygonStore & polystore);
	static generics::RCPtr<osmpbf::AbstractTagFilter> nameFilter(const std::string & keysDefiningRegionsFile, const std::string & keyValuesDefiningRegionsFile);
};

}


#endif