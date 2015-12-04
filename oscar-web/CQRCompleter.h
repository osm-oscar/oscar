#ifndef OSCAR_WEB_CQR_COMPLETER_H
#define OSCAR_WEB_CQR_COMPLETER_H
#include <cppcms/application.h>
#include <sserialize/utility/debug.h>
#include "types.h"
#include "GeoHierarchySubSetSerializer.h"
#include "CellQueryResultsSerializer.h"

namespace sserialize {
class TimeMeasurer;
}

namespace oscar_web {

/**
  *This is the main query completer.
  */

class CQRCompleter: public cppcms::application {
private:
	CompletionFileDataPtr m_dataPtr;
	GeoHierarchySubSetSerializer m_subSetSerializer;
	CellQueryResultsSerializer m_cqrSerializer;
private:
	void writeSubSet(std::ostream & out, const std::string & sst, const sserialize::Static::spatial::GeoHierarchy::SubSet & subSet);
	void writeLogStats(const std::string& query, const sserialize::TimeMeasurer& tm, uint32_t cqrSize, uint32_t idxSize);
public:
	CQRCompleter(cppcms::service& srv, const CompletionFileDataPtr & dataPtr);
	virtual ~CQRCompleter();
	/**
	  * q=<searchstring>
	  * sst=(flatxml|treexml|flatjson)
	  * Return:
	  * CellQueryResultsSerializer.write|SubSet as xml/json/binary
	  */
	void fullCQR();
	/** Returns a binary array of uint32_t with the ids of k items skipping o of the selected regions and the direct children of the region
	  * if openhints are requests then two additional arrays are present containing the path of the openhint and all seen regions on the path
	  * query:
	  * q=<searchstring>
	  * oh=<float between 0..1.0 defining the termination fraction when the opening-path stops>
	  * oht=(global|relative) theshold type: global compares with the global item count, relative with the parent item count
	  * Return:
	  * array<uint32_t>(regionids): region ids of the opening-path calculated if oh option was given
	  * array<uint32_t|array<(uint32_t|uint32_t)> region child info [regionid, [(region children|region children maxitems)] encoded in a single array<uint32_t>
	  */
	void simpleCQR();
	/** returns the top-k items for the query q:
	  * q=<searchstring>
	  * k=<number_of_items>
	  * o=<offset_in_items_result_list>
	  * r=<region_id>
	  * Return:
	  * array<uint32_t> : [item id]
	  */
	void items();
	/** return the region children for the query q:
	  * q=<searchstring>
	  * r=<regionid>
	  * Return:
	  * array<(uint32_t, uint32_t)>: [(region children|region children maxitems)]
	  */
	void children();
	/** return the maximum set of independet region children for the query q:
	  * q=<searchstring>
	  * r=<regionid>
	  * Return:
	  * array<uint32_t>: [regionId]
	  */
	void maximumIndependentChildren();
};

}//end namespace

#endif