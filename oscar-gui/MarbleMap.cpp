#include "MarbleMap.h"
#include <sserialize/Static/GeoWay.h>
#include <sserialize/Static/GeoPolygon.h>
#include <sserialize/Static/GeoMultiPolygon.h>
#include <marble/GeoPainter.h>
#include <marble/GeoDataLineString.h>
#include <marble/GeoDataLinearRing.h>
#include <marble/GeoDataPolygon.h>
#include <marble/ViewportParams.h>
#include "SemaphoreLocker.h"

namespace oscar_gui {

QString itemName(const liboscar::Static::OsmItemSet::value_type & item) {
	QString name;
	uint32_t nameIdx = item.findKey("name");
	if (nameIdx != liboscar::Static::OsmItemSet::value_type::npos) {
		name = QString::fromUtf8(item.value(nameIdx).c_str());
	}
	return name;
}

MarbleMap::MyBaseLayer::MyBaseLayer(const QStringList& renderPos, qreal zVal) :
m_zValue(zVal),
m_renderPosition(renderPos)
{
	m_shapeColor[sserialize::spatial::GS_POINT] = Qt::black;
	m_shapeColor[sserialize::spatial::GS_WAY] = Qt::blue;
	m_shapeColor[sserialize::spatial::GS_POLYGON] = Qt::blue;
	m_shapeColor[sserialize::spatial::GS_MULTI_POLYGON] = Qt::blue;
}

QStringList MarbleMap::MyBaseLayer::renderPosition() const {
	return m_renderPosition;
}

qreal MarbleMap::MyBaseLayer::zValue() const {
	return m_zValue;
}

bool MarbleMap::MyBaseLayer::doRender(const liboscar::Static::OsmKeyValueObjectStore::Item & item, const QString & label, Marble::GeoPainter* painter) {
	sserialize::Static::spatial::GeoShape gs = item.geoShape();
	if (!gs.valid())
		return false;
	sserialize::spatial::GeoShapeType gst = gs.type();
	switch (gst) {
	case sserialize::spatial::GS_POINT:
	{
		painter->setPen(m_shapeColor[gst]);
		painter->setBrush(Qt::BrushStyle::SolidPattern);
		const sserialize::Static::spatial::GeoPoint * p = gs.get<sserialize::Static::spatial::GeoPoint>();
		Marble::GeoDataCoordinates gp(p->lon(), p->lat(), 0.0, Marble::GeoDataCoordinates::Degree);
		painter->drawEllipse(gp, 10, 10);
		if (!label.isEmpty()) {
			painter->drawText(gp, label);
		}
	}
		break;
	case sserialize::spatial::GS_WAY:
	{
		painter->setPen(QPen(QBrush(m_shapeColor[gst], Qt::BrushStyle::SolidPattern), 3));
		painter->setBrush(Qt::BrushStyle::SolidPattern);
		const sserialize::Static::spatial::GeoWay * w = gs.get<sserialize::Static::spatial::GeoWay>();
		Marble::GeoDataLineString l;
		for(sserialize::Static::spatial::GeoWay::const_iterator it(w->cbegin()), end(w->cend()); it != end; ++it) {
			sserialize::Static::spatial::GeoPoint p(*it);
			l.append(Marble::GeoDataCoordinates(p.lon(), p.lat(), 0.0, Marble::GeoDataCoordinates::Degree));
		}
		painter->drawPolyline(l);
		if (!label.isEmpty()) {
			painter->drawText(l.first(), label);
		}
	}
		break;
	case sserialize::spatial::GS_POLYGON:
	{
		const sserialize::Static::spatial::GeoPolygon* w = gs.get<sserialize::Static::spatial::GeoPolygon>();
		Marble::GeoDataLinearRing l;
		for(sserialize::Static::spatial::GeoPolygon::const_iterator it(w->cbegin()), end(w->cend()); it != end; ++it) {
			sserialize::Static::spatial::GeoPoint p(*it);
			l.append(Marble::GeoDataCoordinates(p.lon(), p.lat(), 0.0, Marble::GeoDataCoordinates::Degree));
		}
		painter->setPen(QPen(QBrush(m_shapeColor[gst], Qt::BrushStyle::SolidPattern), 1));
		QColor c(m_shapeColor[gst]);
		c.setAlpha(120);
		painter->setBrush( QBrush(c, Qt::Dense4Pattern) );
		painter->drawPolygon(l);
		if (!label.isEmpty()) {
			painter->drawText(l.latLonAltBox().center(), label);
		}
	}
		break;
	case sserialize::spatial::GS_MULTI_POLYGON:
	{ //so far no support for holes, just draw all outer boundaries seperately
		painter->setPen(QPen(QBrush(m_shapeColor[gst], Qt::BrushStyle::SolidPattern), 1));
		QColor c(m_shapeColor[gst]);
		c.setAlpha(120);
		painter->setBrush( QBrush(c, Qt::Dense4Pattern) );
		const sserialize::Static::spatial::GeoMultiPolygon* gmpo = gs.get<sserialize::Static::spatial::GeoMultiPolygon>();
		for(uint32_t i = 0, s = gmpo->outerPolygons().size(); i < s; ++i) {
			sserialize::Static::spatial::GeoPolygon gpo = gmpo->outerPolygons().at(i);
			Marble::GeoDataLinearRing l;
			for(sserialize::Static::spatial::GeoPolygon::const_iterator it(gpo.cbegin()), end(gpo.cend()); it != end; ++it) {
				sserialize::Static::spatial::GeoPoint p(*it);
				l.append(Marble::GeoDataCoordinates(p.lon(), p.lat(), 0.0, Marble::GeoDataCoordinates::Degree));
			}
			painter->drawPolygon(l);
			if (!label.isEmpty()) {
				painter->drawText(l.latLonAltBox().center(), label);
			}
		}
	}
		break;
	default:
		break;
	};
	return true;
}

MarbleMap::MyItemSetLayer::MyItemSetLayer(const QStringList& renderPos, qreal zVal) :
MyLockableBaseLayer(renderPos, zVal),
m_showItemsBegin(0),
m_showItemsEnd(0)
{}

void MarbleMap::MyItemSetLayer::setStore(const liboscar::Static::OsmKeyValueObjectStore& store) {
	SemaphoreLocker locker(lock(), L_WRITE);
	m_store = store;
}

void MarbleMap::MyItemSetLayer::setItemSet(const sserialize::ItemIndex& idx) {
	SemaphoreLocker locker(lock(), L_WRITE);
	m_set = idx;
	if (m_showItemsBegin >= m_set.size() || m_showItemsEnd > m_set.size()) {
		m_showItemsBegin = m_showItemsEnd = 0;
	}
}


void MarbleMap::MyItemSetLayer::setViewRange(uint32_t begin, uint32_t end) {
	SemaphoreLocker locker(lock(), L_WRITE);
	if (m_set.size() > begin && m_set.size() >= end) {
		m_showItemsBegin = begin;
		m_showItemsEnd = end;
	}
}

bool MarbleMap::MyItemSetLayer::render(Marble::GeoPainter* painter, Marble::ViewportParams* /*viewport*/, const QString& /*renderPos*/, Marble::GeoSceneLayer* /*layer*/) {
	SemaphoreLocker locker(lock(), L_READ);
	typedef liboscar::Static::OsmItemSet::value_type Item;
	if (!m_set.size())
		return true;
	uint32_t adminLevelKey = m_store.keyStringTable().find("admin_level");
	for(uint32_t i = m_showItemsBegin; i < m_showItemsEnd; ++i) {
		Item item = m_store.at(m_set.at(i));
		if (item.findKey(adminLevelKey) != Item::npos) {
			MyBaseLayer::doRender(item, itemName(item), painter);
		}
		else {
			MyBaseLayer::doRender(item, QString(), painter);
		}
	}
	return true;
}

MarbleMap::MySingleItemLayer::MySingleItemLayer(const QStringList & renderPos, qreal zVal) :
MyLockableBaseLayer(renderPos, zVal)
{}

void MarbleMap::MySingleItemLayer::setItem(const liboscar::Static::OsmKeyValueObjectStore::Item & item) {
	SemaphoreLocker locker(lock(), L_WRITE);
	m_item = item;
}

bool MarbleMap::MySingleItemLayer::render(Marble::GeoPainter * painter, Marble::ViewportParams* /*viewport*/, const QString& /*renderPos*/, Marble::GeoSceneLayer* /*layer*/) {
	SemaphoreLocker locker(lock(), L_READ);
	if (!m_item.valid())
		return true;
	return MyBaseLayer::doRender(m_item, itemName(m_item), painter);
}


MarbleMap::MarbleMap(): MarbleWidget() {
	m_baseItemLayer = new MyItemSetLayer({"HOVERS_ABOVE_SURFACE"}, 0.0);
	m_highlightItemLayer = new MySingleItemLayer({"HOVERS_ABOVE_SURFACE"}, 1.0);
	m_singleItemLayer = new MySingleItemLayer({"HOVERS_ABOVE_SURFACE"}, 2.0);
	for(uint32_t i(sserialize::spatial::GS_BEGIN), s(sserialize::spatial::GS_END); i < s; ++i) {
		m_highlightItemLayer->shapeColor(i) = QColor(Qt::red);
		m_singleItemLayer->shapeColor(i) = QColor(Qt::darkYellow);
	}
	addLayer(m_baseItemLayer);
	addLayer(m_highlightItemLayer);
	addLayer(m_singleItemLayer);
}

MarbleMap::~MarbleMap() {
	removeLayer(m_baseItemLayer);
	removeLayer(m_highlightItemLayer);
	removeLayer(m_singleItemLayer);
	delete m_baseItemLayer;
	delete m_highlightItemLayer;
	delete m_singleItemLayer;
}

void MarbleMap::itemStoreChanged(const liboscar::Static::OsmKeyValueObjectStore& store) {
	m_store = store;
	m_baseItemLayer->setStore(store);
}

void MarbleMap::viewSetChanged(uint32_t begin, uint32_t end) {
	m_baseItemLayer->setViewRange(begin, end);
	this->update();
}

void MarbleMap::viewSetChanged(const sserialize::ItemIndex& set) {
	m_set = set;
	m_baseItemLayer->setItemSet(m_set);
	if (m_set.size()) {
		m_highlightItemLayer->setItem( m_store.at(m_set.at(0)));
	}
	else {
		m_highlightItemLayer->setItem(liboscar::Static::OsmKeyValueObjectStore::Item());
	}
	this->update();
}

void MarbleMap::zoomToItem(uint32_t itemPos) {
	std::cout << "Zooming to item " << itemPos << std::endl;
	liboscar::Static::OsmKeyValueObjectStore::Item item(m_store.at(m_set.at(itemPos)));
	sserialize::Static::spatial::GeoShape gs(item.geoShape());
	if (gs.valid()) {
		m_highlightItemLayer->setItem(item);
		this->update();
		sserialize::spatial::GeoRect rect(gs.boundary());
		Marble::GeoDataLatLonBox marbleBounds(rect.maxLat(), rect.minLat(), rect.maxLon(), rect.minLon(), Marble::GeoDataCoordinates::Degree);
		centerOn(marbleBounds, true);
	}
	else {
		std::cerr << "Invalid i: " << itemPos << std::endl;
	}
}

void MarbleMap::drawAndZoomTo(const liboscar::Static::OsmKeyValueObjectStore::Item & item) {
	sserialize::Static::spatial::GeoShape gs(item.geoShape());
	if (gs.valid()) {
		m_singleItemLayer->setItem(item);
		this->update();
		sserialize::spatial::GeoRect rect(gs.boundary());
		Marble::GeoDataLatLonBox marbleBounds(rect.maxLat(), rect.minLat(), rect.maxLon(), rect.minLon(), Marble::GeoDataCoordinates::Degree);
		centerOn(marbleBounds, true);
	}
}



}