#ifndef OSCAR_GUI_STATES_H
#define OSCAR_GUI_STATES_H

#include <marble/GeoDataLineString.h>

#include <memory>

#include <liboscar/StaticOsmCompleter.h>

#include "SemaphoreLocker.h"



namespace oscar_gui {

class SearchGeometryState: public QObject {
Q_OBJECT
public:
	typedef enum {DT_INVALID, DT_POINT, DT_RECT, DT_PATH, DT_POLYGON} DataType;
	typedef enum {AT_NONE=0, AT_SHOW=0x1, AT_TRIANGLES=0x2, AT_CELLS=0x4} ActiveType;
public:
	SearchGeometryState();
	~SearchGeometryState();
public:
	void add(const QString & name, const Marble::GeoDataLineString & data, DataType t);
	void remove(std::size_t p);
	void activate(std::size_t p, ActiveType at);
	void deactivate(std::size_t p, ActiveType at);
public:
	SemaphoreLocker readLock() const;
	std::size_t size() const;
	const QString & name(std::size_t p) const;
	ActiveType active(std::size_t p) const;
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
		int active;
		DataType type;
		Entry() : active(AT_NONE), type(DT_INVALID) {}
		Entry(const QString & name, const Marble::GeoDataLineString & data, DataType t) :
		name(name), data(data), active(AT_NONE), type(t)
		{}
	};
private:
	QVector<Entry> m_entries;
	mutable QSemaphore m_sem;
};


struct States {
	std::shared_ptr<liboscar::Static::OsmCompleter> cmp;
	std::shared_ptr<SearchGeometryState> sgs;
	
	States(const std::shared_ptr<liboscar::Static::OsmCompleter> & cmp) : cmp(cmp) {}
};

} //end namespace oscar_gui


#endif
