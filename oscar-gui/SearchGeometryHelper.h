#ifndef OSCAR_GUI_SEARCH_GEOMETRY_HELPER_H
#define OSCAR_GUI_SEARCH_GEOMETRY_HELPER_H

#include <marble/GeoDataPoint.h>
#include <marble/GeoDataLineString.h>
#include <marble/GeoDataLinearRing.h>
#include <marble/GeoDataLatLonAltBox.h>

#include <liboscar/StaticOsmCompleter.h>

#include "States.h"

namespace oscar_gui {

class SearchGeometryHelper {
public:
	SearchGeometryHelper(const std::shared_ptr<liboscar::Static::OsmCompleter> & cmp);
public:
	sserialize::ItemIndex cells(const Marble::GeoDataPoint & point);
	sserialize::ItemIndex cells(const Marble::GeoDataLatLonAltBox & rect);
	sserialize::ItemIndex cells(const Marble::GeoDataLineString & path);
	sserialize::ItemIndex cells(const Marble::GeoDataLinearRing & polygon);
public:
	sserialize::ItemIndex triangles(const Marble::GeoDataPoint & point);
	sserialize::ItemIndex triangles(const Marble::GeoDataLineString & path);
public:
	QString toOscarQuery(const Marble::GeoDataPoint & point);
	QString toOscarQuery(const Marble::GeoDataLatLonAltBox & rect);
	QString toOscarQuery(const Marble::GeoDataLineString & path);
	QString toOscarQuery(const Marble::GeoDataLinearRing & point);
private:
	std::shared_ptr<liboscar::Static::OsmCompleter> m_cmp;
};

}//end namespace oscar_gui

#endif
