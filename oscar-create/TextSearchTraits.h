#ifndef OSCAR_CREATE_TEXT_SEARCH_TRAITS_H
#define OSCAR_CREATE_TEXT_SEARCH_TRAITS_H
#include <liboscar/OsmKeyValueObjectStore.h>
#include "Config.h"
#include "CellTextCompleter.h"

namespace oscar_create {

class OOM_SA_CTC_Traits {
private:
	struct State {
		std::unordered_set<uint32_t> m_valuePSFilter;
		std::unordered_set<uint32_t> m_valueSSFilter;
		std::unordered_map<uint32_t, std::string> m_tagPSFilter;
		std::unordered_map<uint32_t, std::string> m_tagSSFilter;
	};
public:
	typedef liboscar::Static::OsmKeyValueObjectStore::Item item_type;
	struct ExactStrings {
		std::shared_ptr<State> state;
	public:
		ExactStrings(const std::shared_ptr<State> & state) : state(state) {}
		template<typename TOutputIterator>
		void operator()(const item_type & item, TOutputIterator out) {
			for(uint32_t i(0), s(item.size()); i < s; ++i) {
				uint32_t keyId = item.keyId(i);
				if (state->m_valuePSFilter.count(keyId)) {
					*out = item.value(i);
					++out;
				}
				if (state->m_tagPSFilter.count(keyId)) {
					*out = std::string("@") + state->m_tagPSFilter.at(keyId) + ":" + item.valid(i);
					++out;
				}
			}
		}
	};
	class SuffixStrings {
		std::shared_ptr<State> state;
	public:
		SuffixStrings(const std::shared_ptr<State> & state) : state(state) {}
		template<typename TOutputIterator>
		void operator()(const item_type & item, TOutputIterator out) {
			for(uint32_t i(0), s(item.size()); i < s; ++i) {
				uint32_t keyId = item.keyId(i);
				if (state->m_valueSSFilter.count(keyId)) {
					*out = item.value(i);
					++out;
				}
				if (state->m_tagSSFilter.count(keyId)) {
					*out = std::string("@") + state->m_tagSSFilter.at(keyId) + ":" + item.valid(i);
					++out;
				}
			}
		}
	};
	struct ItemId {
		inline uint32_t operator()(const item_type & item) { return item.id(); }
	};
	struct ItemCells {
		template<typename TOutputIterator>
		void operator()(const item_type & item, TOutputIterator out) {
			for(const auto & x : item.cells()) {
				*out = x;
				++out;
			}
		}
	};
public:
	inline ExactStrings exactStrings() { return ExactStrings(m_state); }
	inline SuffixStrings suffixStrings() { return SuffixStrings(m_state); }
	inline ItemId itemId() { return ItemId(); }
	inline ItemCells itemCells() { return ItemCells(); }
public:
	OOM_SA_CTC_Traits(const TextSearchConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store);
private:
	std::shared_ptr<State> m_state;
};

class OsmKeyValueObjectStoreDerfer {
public:
	typedef std::vector<std::string> value_type;
public:
	OsmKeyValueObjectStoreDerfer(const TextSearchConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store);
	void operator()(
		uint32_t itemId,
		sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer & prefixStrings,
		sserialize::GeneralizedTrie::SinglePassTrie::StringsContainer & suffixStrings
	) const;
protected:
	liboscar::Static::OsmKeyValueObjectStore m_store;
	std::shared_ptr< std::unordered_set<uint32_t> > m_filter;
	std::shared_ptr< std::unordered_map<uint32_t, std::string> > m_tagPrefixSearchFilter;
	std::shared_ptr< std::unordered_map<uint32_t, std::string> > m_tagSuffixSearchFilter;
	uint32_t m_largestId;
	bool m_suffix;

};

class CellTextCompleterDerfer: public OsmKeyValueObjectStoreDerfer {
public:
	typedef detail::CellTextCompleter::SampleItemStringsContainer StringsContainer;
public:
	CellTextCompleterDerfer(const TextSearchConfig & tsc, const liboscar::Static::OsmKeyValueObjectStore & store) ;
	void operator()(const liboscar::Static::OsmKeyValueObjectStore::Item & item, StringsContainer & itemStrings, bool insertsAsItem) const;
private:
	bool m_inSensitive;
	bool m_diacriticInSensitive;
	sserialize::DiacriticRemover m_dr;
	std::unordered_set<uint32_t> m_seps;
};

}//end namespace oscar_create


#endif