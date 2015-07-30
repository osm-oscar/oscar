#ifndef OSCAR_CREATE_SCORE_CREATOR_H
#define OSCAR_CREATE_SCORE_CREATOR_H
#include <sserialize/algorithm/hashspecializations.h>
#include <unordered_map>
#include  <osmpbf/filter.h>

namespace oscar_create {

class ScoreCreator {
	std::unordered_map<std::string, uint32_t> m_keyScores;
	std::unordered_map< std::pair<std::string, std::string>, uint32_t  > m_keyValScores;
public:
	ScoreCreator(const std::string & configFileName);
	virtual ~ScoreCreator();
	const std::unordered_map<std::string, uint32_t> & keyScores() const { return m_keyScores; }
	const std::unordered_map< std::pair<std::string, std::string>, uint32_t  > & keyValScores() const { return m_keyValScores; }
	osmpbf::AbstractTagFilter * createKeyFilter() const;
	osmpbf::AbstractTagFilter * createKeyValueFilter() const;
	bool hasScore(const std::string & key) const;
	uint32_t score(const std::string & key) const;
	uint32_t score(const std::string & key, const std::string & value) const;
	std::ostream & dump(std::ostream & out) const;
};

}//end namespace

#endif