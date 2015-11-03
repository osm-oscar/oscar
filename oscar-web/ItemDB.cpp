#include "ItemDB.h"
#include <iomanip>
#include <sserialize/Static/GeoWay.h>
#include <sserialize/Static/GeoPolygon.h>
#include <sserialize/Static/GeoMultiPolygon.h>
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <cppcms/json.h>
#include <cppcms/util.h>
#include <sserialize/utility/printers.h>
#include "BinaryWriter.h"

namespace oscar_web {

void ItemDB::writeHeader(std::ostream & out) {
	out << "[";
}

void ItemDB::writeFooter(std::ostream & out) {
	out << "]";
}

void toJson(std::ostream & out, sserialize::Static::spatial::GeoPolygon::const_iterator it, sserialize::Static::spatial::GeoPolygon::const_iterator end) {
	sserialize::Static::spatial::GeoPoint gp(*it);
	out << "[" << gp.lat() << "," << gp.lon() << "]";
	for(++it; it != end; ++it) {
		gp = *it;
		out << ",[" << gp.lat() << "," << gp.lon() << "]";
	}
}

void toJson(std::ostream & out, const sserialize::Static::spatial::GeoMultiPolygon::PolygonList & polys) {
	out << "[";
	if (polys.size()) {
		sserialize::Static::spatial::GeoMultiPolygon::PolygonList::const_iterator pIt(polys.cbegin()), pEnd(polys.cend());
		sserialize::Static::spatial::GeoMultiPolygon::PolygonList::const_reference gp(*pIt);
		out << "[";
		toJson(out, gp.cbegin(), gp.cend());
		out << "]";
		for(++pIt; pIt != pEnd; ++pIt) {
			out << ",[";
			gp = *pIt;
			toJson(out, gp.cbegin(), gp.cend());
			out << "]";
		}
	}
	out << "]";
}

void toJson(std::ostream & out, const sserialize::Static::spatial::GeoShape & gs) {
	sserialize::spatial::GeoShapeType gst = gs.type();
	out << "{\"t\":" << gst << ",\"v\":";
	switch(gst) {
	case sserialize::spatial::GS_POINT:
		{
			const sserialize::Static::spatial::GeoPoint * gp = gs.get<sserialize::Static::spatial::GeoPoint>();
			out << "[" << gp->lat() << "," << gp->lon() << "]";
		}
		break;
	case sserialize::spatial::GS_WAY:
	case sserialize::spatial::GS_POLYGON:
		{
			const sserialize::Static::spatial::GeoWay * gw = gs.get<sserialize::Static::spatial::GeoWay>();
			out << "[";
			if (gw->size()) {
				sserialize::Static::spatial::GeoWay::const_iterator it(gw->cbegin()), end(gw->cend());
				sserialize::Static::spatial::GeoPoint gp(*it);
				out << "[" << gp.lat() << "," << gp.lon() << "]";
				for(++it; it != end; ++it) {
					gp = *it;
					out << ",[" << gp.lat() << "," << gp.lon() << "]";
				}
			}
			out << "]";
		}
		break;
	case sserialize::spatial::GS_MULTI_POLYGON:
		{
			out << "{";
			const sserialize::Static::spatial::GeoMultiPolygon * gmw = gs.get<sserialize::Static::spatial::GeoMultiPolygon>();
			if (gmw->innerPolygons().size()) {
				out << "\"inner\":";
				toJson(out, gmw->innerPolygons());
				out << ",";
			}
			out << "\"outer\":";
			toJson(out, gmw->outerPolygons());
			out << "}";
		}
		break;
	default:
		break;
	}
	out << "}";
}

void ItemDB::writeSingleItem(std::ostream & out, uint32_t id, bool withShape) {
	liboscar::Static::OsmKeyValueObjectStore::Item item = m_store.at(id);
	liboscar::Static::OsmKeyValueObjectStorePayload payload(item.payload());
	sserialize::Static::spatial::GeoShape shape(payload.shape());
	if (shape.size() && item.strCount()) {
		out << "{";
		out << "\"id\":" << item.id() << ",";
		out << "\"osmid\":" << item.osmId() << ",";
		out << "\"type\":\"";
		switch (payload.type()) {

		case liboscar::OSMIT_NODE:
			out << "node";
			break;
		case liboscar::OSMIT_WAY:
			out << "way";
			break;
		case liboscar::OSMIT_RELATION:
			out << "relation";
			break;
		case liboscar::OSMIT_INVALID:
		default:
			out << "invalid";
			break;
		};
		out << "\",";
		out << "\"score\":" << payload.score() << ",";
		sserialize::spatial::GeoRect bbox(shape.boundary());
		out << "\"bbox\":[" << bbox.minLat() << "," << bbox.maxLat() << "," << bbox.minLon() << "," << bbox.maxLon() << "],";
		out << "\"shape\":";
		if (withShape) {
			toJson(out, shape);
		}
		else {
			out << "{\"t\":-1}";
		}
		if (item.size()) {
			out << ",\"k\":[\"" << m_escaper.escape(item.key(0)) << "\"";
			for(uint32_t i=1, s=item.size(); i < s; ++i) {
				out << ",\"" << m_escaper.escape(item.key(i)) << "\"";
			}
			out << "],\"v\":[\"" <<  m_escaper.escape(item.value(0)) << "\"";
			for(uint32_t i=1, s=item.size(); i < s; ++i) {
				out << ",\"" <<  m_escaper.escape(item.value(i)) << "\"";
			}
			out << "]";
		}
		out << "}";
	}
}

ItemDB::ItemDB(cppcms::service & srv, const liboscar::Static::OsmKeyValueObjectStore & store) :
cppcms::application(srv),
m_store(store),
m_maxPerRequest(10)
{
	dispatcher().assign<ItemDB>("/single/(\\d+)", &ItemDB::single, this, 1);
	mapper().assign("single","/single/{1}");
	dispatcher().assign<ItemDB>("/multiple",&ItemDB::multiple, this);
	mapper().assign("multiple","/multiple");
	dispatcher().assign<ItemDB>("/multipleshapes",&ItemDB::multipleShapes, this);
	mapper().assign("multipleshapes","/multipleshapes");
	dispatcher().assign<ItemDB>("/itemcells/(\\d+)", &ItemDB::itemCells, this, 1);
	mapper().assign("itemcells","/itemcells/{1}");
	dispatcher().assign<ItemDB>("/cellparents/(\\d+)", &ItemDB::cellParents, this, 1);
	mapper().assign("cellparents","/cellparents/{1}");
	dispatcher().assign<ItemDB>("/itemparents/(\\d+)", &ItemDB::itemParents, this, 1);
	mapper().assign("itemparents","/itemparents/{1}");
	dispatcher().assign<ItemDB>("/itemrelatives/(\\d+)", &ItemDB::itemRelatives, this, 1);
	mapper().assign("itemrelatives","/itemrelatives/{1}");
}

ItemDB::~ItemDB() {}

void ItemDB::single(std::string num) {
	response().set_content_header("text/json"); //content_type needs to be text/ in order for compression to work
	
	uint32_t id = atoi(num.c_str());
	std::ostream & out = response().out();
	if (m_store.size() > id) {
		writeSingleItem(out, id, true);
	}
}

void ItemDB::multiple() {
	response().set_content_header("text/json");

	std::vector<uint64_t> filteredRequestedItems;
	
	std::stringstream rawIdxIds;
	bool withShape = sserialize::toBool(request().post("shape"));
	rawIdxIds << request().post("which");
	cppcms::json::value jsonIdxIds;
	jsonIdxIds.load(rawIdxIds, true);
	
	uint32_t maxId = m_store.size();
	if (jsonIdxIds.type() == cppcms::json::is_array) {
		cppcms::json::array & arr = jsonIdxIds.array();
		if (arr.size() && arr.at(0).type() == cppcms::json::is_number) {
			filteredRequestedItems.reserve(arr.size());
			for(const cppcms::json::value & v : arr) {
				try {
					uint32_t tmp = v.get_value<double>();
					if (tmp < maxId) {
						filteredRequestedItems.push_back(tmp);
					}
				}
				catch(const cppcms::json::bad_value_cast & err) {}
			}
		}
	}
	
	std::ostream & out = response().out();
	
	if (withShape) {
		out << std::fixed << std::setprecision(std::numeric_limits<double>::digits10 + 2);
	}
	
	writeHeader(out);
	writeMultiple(out, filteredRequestedItems.begin(), filteredRequestedItems.end(), withShape);
	writeFooter(out);
}

void ItemDB::multipleShapes() {
	response().set_content_header("text/json");
	
	std::stringstream rawIdxIds;
	rawIdxIds << request().post("which");
	
	cppcms::json::value jsonIdxIds;
	jsonIdxIds.load(rawIdxIds, true);
	
	std::ostream & out = response().out();
	out << std::fixed << std::setprecision(std::numeric_limits<double>::digits10 + 2);
	
	
	out << "{";
	
	uint32_t maxId = m_store.size();
	if (jsonIdxIds.type() == cppcms::json::is_array) {
		cppcms::json::array & arr = jsonIdxIds.array();
		if (arr.size() && arr.at(0).type() == cppcms::json::is_number) {
			char sep = ' ';
			for(const cppcms::json::value & v : arr) {
				try {
					uint32_t tmp = v.get_value<double>();
					if (tmp < maxId) {
						out << sep << "\"" << tmp << "\":";
						sep = ',';
						toJson(out, m_store.geoShape(tmp));
					}
				}
				catch(const cppcms::json::bad_value_cast & err) {}
			}
		}
	}
	out << "}";
}


void ItemDB::multipleNames() {
	response().set_content_header("text/json");
	
	std::stringstream rawIdxIds;
	std::string lang = request().post("lang");
	rawIdxIds << request().post("which");
	
	uint32_t langNameId = sserialize::Static::StringTable::npos;
	
	
	cppcms::json::value jsonIdxIds;
	jsonIdxIds.load(rawIdxIds, true);

	
	std::ostream & out = response().out();
	out << "{";
	
	uint32_t maxId = m_store.size();
	if (jsonIdxIds.type() == cppcms::json::is_array) {
		cppcms::json::array & arr = jsonIdxIds.array();
		if (arr.size() && arr.at(0).type() == cppcms::json::is_number) {
			for(const cppcms::json::value & v : arr) {
				try {
					uint32_t tmp = v.get_value<double>();
					if (tmp < maxId) {
						out << tmp;
					}
				}
				catch(const cppcms::json::bad_value_cast & err) {}
			}
		}
	}
	out << "}";
}

void ItemDB::itemCells(std::string strId) {
	response().content_type("application/octet-stream");
	
	BinaryWriter bw(response().out());
	
	uint32_t itemId = atoi(strId.c_str());
	if (itemId >= m_store.size()) {
		bw.putU32(0);
		return;
	}
	liboscar::Static::OsmKeyValueObjectStore::Item item(m_store.at(itemId));
	auto c = item.cells();
	bw.putU32(c.size());
	for(uint32_t i(0), s(c.size()); i < s; ++i) {
		bw.putU32(c.at(i));
	}
}

void ItemDB::cellParents(std::string cellIdStr) {
	response().content_type("application/octet-stream");
	
	const sserialize::Static::spatial::GeoHierarchy & gh = m_store.geoHierarchy();
	
	BinaryWriter bw(response().out());
	
	uint32_t cellId = atoi(cellIdStr.c_str());
	if (cellId >= gh.cellSize()) {
		bw.putU32(0);
		return;
	}
	sserialize::Static::spatial::GeoHierarchy::Cell cell(gh.cell(cellId));
	bw.putU32(cell.parentsSize());
	for(uint32_t i(cell.parentsBegin()), s(cell.parentsEnd()); i < s; ++i) {
		bw.putU32(gh.ghIdToStoreId(gh.cellPtr(i)));
	}
}

void ItemDB::itemParents(std::string itemIdStr) {
	response().content_type("application/octet-stream");
	
	const sserialize::Static::spatial::GeoHierarchy & gh = m_store.geoHierarchy();
	
	BinaryWriter bw(response().out());
	
	uint32_t itemId = atoi(itemIdStr.c_str());
	if (itemId >= m_store.size()) {
		bw.putU32(0);
		return;
	}
	std::vector<uint32_t> parents;
	std::vector<uint32_t>::iterator pBegin, pEnd;
	if (itemId < gh.regionSize()) {
		std::unordered_set<uint32_t> seenRegionParents;
		parents.push_back(gh.storeIdToGhId(itemId));
		for(uint32_t i(0); i < parents.size(); ++i) {
			uint32_t curRegionId = parents[i];
			for(uint32_t pP(gh.regionParentsBegin(curRegionId)), sP(gh.regionParentsEnd(curRegionId)); pP < sP; ++pP) {
				uint32_t pId = gh.regionPtr(pP);
				if (!seenRegionParents.count(pId)) {
					seenRegionParents.insert(pId);
					parents.push_back(pId);
				}
			}
		}
		parents[0] = std::numeric_limits<uint32_t>::max();//gets rid of the item
		for(std::vector<uint32_t>::iterator it(parents.begin()+1), end(parents.end()); it != end; ++it) {
			*it = gh.ghIdToStoreId(*it);
		}
		std::sort(parents.begin(), parents.end());//unique by definition, due to hash
		pBegin = parents.begin();
		pEnd = parents.end()-1;
	}
	else {
		liboscar::Static::OsmKeyValueObjectStore::Item item(m_store.at(itemId));
		sserialize::BoundedCompactUintArray itemCells(item.cells());
		for(uint32_t cellId : itemCells) {
			sserialize::Static::spatial::GeoHierarchy::Cell cell(gh.cell(cellId));
			parents.reserve(parents.size()+cell.parentsSize());
			for(uint32_t i(cell.parentsBegin()), s(cell.parentsEnd()); i < s; ++i) {
				parents.push_back(gh.ghIdToStoreId(gh.cellPtr(i)));
			}
		}
		std::sort(parents.begin(), parents.end());
		pEnd = std::unique(parents.begin(), parents.end());
		pBegin = parents.begin();
	}
	bw.putU32(pEnd-pBegin);
	for(; pBegin != pEnd; ++pBegin) {
		bw.putU32(*pBegin);
	}
}

void ItemDB::itemRelatives(std::string itemIdStr) {
	response().content_type("application/octet-stream");
	
	const sserialize::Static::spatial::GeoHierarchy & gh = m_store.geoHierarchy();
	
	BinaryWriter bw(response().out());
	
	uint32_t itemId = atoi(itemIdStr.c_str());
	if (itemId >= m_store.size()) {
		bw.putU32(0);
		return;
	}
	std::vector<uint32_t> parents;
	std::vector<uint32_t>::iterator pBegin, pEnd;

	liboscar::Static::OsmKeyValueObjectStore::Item item(m_store.at(itemId));
	sserialize::BoundedCompactUintArray itemCells(item.cells());
	if (itemCells.size() > 10000) {
		std::unordered_set<uint32_t> seenRegions;
		for(uint32_t cellId : itemCells) {
			sserialize::Static::spatial::GeoHierarchy::Cell cell(gh.cell(cellId));
			for(uint32_t i(cell.parentsBegin()), s(cell.parentsEnd()); i < s; ++i) {
				uint32_t rId = gh.ghIdToStoreId(gh.cellPtr(i));
				if (!seenRegions.count(rId)) {
					seenRegions.insert(rId);
					parents.push_back(rId);
				}
			}
		}
		std::sort(parents.begin(), parents.end());
		pEnd = parents.end();
	}
	else {
		for(uint32_t cellId : itemCells) {
			sserialize::Static::spatial::GeoHierarchy::Cell cell(gh.cell(cellId));
			parents.reserve(parents.size()+cell.parentsSize());
			for(uint32_t i(cell.parentsBegin()), s(cell.parentsEnd()); i < s; ++i) {
				parents.push_back(gh.ghIdToStoreId(gh.cellPtr(i)));
			}
		}
		std::sort(parents.begin(), parents.end());
		pEnd = std::unique(parents.begin(), parents.end());
	}
	
	pBegin = parents.begin();
	bw.putU32(pEnd-pBegin);
	for(; pBegin != pEnd; ++pBegin) {
		bw.putU32(*pBegin);
	}
}



}//end namespace