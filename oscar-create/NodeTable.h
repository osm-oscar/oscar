#ifndef OSCAR_CREATE_NODE_TABLE_H
#define OSCAR_CREATE_NODE_TABLE_H
#include <sserialize/containers/MMVector.h>
#include <sserialize/spatial/GeoPoint.h>
#include <sserialize/containers/DirectHugeHash.h>


namespace oscar_create {
class NodeTable;
namespace detail {
namespace nodetable {

typedef std::pair<double, double> RawGeoPoint;

class NodeTableBase: public sserialize::RefCountObject {
public:
	NodeTableBase();
	virtual ~NodeTableBase();
	virtual const RawGeoPoint & at(int64_t nodeId) const = 0;
	virtual void count(int64_t nodeId) const = 0;
};

class DirectNodeTable: public NodeTableBase {
private:
	
private:
	sserialize::DirectHugeHashMap<RawGeoPoint> m_d;
public:
	DirectNodeTable();
	virtual ~DirectNodeTable();
	virtual const RawGeoPoint & at(int64_t nodeId) const override;
	virtual void count(int64_t nodeId) const override;
};

}}//end namespace detail::nodetable


class NodeTable final {
public:
	typedef detail::nodetable::RawGeoPoint RawGeoPoint;
private:
	sserialize::RCPtrWrapper<detail::nodetable::NodeTableBase> m_priv;
public:
	NodeTable();
	~NodeTable();
	void populate(const std::string & filename);
	inline const RawGeoPoint & at(int64_t nodeId) const { return m_priv->at(nodeId);}
	inline void count(int64_t nodeId) const { return m_priv->count(nodeId);}
	inline sserialize::spatial::GeoPoint geoPoint(int64_t nodeId) const { return sserialize::spatial::GeoPoint(at(nodeId));}
};

}//end namespace


#endif