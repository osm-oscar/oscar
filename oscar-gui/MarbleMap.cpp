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

MarbleMap::MyCellLayer::MyCellLayer(const QStringList& renderPos, qreal zVal) :
MyLockableBaseLayer(renderPos, zVal)
{}

void MarbleMap::MyCellLayer::calcCellColors() {
	auto cg = m_store.cellGraph();
	std::vector<uint8_t> tmpColors(cg.size(), Qt::GlobalColor::color0);
	for(uint32_t i(0), s(cg.size()); i < s; ++i) {
		auto cn = cg.node(i);
		if (cn.size()) {
			uint32_t neighborColors = 0;
			for(uint32_t j(0), js(cn.size()); j < js; ++j) {
				neighborColors |= static_cast<uint32_t>(1) << tmpColors.at(cn.neighborId(j));
			}
			for(uint8_t color(Qt::GlobalColor::red); color < Qt::GlobalColor::darkYellow; ++color) {
				if ((neighborColors & (static_cast<uint32_t>(1) << color)) == 0) {
					tmpColors.at(i) = color;
					break;
				}
			}
			//now other color was found
			if (tmpColors.at(i) == Qt::GlobalColor::color0) {
				tmpColors.at(i) = Qt::GlobalColor::black;
			}
		}
		else {
			tmpColors.at(i) = Qt::GlobalColor::white;
		}
	}
	m_cellColors.clear();
	m_cellColors.resize(cg.size());
	for(uint32_t i(0), s(cg.size()); i < s; ++i) {
		m_cellColors.at(i) = QColor((Qt::GlobalColor) tmpColors.at(i));
	}
}

bool MarbleMap::MyCellLayer::doRender(const std::vector<uint32_t> & faces, const QColor& color, Marble::GeoPainter* painter) {
	QColor lineColor(color);
	lineColor.setAlpha(255);
	painter->setPen(QPen(QBrush(lineColor, Qt::BrushStyle::SolidPattern), 1));
	QColor fillColor(lineColor);
	fillColor.setAlpha(0.7*255);
	
	for(uint32_t faceId : faces) {
		Marble::GeoDataLinearRing l;
		sserialize::Static::spatial::Triangulation::Face fh = m_store.regionArrangement().tds().face(faceId);
		for(int j(0); j < 3; ++j) {
			sserialize::Static::spatial::Triangulation::Point p(fh.point(j));
			l.append(Marble::GeoDataCoordinates(p.lon(), p.lat(), 0.0, Marble::GeoDataCoordinates::Degree));
		}
		painter->setBrush( QBrush(fillColor, Qt::SolidPattern) );
		painter->drawPolygon(l);
	}
	return true;
}

bool MarbleMap::MyCellLayer::render(Marble::GeoPainter* painter, Marble::ViewportParams* /*viewport*/, const QString& /*renderPos*/, Marble::GeoSceneLayer* /*layer*/) {
	SemaphoreLocker locker(lock(), L_READ);
	bool ok = true;
	for(const std::pair<const uint32_t, Graph> x : m_cgm) {
		ok = doRender(x.second, m_cellColors.at(x.first), painter) && ok;
	}
	return ok;
}

void MarbleMap::MyCellLayer::setCells(const sserialize::ItemIndex& cells) {
	SemaphoreLocker locker(lock(), L_WRITE);
	GraphMap tmp;
	for(uint32_t cellId : cells) {
		if (m_cgm.count(cellId)) {
			tmp[cellId] = std::move(m_cgm[cellId]);
		}
		else {
			std::vector<uint32_t> & d = tmp[cellId];
			struct MyOutIt {
				std::vector<uint32_t> * cells;
				MyOutIt & operator++() { return *this; }
				MyOutIt & operator*() { return *this; }
				MyOutIt & operator=(const sserialize::Static::spatial::TriangulationGeoHierarchyArrangement::CFGraph::Face & f) {
					cells->push_back(f.id());
					return *this;
				}
			};
			MyOutIt outIt({.cells = &d});
			m_store.regionArrangement().cfGraph(cellId).visit(outIt);
		}
	}
	m_cgm = std::move(tmp);
}

void MarbleMap::MyCellLayer::setStore(const liboscar::Static::OsmKeyValueObjectStore& store) {
	SemaphoreLocker locker(lock(), L_WRITE);
	m_cgm.clear();
	m_store = store;
	calcCellColors();
}

MarbleMap::MarbleMap(): MarbleWidget() {
	m_baseItemLayer = new MyItemSetLayer({"HOVERS_ABOVE_SURFACE"}, 0.0);
	m_cellLayer = new MyCellLayer({"HOVERS_ABOVE_SURFACE"}, 1.0);
	m_highlightItemLayer = new MySingleItemLayer({"HOVERS_ABOVE_SURFACE"}, 2.0);
	m_singleItemLayer = new MySingleItemLayer({"HOVERS_ABOVE_SURFACE"}, 3.0);
	for(uint32_t i(sserialize::spatial::GS_BEGIN), s(sserialize::spatial::GS_END); i < s; ++i) {
		m_highlightItemLayer->shapeColor(i) = QColor(Qt::red);
		m_singleItemLayer->shapeColor(i) = QColor(Qt::darkYellow);
	}
	addLayer(m_baseItemLayer);
	addLayer(m_cellLayer);
	addLayer(m_highlightItemLayer);
	addLayer(m_singleItemLayer);
}

MarbleMap::~MarbleMap() {
	removeLayer(m_baseItemLayer);
	removeLayer(m_highlightItemLayer);
	removeLayer(m_singleItemLayer);
	removeLayer(m_cellLayer);
	delete m_baseItemLayer;
	delete m_highlightItemLayer;
	delete m_singleItemLayer;
	delete m_cellLayer;
}

void MarbleMap::itemStoreChanged(const liboscar::Static::OsmKeyValueObjectStore& store) {
	m_store = store;
	m_baseItemLayer->setStore(store);
	m_cellLayer->setStore(store);
}

void MarbleMap::activeCellsChanged(const sserialize::ItemIndex& cells) {
	m_cellLayer->setCells(cells);
	this->update();
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
		std::cerr << "Invalid item: " << itemPos << std::endl;
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