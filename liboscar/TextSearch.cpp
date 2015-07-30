#include <liboscar/TextSearch.h>
#include "CellTextCompleter.h"
#include <sserialize/Static/StringCompleter.h>
#include <sserialize/search/StringCompleterPrivateGeoHierarchyUnclustered.h>

namespace liboscar {

TextSearch::TextSearch(const sserialize::UByteArrayAdapter& d, const sserialize::Static::ItemIndexStore & indexStore, const sserialize::Static::spatial::GeoHierarchy & gh, const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement & ra) {
	SSERIALIZE_VERSION_MISSMATCH_CHECK(LIBOSCAR_TEXT_SEARCH_VERSION, d.at(0), "liboscar::TextSearch");
	
	sserialize::Static::Array<sserialize::UByteArrayAdapter> tmp(d+1);
	for(uint32_t i = 0, s = tmp.size(); i < s; ++i) {
		sserialize::UByteArrayAdapter td(tmp.at(i));
		switch (td.at(0)) {
		case ITEMS:
			m_completers[ITEMS].emplace_back(new sserialize::Static::StringCompleter(td+1, indexStore));
			break;
		case GEOHIERARCHY:
			m_completers[GEOHIERARCHY].emplace_back(new sserialize::Static::StringCompleter(td+1, indexStore));
			break;
		case GEOHIERARCHY_AND_ITEMS:
			{
				typedef sserialize::Static::detail::StringCompleter::GeoHierarchyUnclustered MyC;
				MyC * tmp = new MyC(gh, indexStore, td+1);
				m_completers[ITEMS].emplace_back(tmp);
				m_completers[ITEMS].emplace_back(tmp->itemsCompleter().get());
				m_completers[GEOHIERARCHY_AND_ITEMS].emplace_back(tmp);
				m_completers[GEOHIERARCHY].emplace_back(tmp->ghCompleter().get());
			}
			break;
		case GEOCELL:
			{
				sserialize::RCPtrWrapper<Static::detail::CellTextCompleter> cellTextCompleter(new Static::detail::CellTextCompleter(td+1, indexStore, gh, ra));
				m_completers[GEOCELL].emplace_back(cellTextCompleter.priv());
				m_completers[ITEMS].emplace_back(new Static::CellTextCompleterUnclustered(Static::CellTextCompleter(cellTextCompleter), gh));
			}
			break;
		default:
			throw sserialize::CorruptDataException("liboscar::TextSearch: invalid text search type");
		};
		
	}
}


}//end namespace