#ifndef OSCAR_WEB_CELL_QUERY_RESULT_SERIALIZER_H
#define OSCAR_WEB_CELL_QUERY_RESULT_SERIALIZER_H
#include <sserialize/spatial/CellQueryResult.h>

namespace oscar_web {

//Due to the asychronicity we need:
//Set of ItemIndex
//We can recycle this for the fetched indices
/** Serializes a CellQueryResult to be send to a webbrowser and parsed by the accompanied javascript library
  *
  * This is a pure binary format:
  * 
  * ----------------------------------------------------------------------------------------------------
  * CellCount|(CellId|FullIndex|Fetched)*CellCount|(FetchedIndices)     |count |(PartialIndexIds)*count
  * ---------------------------------------------------------------------------------------------------
  * uint32   |(30    |    1    |    1  )          |IndexSet from IndexDB|uint32|(uint32)
  */

class CellQueryResultsSerializer {
private:
	sserialize::ItemIndex::Types m_idxType;
public:
	CellQueryResultsSerializer(sserialize::ItemIndex::Types t);
	~CellQueryResultsSerializer();
	void write(std::ostream & out, const sserialize::CellQueryResult & cqr) const;
	void writeReduced(std::ostream & out, const sserialize::CellQueryResult & cqr) const;
};

}//end namespace

#endif