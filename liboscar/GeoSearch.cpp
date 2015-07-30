#include <liboscar/GeoSearch.h>
#include <sserialize/Static/StringCompleter.h>
#include <sserialize/Static/GeoCompleter.h>

namespace liboscar {

GeoSearch::GeoSearch(const sserialize::UByteArrayAdapter& d, const sserialize::Static::ItemIndexStore & indexStore, const Static::OsmKeyValueObjectStore & store) {
	SSERIALIZE_VERSION_MISSMATCH_CHECK(LIBOSCAR_GEO_SEARCH_VERSION, d.at(0), "liboscar::TextSearch");
	
	sserialize::Static::Array<sserialize::UByteArrayAdapter> tmp(d+1);
	for(uint32_t i = 0, s = tmp.size(); i < s; ++i) {
		sserialize::UByteArrayAdapter td(tmp.at(i));
		switch (td.at(0)) {
		case ITEMS:
			m_completers[ITEMS].push_back( sserialize::Static::GeoCompleter::fromData(td+1, indexStore, store) );
			break;
		default:
			throw sserialize::CorruptDataException("liboscar::GeoSearch: invalid text search type");
		};
	}
}


}//end namespace