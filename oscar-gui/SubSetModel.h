#ifndef OSCAR_GUI_SUB_SET_MODEL_H
#define OSCAR_GUI_SUB_SET_MODEL_H
#include <QAbstractItemModel>
#include <sserialize/Static/GeoHierarchy.h>
#include <liboscar/OsmKeyValueObjectStore.h>

namespace oscar_gui {

///QModelIndex.internalid() == RegionId

class SubSetModel: public QAbstractItemModel {
Q_OBJECT
private:
	struct DataNode {
		sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr d;
		DataNode * parent;
		uint32_t posInParent;
		inline uint32_t childrenSize() const { return (uint32_t) d->size(); }
		std::vector<DataNode*> children;
		DataNode(DataNode * parent, const sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr & d, uint32_t posInParent);
		~DataNode() { for(DataNode * n : children) delete n; }
	};
public:
	typedef enum { RegionGhId=Qt::ItemDataRole::UserRole, RegionStoreId=RegionGhId+1, SubSetNodePtr=RegionStoreId+1} DataAccessRoles;
private:
	liboscar::Static::OsmKeyValueObjectStore m_store;
	std::shared_ptr<sserialize::Static::spatial::GeoHierarchy::SubSet> m_subset;
	DataNode * m_root;
private:
	QString displayText(DataNode * n) const;
public:
	explicit SubSetModel(QObject* parent);
	virtual ~SubSetModel();
	void setStore(const liboscar::Static::OsmKeyValueObjectStore & store);
	virtual int columnCount(const QModelIndex & parent) const;
	virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
	virtual QModelIndex index(int row, int column, const QModelIndex & parent) const;
	virtual QModelIndex parent(const QModelIndex & child) const;
	virtual int rowCount(const QModelIndex & parent) const;
	const std::shared_ptr<sserialize::Static::spatial::GeoHierarchy::SubSet> & subSet() const { return m_subset; }
public slots:
	void updateSubSet(const std::shared_ptr<sserialize::Static::spatial::GeoHierarchy::SubSet> & subSet);
};


}//end namespace

Q_DECLARE_METATYPE(sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr);

#endif