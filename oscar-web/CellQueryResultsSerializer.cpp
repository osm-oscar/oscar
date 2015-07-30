#include "CellQueryResultsSerializer.h"
#include "IndexDB.h"
#include "BinaryWriter.h"
#include <sserialize/containers/ItemIndexFactory.h>
#include <sstream>

namespace oscar_web {

CellQueryResultsSerializer::CellQueryResultsSerializer(sserialize::ItemIndex::Types t) : m_idxType(t) {}

CellQueryResultsSerializer::~CellQueryResultsSerializer() {}

struct MyOstream {
	std::vector<char> data;
	void write(const char * src, uint32_t size) {
		data.insert(data.end(), src, src+size);
	}
};

void CellQueryResultsSerializer::write(std::ostream& out, const sserialize::CellQueryResult& cqr) const {
	MyOstream fetchedIndices;
	std::vector<uint32_t> partialIdxIds;
	BinaryWriter bout(out);
	uint32_t numFetchedIndices = 0;
	
	sserialize::ItemIndex::Types cqrIdxType = cqr.defaultIndexType();
	
	bout.putU32(cqr.cellCount());//CellCount
	for(sserialize::CellQueryResult::const_iterator it(cqr.cbegin()), end(cqr.cend()); it != end; ++it) {
		uint32_t rawDesc = it.rawDesc();
		if (!(rawDesc & sserialize::CellQueryResult::const_iterator::RD_FULL_MATCH))  {
			if (rawDesc & sserialize::CellQueryResult::const_iterator::RD_FETCHED) {
				const sserialize::ItemIndex & idx = (*it);
				assert(idx.type() == cqrIdxType);
				IndexDB::writeIndexData(fetchedIndices, 0xFFFFFFFF, idx.data().asMemView());
				++numFetchedIndices;
			}
			else {
				partialIdxIds.push_back(it.idxId());
			}
		}
		bout.putU32(rawDesc);//CellInfo
	}
	//write the ItemIndexSet header
	IndexDB::writeHeader(out, cqrIdxType, sserialize::Static::ItemIndexStore::IndexCompressionType::IC_NONE, numFetchedIndices);
	//and the index data
	out.write(&(fetchedIndices.data[0]), fetchedIndices.data.size());
	bout.putU32(partialIdxIds.size());//cellIdxCount
	for(uint32_t x : partialIdxIds) {//cellIdxIds
		bout.putU32(x);
	}
}

void CellQueryResultsSerializer::writeReduced(std::ostream& out, const sserialize::CellQueryResult& cqr) const {
	std::vector<uint32_t> cellIds;
	cellIds.reserve(cqr.cellCount());
	for(sserialize::CellQueryResult::const_iterator it(cqr.cbegin()), end(cqr.cend()); it != end; ++it) {
		cellIds.push_back(it.cellId());
	}
	IndexDB::writeHeader(out, m_idxType, sserialize::Static::ItemIndexStore::IndexCompressionType::IC_NONE, 1);
	std::vector<uint8_t> tmp;
	sserialize::UByteArrayAdapter d(sserialize::UByteArrayAdapter::createCache(1, sserialize::MM_PROGRAM_MEMORY));
	sserialize::ItemIndexFactory::create(cellIds, d, m_idxType);
	IndexDB::writeIndexData(out, std::numeric_limits<uint32_t>::max(), d.asMemView());
}


}//end namespace