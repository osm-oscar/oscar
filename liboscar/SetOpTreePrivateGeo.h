#ifndef LIBOSCAR_SET_OP_TREE_PRIVATE_GEO_H
#define LIBOSCAR_SET_OP_TREE_PRIVATE_GEO_H
#include <sserialize/search/SetOpTreePrivateSimple.h>
#include <sserialize/containers/ItemIndex.h>
#include <sserialize/spatial/GeoRect.h>
#include "tagcompleters.h"

namespace liboscar {
namespace Static {

template<typename T_DB_TYPE>
class GeoConstraintFilter: public sserialize::ItemIndex::ItemFilter {
	T_DB_TYPE m_db;
	sserialize::spatial::GeoRect m_rect;
public:
	GeoConstraintFilter(const T_DB_TYPE & db, const sserialize::spatial::GeoRect & rect) : m_db(db), m_rect(rect) {}
	virtual ~GeoConstraintFilter() {}
	virtual bool operator()(uint32_t id) const { return m_db.match(id, m_rect); }
};

class SetOpTreePrivateGeo: public sserialize::SetOpTreePrivateSimple {
private:
	std::shared_ptr<sserialize::ItemIndex::ItemFilter> m_filter;
private:
	SetOpTreePrivateGeo & operator=(const SetOpTreePrivateGeo & other);
protected:
	SetOpTreePrivateGeo(const SetOpTreePrivateGeo & other);
public:
	SetOpTreePrivateGeo(const std::shared_ptr<sserialize::ItemIndex::ItemFilter> & filter);
	virtual SetOpTreePrivate * copy() const;
	virtual sserialize::ItemIndex doSetOperations();
};

}}//end namespace



#endif