#include "SetOpTreePrivateGeo.h"
#include <sserialize/containers/ItemIndex.h>

namespace liboscar {
namespace Static {

SetOpTreePrivateGeo::SetOpTreePrivateGeo(const std::shared_ptr<sserialize::ItemIndex::ItemFilter> & filter) :
SetOpTreePrivateSimple(),
m_filter(filter)
{}

SetOpTreePrivateGeo::SetOpTreePrivateGeo(const SetOpTreePrivateGeo & other) :
SetOpTreePrivateSimple(other),
m_filter(other.m_filter)
{}


sserialize::SetOpTreePrivate * SetOpTreePrivateGeo::copy() const {
	return new SetOpTreePrivateGeo(*this);
}

sserialize::ItemIndex SetOpTreePrivateGeo::doSetOperations() {
	if (intersectStrings().size()) {
		std::vector<sserialize::ItemIndex> intersectIdx, diffIdx;
		for(std::vector<QueryStringDescription>::const_iterator it = intersectStrings().begin(); it != intersectStrings().end(); ++it) {
			sserialize::ItemIndex idx = completions().at(*it);
			if (idx.size())
				intersectIdx.push_back( idx );
			else
				return sserialize::ItemIndex();
		}
		
		for(std::vector<QueryStringDescription>::const_iterator it = diffStrings().begin(); it != diffStrings().end(); ++it) {
			sserialize::ItemIndex idx = completions().at(*it);
			if (idx.size())
				diffIdx.push_back( idx );
		}
		
		if (intersectIdx.size() > 0) {
			if (diffIdx.size())
				return sserialize::ItemIndex::fusedIntersectDifference(intersectIdx, diffIdx, maxResultSetSize(), m_filter.get());
			else
				return sserialize::ItemIndex::constrainedIntersect(intersectIdx, maxResultSetSize(), m_filter.get());
		}
	}
	return sserialize::ItemIndex();
}


}}//end namespace