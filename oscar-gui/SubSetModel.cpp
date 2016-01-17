#include "SubSetModel.h"

namespace oscar_gui {

SubSetModel::DataNode::DataNode(oscar_gui::SubSetModel::DataNode* parent, const sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr& d, uint32_t posInParent) :
d(d),
parent(parent),
posInParent(posInParent)
{
	if (d.priv()) {
		children.resize(d->size(), 0);
	}
}

QString SubSetModel::displayText(DataNode * n) const {
	uint32_t regionGhId = n->d->ghId();
	uint32_t regionStoreId = m_store.geoHierarchy().ghIdToStoreId(regionGhId);
	liboscar::Static::OsmKeyValueObjectStoreItem item(m_store.at(regionStoreId));
	uint32_t regionFullItemsCount = m_store.geoHierarchy().regionItemsCount(regionGhId);
	QString name;
	uint32_t keyPos = item.findKey("name");
	if (keyPos != liboscar::Static::OsmKeyValueObjectStoreItem::npos) {
		name = QString::fromUtf8( item.value(keyPos).c_str() ) + "=" + QString::number(regionStoreId);
	}
	else {
		name += "NONAME";
	}
	name.append(" with <=");
	name.append(QString::number(n->d->maxItemsSize()));
	name.append(" of ");
	name.append(QString::number(regionFullItemsCount));
	name.append(')');
	return name;
}

SubSetModel::SubSetModel(QObject* parent) :
QAbstractItemModel(parent),
m_root(0)
{}

SubSetModel::~SubSetModel() {
	delete m_root;
}

void SubSetModel::setStore(const liboscar::Static::OsmKeyValueObjectStore & store) {
	emit beginResetModel();
	delete m_root;
	m_root = 0;
	m_subset = std::make_shared<sserialize::Static::spatial::GeoHierarchy::SubSet>();
	m_store = store;
	emit endResetModel();
}

int SubSetModel::columnCount(const QModelIndex &) const {
	return (m_root ? 1 : 0);
}

QVariant SubSetModel::data(const QModelIndex & index, int role) const {
	if (! index.isValid())
		return QVariant();
	
	
	switch (role) {
	case(Qt::DisplayRole):
		{
			int row = index.row();
			DataNode * node = (DataNode*) index.internalPointer();
			DataNode * parent = node->parent;
			
			if (!parent || row < 0 || parent->childrenSize() <= static_cast<unsigned>(row)) {
				return QVariant("ERROR_INVALID");
			}
			return QVariant( displayText(node) );
		}
	case(RegionGhId):
		{
			DataNode * node = (DataNode*) index.internalPointer();
			return QVariant(node->d->ghId());
		}
	case (RegionStoreId):
		{
			DataNode * node = (DataNode*) index.internalPointer();
			return QVariant( m_store.geoHierarchy().ghIdToStoreId(node->d->ghId()) );
		}
	case (SubSetNodePtr):
		{
			DataNode * node = (DataNode*) index.internalPointer();
			QVariant ret;
			ret.setValue(node->d);
			return ret;
		}
	default:
		return QVariant();
	}
}

QModelIndex SubSetModel::index(int row, int column, const QModelIndex & parent) const {
	if (!m_root)
		return QModelIndex();
	
	DataNode * parentPtr = m_root;
	if (parent.isValid()) {
		parentPtr = static_cast<DataNode*>(parent.internalPointer());
	}
	
	if (row < 0 || parentPtr->childrenSize() <= static_cast<unsigned>(row)) {
		return QModelIndex();
	}
	
	DataNode* &childPtr = parentPtr->children[row];
	if (!childPtr) {
		childPtr = new DataNode(parentPtr, parentPtr->d->at(row), row);
	}
	
	return createIndex(row, column, childPtr);
}

QModelIndex SubSetModel::parent(const QModelIndex & index) const {
	if (!index.isValid())
		return QModelIndex();
	
	DataNode * node = static_cast<DataNode*>(index.internalPointer());
	if (!node->parent) {
		return QModelIndex();
	}
	else {
		return createIndex(node->parent->posInParent, 0, node->parent);
	}
}

int SubSetModel::rowCount(const QModelIndex & parent) const {
	if (!m_root)
		return 0;
	
	if (parent.isValid()) {
		DataNode * node = (DataNode*) parent.internalPointer();
		int rowCount = node->childrenSize();
		return rowCount;
	}
	else {
		return m_root->childrenSize();
	}
	
}

void SubSetModel::updateSubSet(const std::shared_ptr< sserialize::Static::spatial::GeoHierarchy::SubSet >& subSet) {
	emit beginResetModel();
	delete m_root;
	m_root = 0;
	m_subset = subSet;
	if (m_subset->root().priv()) {
		m_root = new DataNode(0, m_subset->root(), 0xFFFFFFFF);
	}
	emit endResetModel();
}


}//end namespace