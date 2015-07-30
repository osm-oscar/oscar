#include "CellOpTree.h"
#include <sserialize/strings/stringfunctions.h>

namespace liboscar {
namespace detail {
namespace CellOpTree {

CTCStringHinter::CTCStringHinter(const Static::CellTextCompleter & qc) : 
m_qc(qc)
{}

CTCStringHinter::~CTCStringHinter() {}

bool CTCStringHinter::operator()(const std::string::const_iterator& begin, const std::string::const_iterator end) const {
	return m_qc.count(begin, end);
}

bool AlwaysFalseStringHinter::operator()(const std::string::const_iterator& /*begin*/, const std::string::const_iterator /*end*/) const {
	return false;
}

}}}//end namespace