#ifndef LIBOSCAR_TAGCOMPLETERS_H
#define LIBOSCAR_TAGCOMPLETERS_H
#include <unordered_map>
#include <sserialize/search/SetOpTree.h>
#include <sserialize/search/StringCompleterPrivate.h>
#include "StaticTagStore.h"

namespace liboscar {
namespace Static {


class TagCompleter: public sserialize::SetOpTree::ExternalFunctoid {
	Static::TagStore m_tagStore;
public:
	TagCompleter();
	TagCompleter(const Static::TagStore & tagStore);
	virtual ~TagCompleter();
	/** @param query : format of the query: (nodeId|((nodeId;)+nodeId)); these are all united! */
	virtual sserialize::ItemIndex operator()(const std::string & query) override;
	virtual const std::string cmdString() const override;
	inline const Static::TagStore & tagStore() const { return m_tagStore;}
};
class TagNameCompleter: public sserialize::SetOpTree::ExternalFunctoid {
	Static::TagStore m_tagStore;
public:
	TagNameCompleter();
	TagNameCompleter(const Static::TagStore & tagStore);
	virtual ~TagNameCompleter();
	/** @param query : format of the query: (path; path...); path=tagname:tagname:.. these are all united! */
	virtual sserialize::ItemIndex operator()(const std::string & query) override;
	virtual const std::string cmdString() const override;
	inline const Static::TagStore & tagStore() const { return m_tagStore;}
};

class TagPhraseCompleter: public sserialize::SetOpTree::ExternalFunctoid {
	sserialize::Static::ItemIndexStore m_indexStore;
	std::unordered_map<std::string, std::vector<uint32_t> > m_poiToId;
public:
	TagPhraseCompleter();
	TagPhraseCompleter(const liboscar::Static::TagStore & tagStore, std::istream & phrases);
	virtual ~TagPhraseCompleter();
	virtual sserialize::ItemIndex complete(const std::string & queryString) const;
	virtual sserialize::ItemIndex complete(const std::string & queryString) override;
	/** @param query : format of the query: (special phrase; special phrase...);  */
	virtual sserialize::ItemIndex operator()(const std::string & query) override;
	virtual const std::string cmdString() const override;
	std::ostream & printStats(std::ostream & out) const;
};

class StringCompleterPrivateTagPhrase: public sserialize::StringCompleterPrivate {
	sserialize::RCPtrWrapper<liboscar::Static::TagPhraseCompleter> m_cmp;
public:
	StringCompleterPrivateTagPhrase(const sserialize::RCPtrWrapper<liboscar::Static::TagPhraseCompleter> & cmp);
	virtual ~StringCompleterPrivateTagPhrase();
	
	virtual sserialize::ItemIndex complete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	
	virtual sserialize::ItemIndexIterator partialComplete(const std::string & str, sserialize::StringCompleter::QuerryType qtype) const;
	
	virtual sserialize::StringCompleter::SupportedQuerries getSupportedQuerries() const;

	virtual std::ostream& printStats(std::ostream& out) const;
	
	virtual std::string getName() const;
};


}}//end namespace

#endif