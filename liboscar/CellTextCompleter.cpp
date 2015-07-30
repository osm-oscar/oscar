#include "CellTextCompleter.h"
#include <sserialize/strings/unicode_case_functions.h>
#define LIBOSCAR_CELL_TEXT_COMPLETER_TAG_COMPLETION_PREFIX '@'


namespace liboscar {
namespace Static {
namespace detail {
CellTextCompleter::Payload::Payload(const sserialize::UByteArrayAdapter & d) :
m_types(d.getUint8(0)),
m_offsets{0,0,0},
m_data(d)
{
	uint8_t typeCount = sserialize::popCount<unsigned int>(m_types & 0xF);
	m_data += sserialize::SerializationInfo<uint8_t>::length;
	for(uint8_t i=1; i < typeCount; ++i) {
		m_offsets[i-1] = m_data.getVlPackedUint32();
	}
	m_data.shrinkToGetPtr();
}

sserialize::UByteArrayAdapter CellTextCompleter::Payload::typeData(sserialize::StringCompleter::QuerryType qt) const {
	qt = (sserialize::StringCompleter::QuerryType) (qt & sserialize::StringCompleter::QT_EPSS);
	sserialize::StringCompleter::QuerryType myTypes = types();

	if (! (qt & myTypes )) {
		if (qt & sserialize::StringCompleter::QT_PREFIX) {
			qt = (sserialize::StringCompleter::QuerryType)(myTypes & sserialize::StringCompleter::QT_EXACT);
		}
		else if (qt & sserialize::StringCompleter::QT_SUBSTRING) {
			qt = (sserialize::StringCompleter::QuerryType)(myTypes & sserialize::StringCompleter::QT_SUFFIX);
		}
		else {
			qt = sserialize::StringCompleter::QT_NONE;
		}
	}
	
	if (qt == sserialize::StringCompleter::QT_NONE || sserialize::popCount<unsigned int>(qt) != 1) {
		return sserialize::UByteArrayAdapter();
	}
	
	uint32_t pos = sserialize::popCount((static_cast<uint32_t>(qt)-1) & types());
	
	uint32_t totalOffset = 0;
	uint32_t dataLength;
	for(uint32_t i(1); i <= pos; ++i) {
		totalOffset += m_offsets[i-1];
	}
	assert(pos < 4);
	if (pos == 3 || m_offsets[pos] == 0) {
		dataLength = m_data.size() - totalOffset;
	}
	else {
		dataLength = m_offsets[pos];
	}
	return sserialize::UByteArrayAdapter(m_data, totalOffset, dataLength);
}


CellTextCompleter::Payload::Type CellTextCompleter::Payload::type(sserialize::StringCompleter::QuerryType qt) const {
	qt = (sserialize::StringCompleter::QuerryType) (qt & sserialize::StringCompleter::QT_EPSS);
	sserialize::StringCompleter::QuerryType myTypes = types();

	if (! (qt & myTypes )) {
		if (qt & sserialize::StringCompleter::QT_PREFIX) {
			qt = (sserialize::StringCompleter::QuerryType)(myTypes & sserialize::StringCompleter::QT_EXACT);
		}
		else if (qt & sserialize::StringCompleter::QT_SUBSTRING) {
			qt = (sserialize::StringCompleter::QuerryType)(myTypes & sserialize::StringCompleter::QT_SUFFIX);
		}
		else {
			qt = sserialize::StringCompleter::QT_NONE;
		}
	}
	
	if (qt == sserialize::StringCompleter::QT_NONE || sserialize::popCount<unsigned int>(qt) != 1) {
		return Type();
	}
	
	uint32_t pos = sserialize::popCount((static_cast<uint32_t>(qt)-1) & types());
	
	uint32_t totalOffset = 0;
	for(uint32_t i(1); i <= pos; ++i) {
		totalOffset += m_offsets[i-1];
	}

	return Type(sserialize::RLEStream(m_data+totalOffset));
}


CellTextCompleter::CellTextCompleter() {}

CellTextCompleter::CellTextCompleter(
const sserialize::UByteArrayAdapter & d, const sserialize::Static::ItemIndexStore& idxStore,
const sserialize::Static::spatial::GeoHierarchy & gh, const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement & ra) :
m_sq( (sserialize::StringCompleter::SupportedQuerries) d.at(1) ),
m_tt( (TrieTypeMarker) d.at(2) ),
m_idxStore(idxStore),
m_gh(gh),
m_ra(ra)
{
	SSERIALIZE_VERSION_MISSMATCH_CHECK(LIBOSCAR_STATIC_CELL_TEXT_COMPLETER_VERSION, d.at(0), "liboscar::Static::CellTextCompleter");
	try {
		switch (m_tt) {
		case TT_TRIE:
			m_trie = Trie( Trie::PrivPtrType(new TrieType(d+3)) );
			break;
		case TT_FLAT_TRIE:
			m_trie = Trie( Trie::PrivPtrType(new FlatTrieType(d+3)) );
			break;
		default:
			throw sserialize::TypeMissMatchException("liboscar::CellTextCompleter::CellTextCompleter unkown trie type");
			break;
		}
	}
	catch (sserialize::Exception & e) {
		e.appendMessage("liboscar::CellTextCompleter::CellTextCompleter: initializing trie");
		throw e;
	}
}

CellTextCompleter::~CellTextCompleter() {}

bool CellTextCompleter::count(const std::string::const_iterator& begin, const std::string::const_iterator & end) const {
	return m_trie.count(begin, end, true);
}

CellTextCompleter::Payload::Type CellTextCompleter::typeFromCompletion(const std::string& qs, const sserialize::StringCompleter::QuerryType qt) const {
	std::string qstr;
	if (m_sq & sserialize::StringCompleter::SQ_CASE_INSENSITIVE && qs.size() && qs[0] != LIBOSCAR_CELL_TEXT_COMPLETER_TAG_COMPLETION_PREFIX) {
		qstr = sserialize::unicode_to_lower(qs);
	}
	else {
		qstr = qs;
	}
	Payload p( m_trie.at(qstr, (qt & sserialize::StringCompleter::QT_SUBSTRING || qt & sserialize::StringCompleter::QT_PREFIX)) );
	Payload::Type t;
	if (p.types() & qt) {
		t = p.type(qt);
	}
	else if (qt & sserialize::StringCompleter::QT_SUBSTRING) {
		if (p.types() & sserialize::StringCompleter::QT_PREFIX) { //exact suffix matches are either available or not
			t = p.type(sserialize::StringCompleter::QT_PREFIX);
		}
		else if (p.types() & sserialize::StringCompleter::QT_SUFFIX) {
			t = p.type(sserialize::StringCompleter::QT_SUFFIX);
		}
		else if (p.types() & sserialize::StringCompleter::QT_EXACT) {
			t = p.type(sserialize::StringCompleter::QT_EXACT);
		}
		else {
			throw sserialize::OutOfBoundsException("CellTextCompleter::typeFromCompletion");
		}
	}
	else if (p.types() & sserialize::StringCompleter::QT_EXACT) { //qt is either prefix, suffix, exact
		t = p.type(sserialize::StringCompleter::QT_EXACT);
	}
	else {
		throw sserialize::OutOfBoundsException("CellTextCompleter::typeFromCompletion");
	}
	return t;
}

sserialize::StringCompleter::SupportedQuerries CellTextCompleter::getSupportedQuerries() const {
	return m_sq;
}

std::ostream& CellTextCompleter::printStats(std::ostream& out) const {
	out << "CellTextCompleter::BEGIN_STATS" << std::endl;
	m_trie.printStats(out);
	out << "CellTextCompleter::END_STATS" << std::endl;
	return out;
}
}//end namespace detail

sserialize::ItemIndex CellTextCompleterUnclustered::complete(const std::string& str, sserialize::StringCompleter::QuerryType qtype) const {
	return m_cellTextCompleter.complete(str, qtype).flaten();
}

std::string CellTextCompleterUnclustered::getName() const {
    return std::string("CellTextCompleterUnclustered");
}

sserialize::StringCompleter::SupportedQuerries CellTextCompleterUnclustered::getSupportedQuerries() const {
	return m_cellTextCompleter.getSupportedQuerries();
}

sserialize::ItemIndex CellTextCompleterUnclustered::indexFromId(uint32_t idxId) const {
	return m_cellTextCompleter.idxStore().at(idxId);
}

std::ostream& CellTextCompleterUnclustered::printStats(std::ostream& out) const {
    return m_cellTextCompleter.printStats(out);
}

}}//end namespace
