#include "IndexDB.h"
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <cppcms/json.h>
#include <sserialize/utility/printers.h>
#include <sserialize/storage/pack_unpack_functions.h>

namespace oscar_web {

IndexDB::IndexDB(cppcms::service& srv, const sserialize::Static::ItemIndexStore& idxStore, const sserialize::Static::spatial::GeoHierarchy & gh) :
cppcms::application(srv),
m_idxStore(idxStore),
m_gh(gh)
{
	dispatcher().assign<IndexDB>("/single/(\\d+)", &IndexDB::single, this, 1);
	mapper().assign("single","/single/{1}");
	dispatcher().assign("/multiple",&IndexDB::multiple, this);
	mapper().assign("multiple","/multiple");
	dispatcher().assign("/cellindexids",&IndexDB::cellIndexIds, this);
	mapper().assign("cellindexids","/cellindexids");
}

IndexDB::~IndexDB() {}

void IndexDB::writeSingleIndex(std::ostream & out, uint32_t id) {
	writeIndexData(out, id, m_idxStore.rawDataAt(id).asMemView());
}

void IndexDB::writeHeader(std::ostream& out, sserialize::ItemIndex::Types indexType, sserialize::Static::ItemIndexStore::IndexCompressionType indexCompression, uint32_t count) {
	char buf[sizeof(uint32_t)];
	
	//version
	uint32_t tmp = 1;
	tmp = htole32(tmp);
	memmove(buf, &tmp, sizeof(uint32_t));
	out.write(buf, sizeof(uint32_t));
	
	//index format
	tmp = indexType;
	tmp = htole32(tmp);
	memmove(buf, &tmp, sizeof(uint32_t));
	out.write(buf, sizeof(uint32_t));
	
	//compression format
	tmp = indexCompression;
	tmp = htole32(tmp);
	memmove(buf, &tmp, sizeof(uint32_t));
	out.write(buf, sizeof(uint32_t));
	
	//index count
	tmp = count;
	tmp = htole32(tmp);
	memmove(buf, &tmp, sizeof(uint32_t));
	out.write(buf, sizeof(uint32_t));
}

void IndexDB::single(std::string num) {
	//now do the respone
	response().set_header("Access-Control-Allow-Origin","*");
	response().content_type("application/octet-stream");
	
	uint32_t idxId = atoi(num.c_str());
	std::ostream & out = response().out();
	if (m_idxStore.size() > idxId) {
		writeMultipleIndexes(out, &idxId, (&idxId)+1);
	}
	else {
		typedef uint32_t * MyDummyIt;
		writeMultipleIndexes(out, MyDummyIt(0), MyDummyIt(0));
	}
}

void IndexDB::multiple() {
	//now do the respone
	response().set_header("Access-Control-Allow-Origin","*");
	response().content_type("application/octet-stream");
	
	std::vector<uint64_t> filteredRequestedIndices;

	cppcms::json::value jsonIdxIds;
	{
		std::stringstream tmp;
		tmp << request().post("which");
		jsonIdxIds.load(tmp, true);
	}
	
	uint32_t maxId = m_idxStore.size();
	if (jsonIdxIds.type() == cppcms::json::is_array) {
		cppcms::json::array & arr = jsonIdxIds.array();
		if (arr.size() && arr.at(0).type() == cppcms::json::is_number) {
			filteredRequestedIndices.reserve(arr.size());
			for(const cppcms::json::value & v : arr) {
				try {
					uint32_t tmp = v.get_value<double>();
					if (tmp < maxId) {
						filteredRequestedIndices.push_back(tmp);
					}
				}
				catch(const cppcms::json::bad_value_cast & err) {}
			}
		}
	}
	writeMultipleIndexes(response().out(), filteredRequestedIndices.begin(), filteredRequestedIndices.end());
}

void IndexDB::cellIndexIds() {
	//now do the respone
	response().set_header("Access-Control-Allow-Origin","*");
	response().content_type("application/octet-stream");
	
	cppcms::json::value jsonCellIds;
	{
		std::stringstream tmp;
		tmp << request().post("which");
		jsonCellIds.load(tmp, true);
	}
	
	uint32_t cellSize = m_gh.cellSize();
	std::ostream & out = response().out();
	
	char buf[sizeof(uint32_t)];
	auto writeU32 = [&buf, &out](uint32_t s) {
		s = htole32(s);
		memmove(buf, &s, sizeof(uint32_t));
		out.write(buf, sizeof(uint32_t));
	};
	
	if (jsonCellIds.type() == cppcms::json::is_array) {
		cppcms::json::array & arr = jsonCellIds.array();
		if (arr.size() && arr.at(0).type() == cppcms::json::is_number) {
			auto it(arr.begin()), end(arr.end());
			for(; it != end; ++it) {
				try {
					uint32_t tmp = it->get_value<double>();
					writeU32(tmp < cellSize ? m_gh.cellItemsPtr(tmp) : 0xFFFFFFFF);
				}
				catch(const cppcms::json::bad_value_cast & err) {}
			}
			
			for(; it != end; ++it) {
				try {
					uint32_t tmp = it->get_value<double>();
					writeU32(tmp < cellSize ? m_gh.cellItemsPtr(tmp) : 0xFFFFFFFF);
				}
				catch(const cppcms::json::bad_value_cast & err) {}
			}
		}
	}
	
}


}//end namespace
