#include "StaticOsmCompleter.h"
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include "constants.h"
#include <istream>
#include <fstream>
#include "SetOpTreePrivateGeo.h"
#include "tagcompleters.h"
#include "CellOpTree.h"
#include "AdvancedCellOpTree.h"
#include <sserialize/search/StringCompleterPrivateMulti.h>
#include <sserialize/search/StringCompleterPrivateGeoHierarchyUnclustered.h>
#include <sserialize/Static/StringCompleter.h>
#include <sserialize/Static/GeoCompleter.h>
#include <sserialize/storage/MmappedFile.h>
#include <sserialize/stats/TimeMeasuerer.h>
#ifdef __ANDROID__
	#define MEMORY_BASED_SUBSET_CREATOR_MIN_CELL_COUNT static_cast<uint32_t>(0xFFFFFFFFF)
#else
	#define MEMORY_BASED_SUBSET_CREATOR_MIN_CELL_COUNT static_cast<uint32_t>(100)
#endif

namespace liboscar {
namespace Static {

std::ostream & OsmCompleter::printStats(std::ostream & out) const {
	m_tagCompleter->tagStore().printStats(out);
	return out;
}

OsmCompleter::OsmCompleter() :
m_selectedGeoCompleter(0)
{}

OsmCompleter::~OsmCompleter() {}


bool OsmCompleter::setAllFilesFromPrefix(const std::string & filePrefix) {
	if (sserialize::MmappedFile::isDirectory(filePrefix)) {
		m_filesDir = filePrefix;
		return true;
	}
	return false;
}

bool OsmCompleter::setGeoCompleter(uint8_t pos) {
	if (pos < m_geoCompleters.size()) {
		m_selectedGeoCompleter = pos;
		return true;
	}
	return false;
}

bool OsmCompleter::setTextSearcher(TextSearch::Type t, uint8_t pos) {
	return m_textSearch.select(t, pos);
}

sserialize::StringCompleter OsmCompleter::getItemsCompleter() const {
	sserialize::StringCompleter strCmp;
	if (m_data.count(FC_TAGSTORE)) {
		sserialize::StringCompleterPrivateMulti * myscmp = new sserialize::StringCompleterPrivateMulti();
		myscmp->addCompleter( sserialize::RCPtrWrapper<sserialize::StringCompleterPrivate>(textSearch().get<liboscar::TextSearch::ITEMS>().getPrivate()) );
		myscmp->addCompleter( sserialize::RCPtrWrapper<sserialize::StringCompleterPrivate>(new StringCompleterPrivateTagPhrase(m_tagPhraseCompleter)) );
		strCmp = sserialize::StringCompleter(myscmp);
	}
	else {
		strCmp = textSearch().get<liboscar::TextSearch::Type::ITEMS>();
	}
	return strCmp;
}


OsmItemSet OsmCompleter::complete(const std::string & query) {
	OsmItemSet itemSet(query, getItemsCompleter(), store(), sserialize::SetOpTree::SOT_COMPLEX);
	registerFilters(itemSet);
	itemSet.execute();
	return itemSet;
}

OsmItemSet OsmCompleter::simpleComplete(const std::string & query, uint32_t maxResultSetSize, uint32_t minStrLen) {
	OsmItemSet itemSet(query, getItemsCompleter(), store(), sserialize::SetOpTree::SOT_SIMPLE);
	registerFilters(itemSet);
	itemSet.setMaxResultSetSize(maxResultSetSize);
	itemSet.setMinStrLen(minStrLen);
	itemSet.execute();
	return itemSet;
}

OsmItemSet OsmCompleter::simpleComplete(const std::string & query, uint32_t maxResultSetSize, uint32_t minStrLen, const sserialize::spatial::GeoRect & rect) {
	sserialize::StringCompleter strCmp(getItemsCompleter());
	std::shared_ptr<sserialize::ItemIndex::ItemFilter> geoFilter(new GeoConstraintFilter<liboscar::Static::OsmKeyValueObjectStore>(store(), rect));
	if (rect.length() < 0.1) {
		OsmItemSet itemSet(sserialize::toString(query," $GEO[",rect.minLat(),";",rect.maxLat(),";",rect.minLon(),";",rect.maxLon(),";]"), store(), sserialize::SetOpTree(new SetOpTreePrivateGeo(geoFilter)));
		itemSet.registerStringCompleter(strCmp);
		registerFilters(itemSet);
		itemSet.setMaxResultSetSize(maxResultSetSize);
		itemSet.setMinStrLen(minStrLen);
		itemSet.execute();
		return itemSet;
	}
	else {
		OsmItemSet itemSet(query, store(), sserialize::SetOpTree(new SetOpTreePrivateGeo(geoFilter)));
		itemSet.registerStringCompleter(strCmp);
		registerFilters(itemSet);
		itemSet.setMaxResultSetSize(maxResultSetSize);
		itemSet.setMinStrLen(minStrLen);
		itemSet.execute();
		return itemSet;
	}
}

OsmItemSetIterator OsmCompleter::partialComplete(const std::string & query, const sserialize::spatial::GeoRect & rect) {
	sserialize::StringCompleter strCmp(getItemsCompleter());
	std::shared_ptr<sserialize::ItemIndex::ItemFilter> geoFilter(new GeoConstraintFilter<liboscar::Static::OsmKeyValueObjectStore>(store(), rect));
	if (rect.length() < 0.1) {
		OsmItemSetIterator itemSet(sserialize::toString(query," $GEO[",rect.minLat(),";",rect.maxLat(),";",rect.minLon(),";",rect.maxLon(),";]"), store(), sserialize::SetOpTree::SOT_COMPLEX, geoFilter);
		itemSet.registerStringCompleter(strCmp);
		registerFilters(itemSet);
		itemSet.execute();
		return itemSet;
	}
	else {
		OsmItemSetIterator itemSet(query, store(), sserialize::SetOpTree::SOT_COMPLEX, geoFilter);
		itemSet.registerStringCompleter(strCmp);
		registerFilters(itemSet);
		itemSet.execute();
		return itemSet;
	}
}

void OsmCompleter::energize() {
	for(uint32_t i = FC_BEGIN; i < FC_END; ++i) {
		bool cmp;
		std::string fn;
		if (fileNameFromPrefix(m_filesDir, (FileConfig)i, fn, cmp)) {
			m_data[i] = sserialize::UByteArrayAdapter::openRo(fn, cmp, MAX_SIZE_FOR_FULL_MMAP, 0);
		}
	}
	
	bool haveNeededData = m_data.count(FC_KV_STORE);
	if (haveNeededData) {
		try {
			m_store = liboscar::Static::OsmKeyValueObjectStore(m_data[FC_KV_STORE]);
		}
		catch( sserialize::Exception & e) {
			sserialize::err("Static::OsmCompleter", std::string("Failed to initialize kvstore with the following error:\n") + std::string(e.what()));
			haveNeededData = false;
		}
		m_geoCompleters.push_back(
			sserialize::RCPtrWrapper<sserialize::SetOpTree::SelectableOpFilter>(
				new sserialize::spatial::GeoConstraintSetOpTreeSF<sserialize::GeoCompleter>(
					sserialize::Static::GeoCompleter::fromDB(m_store)
				)
			)
		);
	}
	if (!haveNeededData) {
		throw sserialize::MissingDataException("OsmCompleter needs a KeyValueStore");
	}
	
	haveNeededData = m_data.count(FC_INDEX);
	if (haveNeededData) {
		try {
			m_indexStore = sserialize::Static::ItemIndexStore(m_data[FC_INDEX]);
		}
		catch ( sserialize::Exception & e) {
			sserialize::err("liboscar::Static::OsmCompleter", std::string("Failed to initialize index with the following error:\n") + e.what());
			haveNeededData = false;
		}
	}
	if(!haveNeededData) {
		std::cout << "OsmCompleter: No index available" << std::endl;
		return;
	}

	if (m_data.count(FC_TEXT_SEARCH)) {
		try {
			m_textSearch = liboscar::TextSearch(m_data[FC_TEXT_SEARCH], m_indexStore, m_store.geoHierarchy(), m_store.regionArrangement());
		}
		catch (sserialize::Exception & e) {
			sserialize::err("liboscar::Static::OsmCompleter", std::string("Failed to initialize textsearch with the following error:\n") + e.what());
		}
	}
	
	if (m_data.count(FC_GEO_SEARCH)) {
		try {
			m_geoSearch = liboscar::GeoSearch(m_data[FC_GEO_SEARCH], m_indexStore, m_store);
			if (m_geoSearch.hasSearch(liboscar::GeoSearch::ITEMS)) {
				for(const auto & x : m_geoSearch.get(liboscar::GeoSearch::ITEMS)) {
					m_geoCompleters.push_back(
						sserialize::RCPtrWrapper<sserialize::SetOpTree::SelectableOpFilter>(
							new sserialize::spatial::GeoConstraintSetOpTreeSF<sserialize::GeoCompleter>(x)
						)
					);
				}
			}
		}
		catch (sserialize::Exception & e) {
			sserialize::err("liboscar::Static::OsmCompleter", std::string("Failed to initialize geosearch with the following error:\n") + e.what());
		}
	}
	
	if (m_data.count(FC_TAGSTORE)) {
		try {
			TagStore tagStore(m_data[FC_TAGSTORE], m_indexStore);
			m_tagCompleter = sserialize::RCPtrWrapper<TagCompleter>( new TagCompleter(tagStore) );
			m_tagNameCompleter = sserialize::RCPtrWrapper<TagNameCompleter>( new TagNameCompleter(tagStore) );
			std::string tagStorePhrasesFn;
			bool cmp;
			if (fileNameFromPrefix(m_filesDir, FC_TAGSTORE_PHRASES, tagStorePhrasesFn, cmp) && !cmp) {
				std::ifstream iFile;
				iFile.open(tagStorePhrasesFn);
				m_tagPhraseCompleter = sserialize::RCPtrWrapper<TagPhraseCompleter>( new TagPhraseCompleter(tagStore, iFile) );
				iFile.close();
			}
			else {
				m_tagPhraseCompleter = sserialize::RCPtrWrapper<TagPhraseCompleter>( new TagPhraseCompleter() );
			}
		}
		catch (sserialize::Exception & e) {
			sserialize::err("liboscar::Static::OsmCompleter", std::string("Failed to initialize tagstore with the following error:\n") + e.what());
		}
	}
	else {
		m_tagCompleter = sserialize::RCPtrWrapper<TagCompleter>( new TagCompleter() );
		m_tagNameCompleter = sserialize::RCPtrWrapper<TagNameCompleter>( new TagNameCompleter() );
		m_tagPhraseCompleter = sserialize::RCPtrWrapper<TagPhraseCompleter>( new TagPhraseCompleter() );
	}

	if (m_store.geoHierarchy().cellSize() >= MEMORY_BASED_SUBSET_CREATOR_MIN_CELL_COUNT) {
		m_ghs = sserialize::spatial::GeoHierarchySubSetCreator(m_store.geoHierarchy());
	}
}

void processCompletionToken(std::string & q, sserialize::StringCompleter::QuerryType & qt) {
	qt = sserialize::StringCompleter::normalize(q);
}

inline std::ostream & operator<<(std::ostream & out, const liboscar::Static::OsmKeyValueObjectStore::Item & item) {
	item.print(out, false);
	return out;
}

sserialize::CellQueryResult OsmCompleter::cqrComplete(const std::string& query, bool treedCQR) {
	if (!m_textSearch.hasSearch(liboscar::TextSearch::Type::GEOCELL)) {
		std::cout << "No support fo clustered completion available" << std::endl;
		return sserialize::CellQueryResult();
	}
	sserialize::Static::CellTextCompleter cmp( m_textSearch.get<liboscar::TextSearch::Type::GEOCELL>() );
	
	std::cout << "BEGIN: CLUSTERED COMPLETE" << std::endl;
	
	sserialize::CellQueryResult r;
	
	sserialize::TimeMeasurer tm;
	tm.begin();
	sserialize::Static::CQRDilator cqrd(store().cellCenterOfMass(), store().cellGraph());
	CQRFromPolygon cqrfp(store(), indexStore());
	sserialize::spatial::GeoHierarchySubSetCreator ghs(store().geoHierarchy(), sserialize::spatial::GeoHierarchySubSetCreator::T_PASS_THROUGH);
	CQRFromComplexSpatialQuery csq(ghs, cqrfp);
	if (!treedCQR) {
// 		CellOpTree<sserialize::CellQueryResult> opTree(cmp, true);
		AdvancedCellOpTree opTree(cmp, cqrd, csq);
		opTree.parse(query);
		r = opTree.calc<sserialize::CellQueryResult>();
	}
	else {
// 		CellOpTree<sserialize::TreedCellQueryResult> opTree(cmp, true);
		AdvancedCellOpTree opTree(cmp, cqrd, csq);
		opTree.parse(query);
		r = opTree.calc<sserialize::TreedCellQueryResult>().toCQR();
	}
	tm.end();
	std::cout << "Completion of " << query << " took " << tm.elapsedUseconds() << " usecs" << std::endl;
	#ifdef DEBUG_CHECK_ALL
	std::cout << r << std::endl;
	#endif
	return r;
}

sserialize::Static::spatial::GeoHierarchy::SubSet
OsmCompleter::clusteredComplete(const std::string& query, const sserialize::spatial::GeoHierarchySubSetCreator & ghs, uint32_t minCq4SparseSubSet, bool treedCQR) {
	sserialize::CellQueryResult r = cqrComplete(query, treedCQR);
	return ghs.subSet(r, r.cellCount() > minCq4SparseSubSet);
}

sserialize::Static::spatial::GeoHierarchy::SubSet OsmCompleter::clusteredComplete(const std::string& query, uint32_t minCq4SparseSubSet, bool treedCQR) {
	sserialize::CellQueryResult r = cqrComplete(query, treedCQR);
	const sserialize::Static::spatial::GeoHierarchy * gh = &store().geoHierarchy();
	
	if (m_store.geoHierarchy().cellSize() >= MEMORY_BASED_SUBSET_CREATOR_MIN_CELL_COUNT) {
		return m_ghs.subSet(r, r.cellCount() > minCq4SparseSubSet);
	}
	else {
		return gh->subSet(r, false);
	}
}

TagStore OsmCompleter::tagStore() const {
	if (m_tagCompleter.priv()) {
		return m_tagCompleter->tagStore();
	}
	return TagStore();
}


}}//end namespace