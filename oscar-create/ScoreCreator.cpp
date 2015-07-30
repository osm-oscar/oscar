#include "ScoreCreator.h"
#include "helpers.h"
#include <iostream>


namespace oscar_create {

//Scores are partly taken from http://wiki.openstreetmap.org/wiki/Nominatim/Development_overview

ScoreCreator::ScoreCreator(const std::string & configFileName) {
	if (!sserialize::MmappedFile::fileExists(configFileName)) {
		return;
	}
	struct MyOutIt {
		std::unordered_map<std::string, uint32_t> * m_keyScores;
		std::unordered_map< std::pair<std::string, std::string>, uint32_t  > * m_keyValScores;
		std::vector<std::string> tmpKVScore;
		std::vector<std::string> tmpKV;
		MyOutIt & operator++() { return *this; }
		MyOutIt & operator*() { return *this; }
		MyOutIt & operator=(const std::string & str) {
			typedef sserialize::OneValueSet<uint32_t> MySet;
			tmpKVScore.clear();
			tmpKV.clear();
			sserialize::split<std::string::const_iterator, MySet, MySet>(str.cbegin(), str.cend(), MySet('='), MySet('\\'), std::back_inserter(tmpKVScore) );
			if (tmpKVScore.size() == 2) {
				sserialize::split<std::string::const_iterator, MySet, MySet>(tmpKVScore.front().cbegin(), tmpKVScore.front().cend(), MySet(':'), MySet('\\'), std::back_inserter(tmpKV) );
				uint32_t myScore = atoi(tmpKVScore.back().c_str());
				if (tmpKV.size() == 1) {
					(*m_keyScores)[tmpKV.front()] = myScore;
				}
				else if (tmpKV.size() == 2) {
					(*m_keyValScores)[std::pair<std::string, std::string>(tmpKV.front(), tmpKV.back())] = myScore;
					if (!m_keyScores->count(tmpKV.front())) {
						(*m_keyScores)[tmpKV.front()] = 0;
					}
				}
			}
			return *this;
		}
	};
	MyOutIt outIt;
	outIt.m_keyScores = &m_keyScores;
	outIt.m_keyValScores = &m_keyValScores;
	sserialize::MmappedMemory<char> mf(configFileName);

	typedef sserialize::OneValueSet<uint32_t> MySet;
	sserialize::split<const char*, MySet, MySet, MyOutIt>(mf.begin(), mf.end(), MySet('\n'), MySet('\\'), outIt);
	std::cout << "ScoreCreator: Found " << m_keyScores.size() << " keys and " << m_keyValScores.size() << " key-values" << std::endl;
}

ScoreCreator::~ScoreCreator() {}


osmpbf::AbstractTagFilter* ScoreCreator::createKeyFilter() const {
	 osmpbf::KeyMultiValueTagFilter * f = new osmpbf::KeyMultiValueTagFilter("place");
	 for(std::unordered_map<std::string, uint32_t>::const_iterator it = m_keyScores.begin(); it != m_keyScores.end(); ++it) {
		f->addValue(it->first);
	 }
	 return f;
}

osmpbf::AbstractTagFilter* ScoreCreator::createKeyValueFilter() const {
	osmpbf::KeyMultiValueTagFilter * f = new osmpbf::KeyMultiValueTagFilter("place");
	f->addValue("place");
	f->addValue("highway");
	return f;
}

bool ScoreCreator::hasScore(const std::string & key ) const {
	return m_keyScores.count(key) > 0;
}

uint32_t ScoreCreator::score(const std::string& key) const {
	if (m_keyScores.count(key) > 0)
		return m_keyScores.at(key);
	return 0;
}

uint32_t ScoreCreator::score(const std::string & key, const std::string& value) const {
	if (m_keyValScores.count(std::pair<std::string, std::string>(key, value)) > 0)
		return m_keyValScores.at(std::pair<std::string, std::string>(key, value));
	return 0;
}

std::ostream& ScoreCreator::dump(std::ostream& out) const {
	for(const auto & x : m_keyScores) {
		out << x.first << "=" << x.second << "\n";
	}
	for(const auto & x : m_keyValScores) {
		out << x.first.first << ":" << x.first.second << "=" << x.second << "\n";
	}
	return out;
}



}//end namespace
