#include "States.h"

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


void SearchGeometryState::add(const QString & name, const Marble::GeoDataLineString & data, DataType t) {
	std::size_t p = m_entries.size();
	{
		auto l(writeLock());
		m_entries[p] = Entry(name, data, t);
	}
	emit dataChanged(p);
}

void SearchGeometryState::remove(std::size_t p) {
	{
		auto l(writeLock());
		m_entries.erase(p);
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
	if (m_entries.count(p)) {
		return (ActiveType) m_entries.at(p).active;
	}
	return AT_NONE;
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
	auto l(writeLock());
	m_searchText = value;
	emit( searchTextChanged(value) );
}

//END TextSearchState
//BEGIN ResultListState

void ResultListState::setResult(const QString & queryString, const sserialize::ItemIndex & items) {
	auto l(writeLock());
	m_qs = queryString;
	m_items = items;
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
igs(std::make_shared<ItemGeometryState>()),
tss(std::make_shared<TextSearchState>()),
rls(std::make_shared<ResultListState>())
{}

//END States

} //end namespace oscar_gui
