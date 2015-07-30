#ifndef OSCAR_WEB_INDEX_DB_H
#define OSCAR_WEB_INDEX_DB_H
#include <cppcms/application.h>
#include <sserialize/Static/ItemIndexStore.h>
#include <sserialize/Static/GeoHierarchy.h>
#include <sserialize/storage/pack_unpack_functions.h>
#define OSCAR_WEB_INDEX_DB_VERSION 1

namespace oscar_web {

/** @brief This class gives access to the ItemIndexStore. It returns a binary representation of the requested ItemIndex an returns them as a special set
  * The binary format is given in the following (parse it with the accompanying java script library):
  * All integers are given in little-endian
  * -----------------------------------------------------------------------------
  * uint32_t|uint32_t |uint32_t   |uint32_t|(uint32_t|uint32_t|uint8_t|padding)|
  * -----------------------------------------------------------------------------
  * version |idxformat|compression|idxcount|(idx id  |datalen |idxdata|padto4 )|
  * 
  * 
  * idx formats as defined in sserialize::ItemIndex
  * compression formats as defined in sserialize::Static::ItemIndexStore
  * 
  * All requests are answered using this format, this also true for a single() request!
  *
  *
  * TODO:cppcms disable compressions for content_type != text/\*
  * 
  */


class IndexDB: public cppcms::application {
private:
	sserialize::Static::ItemIndexStore m_idxStore;
	sserialize::Static::spatial::GeoHierarchy m_gh;
	uint32_t m_maxPerRequest;
private:
	void writeSingleIndex(std::ostream & out, uint32_t id);
	template<typename T_IT>
	void writeMultipleIndexes(std::ostream & out, T_IT begin, T_IT end);
public:
	IndexDB(cppcms::service & srv, const sserialize::Static::ItemIndexStore & idxStore, const sserialize::Static::spatial::GeoHierarchy & gh);
	virtual ~IndexDB();
	inline void setMaxPerRequest(uint32_t v) { m_maxPerRequest = v; }
	inline uint32_t maxPerRequest() const { return m_maxPerRequest; }
	void single(std::string num);
	///by POST with variable which=[int] containing indexids
	void multiple();
	///by POST with variable which=[int] containing cellids, returning (uint32_t)-Array, 0xFFFFFFFF if cell id was invalid
	void cellIndexIds();
	static void writeHeader(std::ostream & out, sserialize::ItemIndex::Types indexType, sserialize::Static::ItemIndexStore::IndexCompressionType indexCompression, uint32_t count);
	///T_OSTREAM needs a function write(const char *, size_t size)
	template<typename T_OSTREAM>
	static void writeIndexData(T_OSTREAM & out, uint32_t id, const sserialize::UByteArrayAdapter::MemoryView & memv);
};

template<typename T_OSTREAM>
void IndexDB::writeIndexData(T_OSTREAM & out, uint32_t id, const sserialize::UByteArrayAdapter::MemoryView & memv) {
	uint32_t memvSize = memv.size();
	
	assert(memvSize);
	
	char buf[sizeof(uint32_t)];
	
	//write idxid
	uint32_t tmp = id;
	tmp = htole32(tmp);
	memmove(buf, &tmp, sizeof(uint32_t));
	out.write(buf, sizeof(uint32_t));
	
	//write idx data len
	tmp = memvSize;
	tmp = htole32(tmp);
	memmove(buf, &tmp, sizeof(uint32_t));
	out.write(buf, sizeof(uint32_t));
	
	//write idx data
	out.write((const char*)memv.get(), memvSize);
	
	//padding
	if (memvSize%4 != 0) {
		tmp=0;
		out.write((char*)&tmp, 4-(memvSize%4));
	}
}

template<typename T_IT>
void IndexDB::writeMultipleIndexes(std::ostream & out, T_IT begin, T_IT end) {
	uint32_t size = end-begin;
	if (size > m_maxPerRequest) {
		size = m_maxPerRequest;
	}
	writeHeader(out, m_idxStore.indexType(), m_idxStore.compressionType(), size);
	for(uint32_t i(0); i < size; ++i, ++begin) {
		writeSingleIndex(out, *begin);
	}
}

}//end namespace

#endif