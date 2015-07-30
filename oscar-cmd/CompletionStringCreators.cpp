#include "CompletionStringCreators.h"
#include <sserialize/strings/unicode_case_functions.h>

namespace oscarcmd {

inline std::string escapeString(const std::set<char> & escapeChars, const std::string & istr) {
	std::string ostr = "";
	for(std::string::const_iterator it = istr.begin(); it != istr.end(); ++it) {
		if (escapeChars.count(*it) > 0) {
			ostr += '\\';
		}
		ostr += *it;
	}
	return ostr;
}

std::deque< std::string > fromDB(const liboscar::Static::OsmKeyValueObjectStore & db, sserialize::StringCompleter::QuerryType qt, uint32_t count) {
	std::deque< std::string> ret;
	std::string escapeStr = "-+/\\^$[]() ";
	std::set<char> escapeChars(escapeStr.begin(), escapeStr.end());

	if (qt & sserialize::StringCompleter::QT_SUBSTRING) {
		for(size_t i = 0; i < count; i++) {
			uint32_t itemId = ((double)rand()/RAND_MAX)*db.size();
			liboscar::Static::OsmKeyValueObjectStore::Item item( db.at(itemId) );
			std::string str = "";
			for(size_t j = 0; j < item.strCount(); j++) {
				std::string s = item.strAt(j);
				if (s.size()) {
					if (*(s.begin()) == '\"') {
						s.erase(0,1);
						if (s.size() > 0 && *(s.rbegin()) == '\"') {
							s.erase(s.size()-1, 1);
						}
					}
					str += escapeString(escapeChars, s) + " ";
				}
			}
			if (qt & sserialize::StringCompleter::QT_CASE_INSENSITIVE)
				str = sserialize::unicode_to_lower(str);
			if (str.size() > 3)
				ret.push_back(str);
		}
	}
	return ret;
}






}//end  namespace