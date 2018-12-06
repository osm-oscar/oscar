#include "SearchGeometryHelper.h"

namespace oscar_gui {


SearchGeometryHelper::SearchGeometryHelper(const std::shared_ptr<liboscar::Static::OsmCompleter> & cmp) :
m_cmp(cmp)
{}

sserialize::ItemIndex SearchGeometryHelper::cells(const Marble::GeoDataPoint & point) {
	return m_cmp->cqrComplete(toOscarQuery(point).toStdString()).cells();
}

sserialize::ItemIndex SearchGeometryHelper::cells(const Marble::GeoDataLatLonAltBox & rect) {
	return m_cmp->cqrComplete(toOscarQuery(rect).toStdString()).cells();
}

sserialize::ItemIndex SearchGeometryHelper::cells(const Marble::GeoDataLineString & path) {
	return m_cmp->cqrComplete(toOscarQuery(path).toStdString()).cells();
}

sserialize::ItemIndex SearchGeometryHelper::cells(const Marble::GeoDataLinearRing & polygon) {
	return m_cmp->cqrComplete(toOscarQuery(polygon).toStdString()).cells();
}

sserialize::ItemIndex SearchGeometryHelper::triangles(const Marble::GeoDataPoint & point) {
	uint32_t faceId = m_cmp->store().regionArrangement().grid().faceId(
		point.coordinates().latitude(Marble::GeoDataCoordinates::Degree),
		point.coordinates().longitude(Marble::GeoDataCoordinates::Degree)
	);
	if (faceId != m_cmp->store().regionArrangement().tds().NullFace) {
		return sserialize::ItemIndex(std::vector<uint32_t>(1, faceId));
	}
	return sserialize::ItemIndex();
}

sserialize::ItemIndex SearchGeometryHelper::triangles(const Marble::GeoDataLineString & path) {
	std::vector<sserialize::spatial::GeoPoint> tmp(path.size());
	
	for(int i(0), s(path.size()); i < s; ++i) {
		tmp.emplace_back(
			path.at(i).latitude(Marble::GeoDataCoordinates::Degree),
			path.at(i).longitude(Marble::GeoDataCoordinates::Degree)
		);
	}
	
	return m_cmp->store().regionArrangement().trianglesAlongPath(tmp.begin(), tmp.end());
}

QString SearchGeometryHelper::toOscarQuery(const Marble::GeoDataPoint & point) {
	return QString("$path:%1:%2").arg(
		point.coordinates().latitude(Marble::GeoDataCoordinates::Degree),
		point.coordinates().longitude(Marble::GeoDataCoordinates::Degree)
	);
}

QString SearchGeometryHelper::toOscarQuery(const Marble::GeoDataLatLonAltBox & rect) {
	return QString("$rect:%1,%2,%3,%4").arg(
		rect.west(Marble::GeoDataCoordinates::Degree),
		rect.south(Marble::GeoDataCoordinates::Degree),
		rect.east(Marble::GeoDataCoordinates::Degree),
		rect.north(Marble::GeoDataCoordinates::Degree)
	);
}

QString SearchGeometryHelper::toOscarQuery(const Marble::GeoDataLineString & path) {
	if (path.size() < 1) {
		return QString();
	}
	QString tmp("$path:0");
	for(int i(0), s(path.size()); i < s; ++i) {
		tmp.append(',');
		tmp.append(QString::number(path.at(i).latitude(Marble::GeoDataCoordinates::Degree)));
		tmp.append(',');
		tmp.append(QString::number(path.at(i).longitude(Marble::GeoDataCoordinates::Degree)));
	}
	return tmp;
}

QString SearchGeometryHelper::toOscarQuery(const Marble::GeoDataLinearRing & polygon) {
	if (polygon.size() < 3) {
		return QString();
	}
	QString tmp("$poly:");
	for(int i(0), s(polygon.size()); i < s; ++i) {
		tmp.append(QString::number(polygon.at(i).latitude(Marble::GeoDataCoordinates::Degree)));
		tmp.append(',');
		tmp.append(QString::number(polygon.at(i).longitude(Marble::GeoDataCoordinates::Degree)));
		tmp.append(',');
	}
	tmp.truncate(tmp.size()-1);
	return tmp;
}

}//end namespace oscar_gui
