#ifndef OSCAR_GUI_STATES_H
#define OSCAR_GUI_STATES_H

#include <marble/GeoDataLineString.h>

#include <memory>

#include "SemaphoreLocker.h"

namespace oscar_gui {

class SearchGeometryState: public QObject {
Q_OBJECT
public:
	typedef enum {DT_INVALID, DT_POINT, DT_RECT, DT_PATH, DT_POLYGON} DataType;
public:
	SearchGeometryState();
	~SearchGeometryState();
public:
	void add(const QString & name, const Marble::GeoDataLineString & data, DataType t);
	void remove(std::size_t p);
	void activate(std::size_t p);
	void deactivate(std::size_t p);
public:
	SemaphoreLocker readLock() const;
	std::size_t size() const;
	const QString & name(std::size_t p) const;
	bool active(std::size_t p) const;
	DataType type(std::size_t p) const;
	const Marble::GeoDataLineString & data(std::size_t p) const;
signals:
	void dataChanged();
private:
	SemaphoreLocker lock(SemaphoreLocker::Type t) const;
private:
	struct Entry {
		QString name;
		Marble::GeoDataLineString data;
		bool active;
		DataType type;
		Entry() : active(false), type(DT_INVALID) {}
		Entry(const QString & name, const Marble::GeoDataLineString & data, DataType t) :
		name(name), data(data), active(false), type(t)
		{}
	};
private:
	QVector<Entry> m_entries;
	mutable QSemaphore m_sem;
};


struct States {
	std::shared_ptr<SearchGeometryState> sgs;
};

} //end namespace oscar_gui


#endif
