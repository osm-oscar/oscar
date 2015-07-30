#include "tagcompleters.h"
#include <sserialize/strings/stringfunctions.h>
#include <sserialize/strings/unicode_case_functions.h>
#include <sserialize/utility/log.h>

namespace liboscar {
namespace Static {

TagCompleter::TagCompleter() {}
TagCompleter::TagCompleter(const Static::TagStore& tagStore) :
ExternalFunctoid(),
m_tagStore(tagStore)
{}

TagCompleter::~TagCompleter() {}

const std::string TagCompleter::cmdString() const {
	return std::string("i");
}

sserialize::ItemIndex TagCompleter::operator()(const std::string& queryString) {
	if (queryString.size() > 0) {
		std::string curStr;
		std::vector< uint16_t > nodeIds;
		std::string::const_iterator qStrIt = queryString.begin();
		while (qStrIt != queryString.end()) {
			
			if (*qStrIt == ';') {
				if (curStr.size() > 0) {
					uint16_t n = atoi(curStr.c_str());
					nodeIds.push_back(n);
					curStr.clear();
				}
			}
			else {
				curStr += *qStrIt;
			}
			qStrIt++;
		}
		if (curStr.size() > 0) {
			uint16_t n = atoi(curStr.c_str());
			nodeIds.push_back(n);
		}
		std::vector<sserialize::ItemIndex> indices;
		sserialize::ItemIndex idx;
		for(size_t i = 0; i < nodeIds.size(); i++) {
			idx = m_tagStore.complete(nodeIds[i]);
			if (idx.size() > 0) {
				indices.push_back(idx);
			}
		}
		return sserialize::ItemIndex::unite(indices);
	}
	else {
		return sserialize::ItemIndex();
	}
}

TagNameCompleter::TagNameCompleter() {}
TagNameCompleter::TagNameCompleter(const Static::TagStore& tagStore) :
ExternalFunctoid(),
m_tagStore(tagStore)
{}

TagNameCompleter::~TagNameCompleter() {}

const std::string TagNameCompleter::cmdString() const {
	return std::string("t");
}

sserialize::ItemIndex TagNameCompleter::operator()(const std::string& queryString) {
	if (queryString.size() > 0) {
		std::string curStr;
		std::set< uint16_t > nodeIds;
		std::string::const_iterator qStrIt = queryString.begin();
		while (qStrIt != queryString.end()) {
			if (*qStrIt == ';') {
				if (curStr.size() > 0) {
					uint16_t nodeId;
					bool ok = m_tagStore.nodeId(sserialize::split< std::vector<std::string> >(curStr ,':'), nodeId);
					if (ok)
						nodeIds.insert(nodeId);
					curStr.clear();
				}
			}
			else {
				curStr += *qStrIt;
			}
			++qStrIt;
		}
		if (curStr.size() > 0) {
			uint16_t nodeId;
			bool ok = m_tagStore.nodeId(sserialize::split< std::vector<std::string> >(curStr, ':'), nodeId);
			if (ok)
				nodeIds.insert(nodeId);
			curStr.clear();
		}
		std::vector<sserialize::ItemIndex> indices;
		sserialize::ItemIndex idx;
		for(std::set< uint16_t >::iterator it = nodeIds.begin(); it != nodeIds.end(); ++it) {
			idx = m_tagStore.complete(*it);
			if (idx.size() > 0) {
				indices.push_back(idx);
			}
		}
		return sserialize::ItemIndex::unite(indices);
	}
	else {
		return sserialize::ItemIndex();
	}
}

TagPhraseCompleter::TagPhraseCompleter() {}
TagPhraseCompleter::TagPhraseCompleter(const TagStore & tagStore, std::istream & phrases) :
m_indexStore(tagStore.itemIndexStore())
{
	std::vector<std::string> path;
	path.resize(2);
	while (!phrases.eof()) {
		std::string poi;
		try {
			std::getline<>(phrases, poi);
			std::getline<>(phrases, path[0]);
			std::getline<>(phrases, path[1]);
			uint16_t nodeId;
			if (poi.find(' ') == std::string::npos && tagStore.nodeId(path, nodeId)) {
				m_poiToId[sserialize::unicode_to_lower(poi)].push_back(tagStore.at(nodeId).getIndexPtr() );
			}
		}
		catch (...) {
			sserialize::err("TagPhrase", "Parsing defintions");
			throw;
		}
	}
}
TagPhraseCompleter::~TagPhraseCompleter() {}

const std::string TagPhraseCompleter::cmdString() const {
	return std::string("p");
}

std::ostream & TagPhraseCompleter::printStats(std::ostream & out) const {
	out << "TagPhraseCompleter::Stats::BEGIN" << std::endl;
	out << "Stored Name->Tag mappings: " << m_poiToId.size() << std::endl;
	out << "TagPhraseCompleter::Stats::END" << std::endl;
	return out;
}

sserialize::ItemIndex TagPhraseCompleter::operator()(const std::string & queryString) {
	return complete(queryString);
}

sserialize::ItemIndex TagPhraseCompleter::complete(const std::string & queryString) {
	return const_cast<const TagPhraseCompleter*>(this)->complete(queryString);
}

sserialize::ItemIndex TagPhraseCompleter::complete(const std::string & queryString) const {
	if (queryString.size() > 0) {
		std::string curStr;
		std::set< uint32_t > indexIds;
		std::string::const_iterator qStrIt = queryString.begin();
		while (qStrIt != queryString.end()) {
			if (*qStrIt == ';') {
				if (curStr.size() > 0) {
					std::unordered_map<std::string, std::vector<uint32_t> >::const_iterator pIt( m_poiToId.find( sserialize::unicode_to_lower(curStr) ) );
					if (pIt != m_poiToId.end()) {
						for(std::vector<uint32_t>::const_iterator tIt(pIt->second.begin()); tIt != pIt->second.end(); ++tIt)
							indexIds.insert(*tIt);
					}
					curStr.clear();
				}
			}
			else {
				curStr += *qStrIt;
			}
			++qStrIt;
		}
		if (curStr.size() > 0) {
			std::unordered_map<std::string, std::vector<uint32_t> >::const_iterator pIt( m_poiToId.find( sserialize::unicode_to_lower(curStr) ) );
			if (pIt != m_poiToId.end()) {
				for(std::vector<uint32_t>::const_iterator tIt(pIt->second.begin()); tIt != pIt->second.end(); ++tIt)
					indexIds.insert(*tIt);
			}
			curStr.clear();
		}
		std::vector<sserialize::ItemIndex> indices;
		sserialize::ItemIndex idx;
		for(std::set< uint32_t >::iterator it = indexIds.begin(); it != indexIds.end(); ++it) {
			idx = m_indexStore.at(*it);
			if (idx.size() > 0) {
				indices.push_back(idx);
			}
		}
		return sserialize::ItemIndex::unite(indices);
	}
	else {
		return sserialize::ItemIndex();
	}
}

StringCompleterPrivateTagPhrase::StringCompleterPrivateTagPhrase(const sserialize::RCPtrWrapper<liboscar::Static::TagPhraseCompleter> & cmp) :
m_cmp(cmp)
{}

StringCompleterPrivateTagPhrase::~StringCompleterPrivateTagPhrase() {}

sserialize::ItemIndex StringCompleterPrivateTagPhrase::complete(const std::string & str, sserialize::StringCompleter::QuerryType /*qtype*/) const {
	return m_cmp->complete(str);
}

sserialize::ItemIndexIterator StringCompleterPrivateTagPhrase::partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType /*qtype*/) const {
	return sserialize::ItemIndexIterator(m_cmp->complete(str));
}

sserialize::StringCompleter::SupportedQuerries StringCompleterPrivateTagPhrase::getSupportedQuerries() const {
	return sserialize::StringCompleter::SQ_ALL;
}


std::ostream& StringCompleterPrivateTagPhrase::printStats(std::ostream& out) const {
	if (m_cmp.priv())
		m_cmp->printStats(out);
	return out;
}

std::string StringCompleterPrivateTagPhrase::getName() const {
	return std::string("TagPhraseCompleter");
}


}}//end namespace