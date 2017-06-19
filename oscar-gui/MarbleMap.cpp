#include "MarbleMap.h"
#include <marble/GeoPainter.h>
#include <marble/GeoDataLineString.h>
#include <marble/GeoDataLinearRing.h>
#include <marble/GeoDataPolygon.h>
#include <marble/ViewportParams.h>
#include <marble/MarbleWidgetPopupMenu.h>
#include <marble/MarbleWidgetInputHandler.h>
#include <QAction>

namespace oscar_gui {


MarbleMap::MyBaseLayer::MyBaseLayer(const QStringList& renderPos, qreal zVal, const DataPtr & data) :
m_zValue(zVal),
m_renderPosition(renderPos),
m_data(data),
m_cellOpacity(255)
{}

QStringList MarbleMap::MyBaseLayer::renderPosition() const {
	return m_renderPosition;
}

qreal MarbleMap::MyBaseLayer::zValue() const {
	return m_zValue;
}

bool MarbleMap::MyBaseLayer::doRender(const CFGraph & cg, const QString & label, Marble::GeoPainter* painter) {
	if (!cg.size()) {
		return true;
	}
	uint32_t cellId = cg.cellId();
	QColor lineColor(m_data->cellColors[m_colorScheme].at(cellId));
	lineColor.setAlpha(255);
	painter->setPen(QPen(QBrush(lineColor, Qt::BrushStyle::SolidPattern), 1));
	QColor fillColor(lineColor);
	fillColor.setAlpha(m_cellOpacity);
	QBrush brush(fillColor, Qt::SolidPattern);
	
	cg.visit([this, &label, painter, &brush](const Face & f) {
		this->doRender(f, brush, label, painter);
	});
	return true;
}


bool MarbleMap::MyBaseLayer::doRender(const Face & f, const QBrush & brush, const QString& label, Marble::GeoPainter* painter) {
	Marble::GeoDataLinearRing l;
	for(int i(0); i < 3; ++i) {
		auto p = f.point(i);
		l.append(Marble::GeoDataCoordinates(p.lat(), p.lon(), 0.0, Marble::GeoDataCoordinates::Degree));
	}
	painter->setBrush( brush );
	painter->drawPolygon(l);
	if (!label.isEmpty()) {
		painter->drawText(l.latLonAltBox().center(), label);
	}
}

//BEGIN: MyTriangleLayer

MarbleMap::MyTriangleLayer::MyTriangleLayer(const QStringList & renderPos, qreal zVal, const DataPtr & data) :
MyBaseLayer(renderPos, zVal, data)
{}

bool MarbleMap::MyTriangleLayer::render(Marble::GeoPainter * painter, Marble::ViewportParams* /*viewport*/, const QString& /*renderPos*/, Marble::GeoSceneLayer* /*layer*/) {
	for(uint32_t & triangleId : m_cgi) {
		if( !MyBaseLayer::doRender(data()->trs.tds().face(triangleId), "", painter)) {
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
MyBaseLayer(renderPos, zVal, data)
{}

bool MarbleMap::MyCellLayer::render(Marble::GeoPainter * painter, Marble::ViewportParams* /*viewport*/, const QString& /*renderPos*/, Marble::GeoSceneLayer* /*layer*/) {
	for(std::pair<const uint32_t, CFGraph> & gi : m_cgi) {
		if( !MyBaseLayer::doRender(gi.second, "", painter)) {
			return false;
		}
	}
	return true;
}

void MarbleMap::MyCellLayer::addCell(uint32_t cellId) {
	if (m_cgi.count(cellId)) {
		return;
	}
	m_cgi[cellId] = data()->trs().cfGraph(cellId);
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

//BEGIN Data


MarbleMap::Data::Data(const TriangulationGeoHierarchyArrangement & trs, const TracGraph & cg) :
trs(trs),
cg(cg)
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

MarbleMap::MarbleMap(const TriangulationGeoHierarchyArrangement & trs, const TracGraph & cg):
MarbleWidget(),
m_data(new Data(trs, cg))
{
	m_triangleLayer = new MarbleMap::MyTriangleLayer({"HOVERS_ABOVE_SURFACE"}, 0.0, m_data);
	m_cellLayer = new MarbleMap::MyCellLayer({"HOVERS_ABOVE_SURFACE"}, 0.0, m_data);
	
	addLayer(m_triangleLayer);
	addLayer(m_cellLayer);

	QAction * toggleCellAction = new QAction("Toggle Cell", this);
	popupMenu()->addAction(Qt::MouseButton::RightButton, toggleCellAction);

	//get mouse clicks
	connect(this->inputHandler(), SIGNAL(rmbRequest(int,int)), this, SLOT(rmbRequested(int,int)));
	
	connect(toggleCellAction, SIGNAL(triggered(bool)), this, SLOT(toggleCellTriggered()));
}

QColor MarbleMap::Data::cellColor(uint32_t cellId, MarbleMap::ColorScheme cs) const {
	if (cs == CS_DIFFERENT) {
		return cellColors.at(cellId);
	}
	return QColor(Qt::blue);
}

MarbleMap::~MarbleMap() {
	removeLayer(m_triangleLayer);
	removeLayer(m_cellLayer);
	
	delete m_triangleLayer;
	delete m_cellLayer;
}


void MarbleMap::zoomToTriangle(uint32_t triangleId) {
	auto p = m_data->trs.tds().face( triangleId ).centroid();
	Marble::GeoDataLatLonBox marbleBounds(p.lat(), p.lon(), p.lat(), p.lon(), Marble::GeoDataCoordinates::Degree);
	centerOn(marbleBounds, true);
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

void MarbleMap::setCellOpacity(int cellOpacity) {
	m_cellOpacity = cellOpacity;
	if (m_cellLayer) {
		m_cellLayer->setCellOpacity(m_cellOpacity);
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
	this->geoCoordinates(x, y, m_lastRmbClickLon, m_lastRmbClickLat, Marble::GeoDataCoordinates::Degree);
}

void MarbleMap::toggleCellTriggered() {
	emit toggleCellClicked(m_lastRmbClickLat, m_lastRmbClickLon);
}

} //end namespace oscar_gui