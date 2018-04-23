#include "States.h"
#include <sserialize/Static/GeoWay.h>
#include <sserialize/Static/GeoPolygon.h>
#include <sserialize/Static/GeoMultiPolygon.h>

namespace oscar_gui {
	

LockableState::LockableState() :
m_sem(SemaphoreLocker::WRITE_LOCK)
{}

LockableState::~LockableState() {}

SemaphoreLocker LockableState::lock(SemaphoreLocker::Type t) const {
	return SemaphoreLocker(m_sem, t);
}

SemaphoreLocker LockableState::writeLock() const {
	return lock(SemaphoreLocker::WRITE_LOCK);
}

SemaphoreLocker LockableState::readLock() const {
	return lock(SemaphoreLocker::READ_LOCK);
}

//BEGIN SearchGeometryState

SearchGeometryState::SearchGeometryState() {}
SearchGeometryState::~SearchGeometryState() {}


SearchGeometryState::const_iterator SearchGeometryState::begin() const {
	return const_iterator::fromIterator(m_entries.begin());
}

SearchGeometryState::const_iterator SearchGeometryState::end() const {
	return const_iterator::fromIterator(m_entries.end());
}

void SearchGeometryState::add(const QString & name, const Marble::GeoDataLineString & data, DataType t) {
	std::size_t p = m_entries.size();
	{
		auto l(writeLock());
		m_entries.emplace_back(name, data, t);
	}
	emit dataChanged(p);
}

void SearchGeometryState::remove(std::size_t p) {
	{
		auto l(writeLock());
		m_entries.erase(m_entries.begin()+p);
	}
	emit dataChanged(m_entries.size());
}

void SearchGeometryState::activate(std::size_t p, oscar_gui::SearchGeometryState::ActiveType at) {
	{
		auto l(writeLock());
		m_entries.at(p).active |= at;
	}
	emit dataChanged(p);
}

void SearchGeometryState::deactivate(std::size_t p, oscar_gui::SearchGeometryState::ActiveType at) {
	{
		auto l(writeLock());
		m_entries.at(p).active &= ~at;
	}
	emit dataChanged(p);
}

void SearchGeometryState::toggle(std::size_t p, SearchGeometryState::ActiveType at) {
	{
		auto l(writeLock());
		if (m_entries.at(p).active & at) {
			m_entries[p].active &= ~at;
		}
		else {
			m_entries.at(p).active |= at;
		}
	}
	emit dataChanged(p);
}

void SearchGeometryState::setCells(std::size_t p, const sserialize::ItemIndex & idx) {
	{
		auto l(writeLock());
		m_entries.at(p).cells = idx;
	}
	emit dataChanged(p);
}

void SearchGeometryState::setTriangles(std::size_t p, const sserialize::ItemIndex & idx) {
	{
		auto l(writeLock());
		m_entries.at(p).triangles = idx;
	}
	emit dataChanged(p);
}

std::size_t SearchGeometryState::size() const {
	auto l(readLock());
	return m_entries.size();
}

const QString & SearchGeometryState::name(std::size_t p) const {
	auto l(readLock());
	return m_entries.at(p).name;
}

SearchGeometryState::ActiveType SearchGeometryState::active(std::size_t p) const {
	auto l(readLock());
	return (ActiveType) m_entries.at(p).active;
}

SearchGeometryState::DataType SearchGeometryState::type(std::size_t p) const {
	auto l(readLock());
	return m_entries.at(p).type;
}

const Marble::GeoDataLineString & SearchGeometryState::data(std::size_t p) const {
	auto l(readLock());
	return m_entries.at(p).data;
}

const sserialize::ItemIndex& SearchGeometryState::cells(std::size_t p) const {
	auto l(readLock());
	return m_entries.at(p).cells;
}

const sserialize::ItemIndex& SearchGeometryState::triangles(std::size_t p) const {
	auto l(readLock());
	return m_entries.at(p).triangles;
}

//END SearchGeometryState
//BEGIN TextSearchState

QString TextSearchState::searchText() const {
	auto l(readLock());
	return m_searchText;
}

void TextSearchState::setSearchText(const QString & value) {
	{
		auto l(writeLock());
		m_searchText = value;
	}
	emit( searchTextChanged(value) );
}

//END TextSearchState
//BEGIN ItemGeometryState


ItemGeometryState::ItemGeometryState(const liboscar::Static::OsmKeyValueObjectStore & store) :
m_store(store)
{}

ItemGeometryState::const_iterator ItemGeometryState::begin() const {
	using TransformIterator = sserialize::TransformIterator<GetSecond, const Entry&, std::unordered_map<uint32_t, Entry>::const_iterator>;
	return const_iterator::fromIterator( TransformIterator(m_entries.begin()) );
}

ItemGeometryState::const_iterator ItemGeometryState::end() const {
	using TransformIterator = sserialize::TransformIterator<GetSecond, const Entry&, std::unordered_map<uint32_t, Entry>::const_iterator>;
	return const_iterator::fromIterator( TransformIterator(m_entries.end()) );
}


int ItemGeometryState::active(uint32_t itemId) const {
	if (m_entries.count(itemId)) {
		return m_entries.at(itemId).active;
	}
	return AT_NONE;
}

void ItemGeometryState::activate(uint32_t itemId, ActiveType at) {
	auto l(writeLock());
	if (!m_entries.count(itemId)) {
		addItem(itemId);
	}
	m_entries.at(itemId).active |= at;
	emit( dataChanged() );
}

void ItemGeometryState::deactivate(uint32_t itemId, ActiveType at) {
	auto l(writeLock());
	if (m_entries.count(itemId)) {
		m_entries.at(itemId).active &= ~at;
		if (m_entries.at(itemId).active == AT_NONE) {
			m_entries.erase(itemId);
		}
		emit( dataChanged() );
	}
}

void ItemGeometryState::toggleItem(uint32_t itemId, ActiveType at) {
	auto l(writeLock());
	if (!m_entries.count(itemId)) {
		addItem(itemId);
	}
	if (m_entries.at(itemId).active & at) {
		m_entries[itemId].active &= ~at;
	}
	else {
		m_entries.at(itemId).active |= at;
	}
	if (m_entries.at(itemId).active == AT_NONE) {
		m_entries.erase(itemId);
	}
	emit( dataChanged() );
}

void ItemGeometryState::addItem(uint32_t itemId) {
	using Item = liboscar::Static::OsmKeyValueObjectStore::Item;
	Entry & entry = m_entries[itemId];
	Item item = m_store.at(itemId);
	auto namePos = item.findKey("name");
	if (namePos != item.npos) {
		entry.name = QString::fromStdString( item.value(namePos) );
	}
	entry.cells = sserialize::ItemIndex(item.cells());
	auto shape = item.geoShape();
	switch (shape.type()) {
	case sserialize::spatial::GS_POINT:
	{
		auto p = shape.get<sserialize::Static::spatial::GeoPoint>();
		entry.type = DT_POINT;
		entry.data.append(Marble::GeoDataCoordinates(p->lon(), p->lat(), 0.0, Marble::GeoDataCoordinates::Degree));
		break;
	}
	case sserialize::spatial::GS_WAY:
	{
		auto gs = shape.get<sserialize::Static::spatial::GeoWay>();
		entry.type = DT_PATH;
		for(sserialize::Static::spatial::GeoPoint p : *gs) {
			entry.data.append(Marble::GeoDataCoordinates(p.lon(), p.lat(), 0.0, Marble::GeoDataCoordinates::Degree));
		}
		break;
	}
	case sserialize::spatial::GS_POLYGON:
	{
		auto gs = shape.get<sserialize::Static::spatial::GeoPolygon>();
		entry.type = DT_POLYGON;
		for(sserialize::Static::spatial::GeoPoint p : *gs) {
			entry.data.append(Marble::GeoDataCoordinates(p.lon(), p.lat(), 0.0, Marble::GeoDataCoordinates::Degree));
		}
		break;
	}
	case sserialize::spatial::GS_MULTI_POLYGON:
	{
		auto gs = shape.get<sserialize::Static::spatial::GeoMultiPolygon>();
		entry.type = DT_POLYGON;
		for(sserialize::Static::spatial::GeoPoint p : gs->outerPolygons().front()) {
			entry.data.append(Marble::GeoDataCoordinates(p.lon(), p.lat(), 0.0, Marble::GeoDataCoordinates::Degree));
		}
		break;
	}
	default:
		break;
	};
}

//END ItemGeometryState
//BEGIN ResultListState

void ResultListState::setResult(const QString & queryString, const sserialize::CellQueryResult & cqr, const sserialize::ItemIndex & items) {
	{
		auto l(writeLock());
		m_qs = queryString;
		m_cqr = cqr;
		m_items = items;
	}
	emit( dataChanged() );
}

QString ResultListState::queryString() const {
	auto l(readLock());
	return m_qs;
}

sserialize::ItemIndex ResultListState::items() const {
	auto l(readLock());
	auto cp = m_items;
	return cp;
}

sserialize::CellQueryResult ResultListState::cqr() const {
	auto l(readLock());
	auto cp = m_cqr;
	return cp;
}

uint32_t ResultListState::itemId(uint32_t pos) const {
	auto l(readLock());
	return m_items.at(pos);
}

std::size_t ResultListState::size() const {
	auto l(readLock());
	return m_items.size();
}

//END ResultListState

//BEGIN States

States::States(const std::shared_ptr<liboscar::Static::OsmCompleter> & cmp) :
cmp(cmp),
sgs(std::make_shared<SearchGeometryState>()),
igs(std::make_shared<ItemGeometryState>(cmp->store())),
tss(std::make_shared<TextSearchState>()),
rls(std::make_shared<ResultListState>())
{}

//END States

} //end namespace oscar_gui
