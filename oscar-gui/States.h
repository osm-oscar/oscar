#ifndef OSCAR_GUI_STATES_H
#define OSCAR_GUI_STATES_H

#include <marble/GeoDataLineString.h>

namespace oscar_gui {

class SearchGeometryState {
public:
	typedef enum {DT_INVALID, DT_POINT, DT_RECT, DT_PATH, DT_POLYGON} DataType;
public:
	SearchGeometryState() {}
	~SearchGeometryState() {}
public:
	inline void add(const QString & name, const Marble::GeoDataLineString & data, DataType t) {
		m_entries.push_back(Entry(name, data, t));
	}
	inline void remove(std::size_t p) { m_entries.remove(p); }
	inline void activate(std::size_t p) { m_entries[p].active = true; }
	inline void deactivate(std::size_t p) { m_entries[p].active = false; }
public:
	inline std::size_t size() const { return m_entries.size(); }
	inline const QString & name(std::size_t p) const { return m_entries.at(p).name; }
	inline bool active(std::size_t p) const { return m_entries.at(p).active; }
	inline const Marble::GeoDataLineString & data(std::size_t p) const { return m_entries.at(p).data;}
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
};


} //end namespace oscar_gui


#endif
