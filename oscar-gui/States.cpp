#include "States.h"

namespace oscar_gui {

SearchGeometryState::SearchGeometryState() : m_sem(SemaphoreLocker::WRITE_LOCK) {}
SearchGeometryState::~SearchGeometryState() {}

SemaphoreLocker SearchGeometryState::lock(SemaphoreLocker::Type t) const {
	return SemaphoreLocker(m_sem, t);
}

void SearchGeometryState::add(const QString & name, const Marble::GeoDataLineString & data, DataType t) {
	{
		auto l(lock(SemaphoreLocker::WRITE_LOCK));
		m_entries.push_back(Entry(name, data, t));
	}
	emit dataChanged();
}

void SearchGeometryState::remove(std::size_t p) {
	{
		auto l(lock(SemaphoreLocker::WRITE_LOCK));
		m_entries.remove(p);
	}
	emit dataChanged();
}

void SearchGeometryState::activate(std::size_t p) {
	{
		auto l(lock(SemaphoreLocker::WRITE_LOCK));
		m_entries[p].active = true;
	}
	emit dataChanged();
}

void SearchGeometryState::deactivate(std::size_t p) {
	{
		auto l(lock(SemaphoreLocker::WRITE_LOCK));
		m_entries[p].active = false;
	}
	emit dataChanged();
}

SemaphoreLocker SearchGeometryState::readLock() const {
	return lock(SemaphoreLocker::READ_LOCK);
}

std::size_t SearchGeometryState::size() const {
	auto l(readLock());
	return m_entries.size();
}

const QString & SearchGeometryState::name(std::size_t p) const {
	auto l(readLock());
	return m_entries.at(p).name;
}

bool SearchGeometryState::active(std::size_t p) const {
	auto l(readLock());
	return m_entries.at(p).active;
}

SearchGeometryState::DataType SearchGeometryState::type(std::size_t p) const {
	auto l(readLock());
	return m_entries.at(p).type;
}

const Marble::GeoDataLineString & SearchGeometryState::data(std::size_t p) const {
	auto l(readLock());
	return m_entries.at(p).data;
}


} //end namespace oscar_gui