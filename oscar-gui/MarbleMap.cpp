#include "MarbleMap.h"
#include <marble/GeoPainter.h>
#include <marble/GeoDataPoint.h>
#include <marble/GeoDataLineString.h>
#include <marble/GeoDataLinearRing.h>
#include <marble/GeoDataPolygon.h>
#include <marble/ViewportParams.h>
#include <marble/MarbleWidgetPopupMenu.h>
#include <marble/MarbleWidgetInputHandler.h>
#include <QAction>
#include <QtGui/QHBoxLayout>

namespace oscar_gui {


MarbleMap::MyBaseLayer::MyBaseLayer(const QStringList& renderPos, qreal zVal, const DataPtr & data) :
m_zValue(zVal),
m_renderPosition(renderPos),
m_data(data),
m_opacity(255)
{}

QStringList MarbleMap::MyBaseLayer::renderPosition() const {
	return m_renderPosition;
}

qreal MarbleMap::MyBaseLayer::zValue() const {
	return m_zValue;
}

bool MarbleMap::MyBaseLayer::doRender(const CFGraph & cg, const QBrush & brush, const QString & label, Marble::GeoPainter* painter) {
	if (!cg.size()) {
		return true;
	}
	
	cg.visitCB([this, &label, painter, &brush](const Face & f) {
		this->doRender(f, brush, label, painter);
	});
	return true;
}


bool MarbleMap::MyBaseLayer::doRender(const Face & f, const QBrush & brush, const QString& label, Marble::GeoPainter* painter) {
	Marble::GeoDataLinearRing l;
	for(int i(0); i < 3; ++i) {
		auto p = f.point(i);
		l.append(Marble::GeoDataCoordinates(p.lon(), p.lat(), 0.0, Marble::GeoDataCoordinates::Degree));
	}
	painter->setBrush( brush );
	painter->drawPolygon(l);
	if (!label.isEmpty()) {
		painter->drawText(l.latLonAltBox().center(), label);
	}
	return true;
}

//BEGIN: MyTriangleLayer

MarbleMap::MyTriangleLayer::MyTriangleLayer(const QStringList & renderPos, qreal zVal, const DataPtr & data) :
MyBaseLayer(renderPos, zVal, data)
{}

bool MarbleMap::MyTriangleLayer::render(Marble::GeoPainter * painter, Marble::ViewportParams* /*viewport*/, const QString& /*renderPos*/, Marble::GeoSceneLayer* /*layer*/) {
	QColor lineColor(Qt::blue);
	lineColor.setAlpha(255);
	painter->setPen(QPen(QBrush(lineColor, Qt::BrushStyle::SolidPattern), 1));
	QColor fillColor(lineColor);
	fillColor.setAlpha(opacity());
	QBrush brush(fillColor, Qt::SolidPattern);


	for(uint32_t triangleId : m_cgi) {
		if( !MyBaseLayer::doRender(data()->trs.tds().face(triangleId), brush, "", painter)) {
			return false;
		}
	}
	return true;
}

void MarbleMap::MyTriangleLayer::addTriangle(uint32_t triangleId) {
	m_cgi.insert(triangleId);
}

void MarbleMap::MyTriangleLayer::removeTriangle(uint32_t triangleId) {
	m_cgi.erase(triangleId);
}

void MarbleMap::MyTriangleLayer::clear() {
	m_cgi.clear();
}

//END: MyTriangleLayer


//BEGIN: MyCellLayer

MarbleMap::MyCellLayer::MyCellLayer(const QStringList & renderPos, qreal zVal, const DataPtr & data) :
MyBaseLayer(renderPos, zVal, data),
m_colorScheme(CS_DIFFERENT)
{}

bool MarbleMap::MyCellLayer::render(Marble::GeoPainter * painter, Marble::ViewportParams* /*viewport*/, const QString& /*renderPos*/, Marble::GeoSceneLayer* /*layer*/) {
	for(std::pair<const uint32_t, CFGraph> & gi : m_cgi) {
		uint32_t cellId = gi.first;
		QColor lineColor(data()->cellColor(cellId, colorScheme() ));
		lineColor.setAlpha(255);
		painter->setPen(QPen(QBrush(lineColor, Qt::BrushStyle::SolidPattern), 1));
		QColor fillColor(lineColor);
		fillColor.setAlpha(opacity());
		QBrush brush(fillColor, Qt::SolidPattern);

		if( !MyBaseLayer::doRender(gi.second, brush, "", painter)) {
			return false;
		}
	}
	return true;
}

void MarbleMap::MyCellLayer::addCell(uint32_t cellId) {
	if (m_cgi.count(cellId)) {
		return;
	}
	m_cgi[cellId] = data()->trs.cfGraph(cellId);
}

void MarbleMap::MyCellLayer::removeCell(uint32_t cellId) {
	m_cgi.erase(cellId);
}

void MarbleMap::MyCellLayer::clear() {
	m_cgi.clear();
}

//END: MyCellLayer

//BEGIN MyPathLayer

MarbleMap::MyPathLayer::MyPathLayer(const QStringList& renderPos, qreal zVal, const DataPtr & trs) :
MyLockableBaseLayer(renderPos, zVal, trs)
{}

bool MarbleMap::MyPathLayer::render(Marble::GeoPainter* painter, Marble::ViewportParams* /*viewport*/, const QString& /*renderPos*/, Marble::GeoSceneLayer* /*layer*/) {
	if (m_path.size()) {
		painter->setPen(QPen(QBrush(Qt::red, Qt::BrushStyle::SolidPattern), 3));
		painter->setBrush(Qt::BrushStyle::SolidPattern);
		painter->drawPolyline(m_path);
	}
	return true;
}

void MarbleMap::MyPathLayer::changePath(const sserialize::spatial::GeoWay& w) {
	m_path.clear();
	for(auto p : w) {
		m_path.append(Marble::GeoDataCoordinates(p.lon(), p.lat(), 0.0, Marble::GeoDataCoordinates::Degree));
	}
}

//END MyPathLayer

//BEGIN MyGeometryLayer

MarbleMap::MyGeometryLayer::MyGeometryLayer(const QStringList & renderPos, qreal zVal, const DataPtr & data) :
MyLockableBaseLayer(renderPos, zVal, data)
{}

MarbleMap::MyGeometryLayer::~MyGeometryLayer() {}

bool MarbleMap::MyGeometryLayer::render(Marble::GeoPainter* painter, Marble::ViewportParams* /*viewport*/, const QString& /*renderPos*/, Marble::GeoSceneLayer* /*layer*/) {
	QPen pen(QColor(Qt::blue));
	QColor fillColor(0, 0, 255, 50);
	QBrush brush(fillColor);
	
	painter->setPen(pen);
	painter->setBrush(brush);
	
	auto lock(data()->sgs->readLock());
	for(uint32_t i(0), s(data()->sgs->size()); i < s; ++i) {
		auto at = data()->sgs->active(i);
		if (at & SearchGeometryState::AT_SHOW) {
			const auto & d = data()->sgs->data(i);
			switch (data()->sgs->type(i)) {
			case SearchGeometryState::DT_POINT:
				painter->drawEllipse(d.first(), 20, 20);
				break;
			case SearchGeometryState::DT_RECT:
				painter->drawRect(
					d.latLonAltBox().center(),
					d.latLonAltBox().height(Marble::GeoDataCoordinates::Degree),
					d.latLonAltBox().width(Marble::GeoDataCoordinates::Degree),
					true
				);
				break;
			case SearchGeometryState::DT_PATH:
				painter->drawPolyline(d);
				break;
			case SearchGeometryState::DT_POLYGON:
				painter->drawPolygon(d);
				break;
			default:
				break;
			}
		}
		if (at & SearchGeometryState::AT_TRIANGLES) {
			const auto & triangles = data()->sgs->triangles(i);
			for(uint32_t faceId : triangles) {
				this->doRender(data()->trs.tds().face(faceId), brush, QString(), painter);
			}
		}
		if (at & SearchGeometryState::AT_CELLS) {
			const auto & cells = data()->sgs->cells(i);
			for(uint32_t cellId : cells) {
				this->doRender(data()->trs.cfGraph(cellId), brush, QString(), painter);
			}
		}
	}
	return true;
}


//END MyGeometryLayer

//BEGIN MyInputSearchGeometryLayer

MarbleMap::MyInputSearchGeometryLayer::MyInputSearchGeometryLayer(const QStringList& renderPos, qreal zVal, const oscar_gui::MarbleMap::DataPtr& trs) :
MyBaseLayer(renderPos, zVal, trs)
{}

MarbleMap::MyInputSearchGeometryLayer::~MyInputSearchGeometryLayer() {}

void MarbleMap::MyInputSearchGeometryLayer::update(const Marble::GeoDataLineString & d) {
	m_d = d;
}

bool MarbleMap::MyInputSearchGeometryLayer::render(Marble::GeoPainter* painter, Marble::ViewportParams*, const QString&, Marble::GeoSceneLayer*) {
	if (!m_d.size()) {
		return true;
	}
	QBrush brush(QColor(Qt::red));
	painter->setBrush(brush);
	
	if (m_d.size() == 1) {
		painter->drawPoint(m_d.first());
	}
	else {
		painter->drawPolyline(m_d);
	}
	return true;
}



//END MyInputSearchGeometryLayer

//BEGIN Data


MarbleMap::Data::Data(const liboscar::Static::OsmKeyValueObjectStore& store, const oscar_gui::States& states) :
store(store),
trs(store.regionArrangement()),
cg(store.cellGraph()),
sgs(states.sgs)
{
	std::vector<uint8_t> tmpColors(cg.size(), Qt::GlobalColor::color0);
	for(uint32_t i(0), s(cg.size()); i < s; ++i) {
		auto cn = cg.node(i);
		if (cn.size()) {
			uint32_t neighborColors = 0;
			for(auto nn : cn) {
				neighborColors |= static_cast<uint32_t>(1) << tmpColors.at(nn.cellId());
			}
			//start with blue, since red is used for removed edges, and green for edges intersected by removed edges
			for(uint8_t color(Qt::GlobalColor::blue); color < Qt::GlobalColor::darkYellow; ++color) {
				if ((neighborColors & (static_cast<uint32_t>(1) << color)) == 0) {
					tmpColors.at(i) = color;
					break;
				}
			}
			//no other color was found
			if (tmpColors.at(i) == Qt::GlobalColor::color0) {
				tmpColors.at(i) = Qt::GlobalColor::black;
			}
		}
		else {
			tmpColors.at(i) = Qt::GlobalColor::white;
		}
	}
	for(uint32_t i(0), s(cellColors.size()); i < s; ++i) {
		cellColors.at(i) = QColor((Qt::GlobalColor) tmpColors.at(i));
	}
}

//END Data

MarbleMap::MarbleMap(const liboscar::Static::OsmKeyValueObjectStore & store, const States & states):
m_map( new Marble::MarbleWidget() ),
m_data(new Data(store, states))
{
	m_map->setMapThemeId("earth/openstreetmap/openstreetmap.dgml");

	m_triangleLayer = new MarbleMap::MyTriangleLayer({"HOVERS_ABOVE_SURFACE"}, 0.0, m_data);
	m_cellLayer = new MarbleMap::MyCellLayer({"HOVERS_ABOVE_SURFACE"}, 0.0, m_data);
	m_geometryLayer = new MarbleMap::MyGeometryLayer({"HOVERS_ABOVE_SURFACE"}, 0.0, m_data);
	m_isgLayer = new MarbleMap::MyInputSearchGeometryLayer({"HOVERS_ABOVE_SURFACE"}, 0.0, m_data);
	
	m_map->addLayer(m_triangleLayer);
	m_map->addLayer(m_cellLayer);
	m_map->addLayer(m_geometryLayer);
	m_map->addLayer(m_isgLayer);


	QAction * beginSearchGeometry = new QAction("Begin search geometry", this);
	m_map->popupMenu()->addAction(Qt::MouseButton::RightButton, beginSearchGeometry);
	
	QAction * addToSearchGeometry = new QAction("Add to search geometry", this);
	m_map->popupMenu()->addAction(Qt::MouseButton::RightButton, addToSearchGeometry);
	
	QAction * endAsPoint = new QAction("End search geometry as point", this);
	m_map->popupMenu()->addAction(Qt::MouseButton::RightButton, endAsPoint);
	
	QAction * endAsRect = new QAction("End search geometry as rectangle", this);
	m_map->popupMenu()->addAction(Qt::MouseButton::RightButton, endAsRect);

	QAction * endAsPath = new QAction("End search geometry as path", this);
	m_map->popupMenu()->addAction(Qt::MouseButton::RightButton, endAsPath);
	
	QAction * endAsPolygon = new QAction("End search geometry as polygon", this);
	m_map->popupMenu()->addAction(Qt::MouseButton::RightButton, endAsPolygon);
	
	//get mouse clicks
	connect(m_map->inputHandler(), SIGNAL(rmbRequest(int,int)), this, SLOT(rmbRequested(int,int)));
	connect(beginSearchGeometry, SIGNAL(triggered(bool)), this, SLOT(beginSearchGeometryTriggered()));
	connect(addToSearchGeometry, SIGNAL(triggered(bool)), this, SLOT(addToSearchGeometryTriggered()));
	
	connect(endAsPoint, SIGNAL(triggered(bool)), this, SLOT(endSearchGeometryAsPointTriggered()));
	connect(endAsRect, SIGNAL(triggered(bool)), this, SLOT(endSearchGeometryAsRectTriggered()));
	connect(endAsPath, SIGNAL(triggered(bool)), this, SLOT(endSearchGeometryAsPathTriggered()));
	connect(endAsPolygon, SIGNAL(triggered(bool)), this, SLOT(endSearchGeometryAsPolygonTriggered()));
	
	//data changes
	connect(states.sgs.get(), SIGNAL(dataChanged(int)), this, SLOT(geometryDataChanged(int)));
	
	QHBoxLayout * mainLayout = new QHBoxLayout();
	mainLayout->addWidget(m_map);
	this->setLayout(mainLayout);
	
	sserialize::spatial::GeoRect bounds = states.cmp->store().boundary();
	Marble::GeoDataLatLonBox mbounds(bounds.maxLat()*Marble::DEG2RAD, bounds.minLat()*Marble::DEG2RAD, bounds.minLon()*Marble::DEG2RAD+M_PI, bounds.maxLon()*Marble::DEG2RAD+M_PI, Marble::GeoDataCoordinates::Radian);
	qDebug() << "Zoom to" << mbounds.toString(Marble::GeoDataCoordinates::Degree);
	this->zoomTo(mbounds);
}

QColor MarbleMap::Data::cellColor(uint32_t cellId, int cs) const {
	if (cs == CS_DIFFERENT) {
		return cellColors.at(cellId);
	}
	return QColor(Qt::blue);
}

MarbleMap::~MarbleMap() {
	m_map->removeLayer(m_triangleLayer);
	m_map->removeLayer(m_cellLayer);
	m_map->removeLayer(m_geometryLayer);
	m_map->removeLayer(m_isgLayer);
	
	delete m_triangleLayer;
	delete m_cellLayer;
	delete m_geometryLayer;
	delete m_isgLayer;
}

void MarbleMap::zoomTo(const Marble::GeoDataLatLonBox& bbox) {
	m_map->centerOn(bbox, true);
}

void MarbleMap::zoomToTriangle(uint32_t triangleId) {
	auto p = m_data->trs.tds().face( triangleId ).centroid();
	Marble::GeoDataLatLonBox marbleBounds(p.lat(), p.lon(), p.lat(), p.lon(), Marble::GeoDataCoordinates::Degree);
	zoomTo(marbleBounds);
}

void MarbleMap::zoomToCell(uint32_t cellId) {
	zoomToTriangle(  m_data->trs.faceIdFromCellId(cellId) );
}

void MarbleMap::addTriangle(uint32_t triangleId) {
	if (m_triangleLayer) {
		m_triangleLayer->addTriangle(triangleId);
		this->update();
	}
}

void MarbleMap::removeTriangle(uint32_t triangleId) {
	if (m_triangleLayer) {
		m_triangleLayer->removeTriangle(triangleId);
		this->update();
	}
}

void MarbleMap::clearTriangles() {
	if (m_triangleLayer) {
		m_triangleLayer->clear();
		this->update();
	}
}

void MarbleMap::addCell(uint32_t cellId) {
	if (m_cellLayer) {
		m_cellLayer->addCell(cellId);
		this->update();
	}
}

void MarbleMap::removeCell(uint32_t cellId) {
	if (m_cellLayer) {
		m_cellLayer->removeCell(cellId);
		this->update();
	}
}

void MarbleMap::clearCells() {
	if (m_cellLayer) {
		m_cellLayer->clear();
		this->update();
	}
}

void MarbleMap::showPath(const sserialize::spatial::GeoWay& p) {
	m_pathLayer->changePath(p);
	this->update();
}

void MarbleMap::geometryDataChanged(int) {
	this->update();
}

void MarbleMap::setCellOpacity(int cellOpacity) {
	m_cellOpacity = cellOpacity;
	if (m_cellLayer) {
		m_cellLayer->opacity(m_cellOpacity);
		this->update();
	}
}

void MarbleMap::setColorScheme(int colorScheme) {
	m_colorScheme = colorScheme;
	if (m_cellLayer) {
		m_cellLayer->setColorScheme(m_colorScheme);
		this->update();
	}
}

void MarbleMap::rmbRequested(int x, int y) {
	m_map->geoCoordinates(x, y, m_lastRmbClickLon, m_lastRmbClickLat, Marble::GeoDataCoordinates::Radian);
}

void MarbleMap::beginSearchGeometryTriggered() {
	m_isg.clear();
	m_isgLayer->update(m_isg);
}

void MarbleMap::addToSearchGeometryTriggered() {
	Marble::GeoDataCoordinates pos(m_lastRmbClickLon, m_lastRmbClickLat, Marble::GeoDataCoordinates::Radian);
	qDebug() << "Adding " << pos.toString() << " to search geometry from lon=" << m_lastRmbClickLon << "; lat=" << m_lastRmbClickLat;
	m_isg.append(pos);
	m_isgLayer->update(m_isg);
	this->update();
}

void MarbleMap::endSearchGeometryAsPointTriggered() {
	if (m_isg.size() >= 1) {
		Marble::GeoDataLineString tmp;
		tmp.append(m_isg.first());
		m_data->sgs->add("Point", tmp, SearchGeometryState::DT_POINT);
	}
	beginSearchGeometryTriggered();
}

void MarbleMap::endSearchGeometryAsRectTriggered() {
	if (m_isg.size() >= 2) {
		m_data->sgs->add("Rectangle", m_isg, SearchGeometryState::DT_RECT);
	}
	beginSearchGeometryTriggered();
}

void MarbleMap::endSearchGeometryAsPathTriggered() {
	if (m_isg.size() >= 1) {
		m_data->sgs->add("Path", m_isg, SearchGeometryState::DT_PATH);
	}
	beginSearchGeometryTriggered();
}

void MarbleMap::endSearchGeometryAsPolygonTriggered() {
	if (m_isg.size() >= 3) {
		m_data->sgs->add("Polygon", m_isg, SearchGeometryState::DT_POLYGON);
	}
	beginSearchGeometryTriggered();
}




} //end namespace oscar_gui