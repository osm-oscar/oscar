#ifndef OSCAR_GUI_MARBLE_MAP_H
#define OSCAR_GUI_MARBLE_MAP_H
#include <marble/MarbleWidget.h>
#include <marble/LayerInterface.h>
#include <marble/GeoDataLineString.h>

#include <QtCore/QSemaphore>

#include <liboscar/OsmKeyValueObjectStore.h>
#include <sserialize/spatial/GeoWay.h>
#include "States.h"


namespace oscar_gui {

class MarbleMap : public QWidget {
	Q_OBJECT
private:

	typedef sserialize::Static::spatial::TriangulationGeoHierarchyArrangement TriangulationGeoHierarchyArrangement;
	typedef sserialize::Static::spatial::TracGraph TracGraph;
	
	typedef TriangulationGeoHierarchyArrangement::Triangulation Triangulation;
	typedef TriangulationGeoHierarchyArrangement::CFGraph CFGraph;
	
	typedef Triangulation::Face Face;
	
	typedef enum {CS_DIFFERENT=0, CS_SAME=1, __CS_COUNT=2} ColorScheme;
	
	class Data {
	public:
		liboscar::Static::OsmKeyValueObjectStore store;
		TriangulationGeoHierarchyArrangement trs;
		TracGraph cg;
		std::shared_ptr<SearchGeometryState> sgs;
	public:
		Data(const liboscar::Static::OsmKeyValueObjectStore & store, const States & states);
		QColor cellColor(uint32_t cellId, int cs) const;
	private:
		std::vector<QColor> cellColors;
	};
	
	typedef std::shared_ptr<Data> DataPtr;
	
	class MyBaseLayer: public Marble::LayerInterface {
	private:
		qreal m_zValue;
		QStringList m_renderPosition;
		DataPtr m_data;
		int m_opacity;
	protected:
		//render a single cell
		bool doRender(const oscar_gui::MarbleMap::CFGraph& cg, const QBrush & brush, const QString& label, Marble::GeoPainter* painter);
		//render a single Triangulation::Face
		bool doRender(const Face & f, const QBrush & brush, const QString& label, Marble::GeoPainter* painter);
	protected:
		inline const DataPtr & data() const { return m_data; }
		inline DataPtr & data() { return m_data; }
	public:
		MyBaseLayer(const QStringList & renderPos, qreal zVal, const DataPtr & data);
		virtual ~MyBaseLayer() {}
		virtual QStringList renderPosition() const;
		virtual qreal zValue() const;
		inline void opacity(int opacity) { m_opacity = opacity; }
		inline int opacity() const { return m_opacity; }
	};
	
	class MyLockableBaseLayer: public MyBaseLayer {
	protected:
		enum { L_READ=0x1, L_WRITE=0x7FFFFFFF, L_FULL=0x7FFFFFFF};
	private:
		QSemaphore m_lock;
	protected:
		inline QSemaphore & lock() { return m_lock;}
	public:
		MyLockableBaseLayer(const QStringList & renderPos, qreal zVal, const DataPtr & data) :
		MyBaseLayer(renderPos, zVal, data), m_lock(L_FULL) {}
		virtual ~MyLockableBaseLayer() {}
	};
	
	class MyTriangleLayer: public MyBaseLayer {
	private:
		typedef std::unordered_set<uint32_t> TriangleSet;
	private:
		TriangleSet m_cgi;
	public:
		MyTriangleLayer(const QStringList & renderPos, qreal zVal, const DataPtr & data);
		virtual ~MyTriangleLayer() {}
		virtual bool render(Marble::GeoPainter *painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
	public:
		void clear();
		void addTriangle(uint32_t cellId);
		void removeTriangle(uint32_t cellId);
	};
	
	class MyCellLayer: public MyBaseLayer {
	private:
		typedef std::unordered_map<uint32_t, CFGraph> GraphMap;
	private:
		GraphMap m_cgi;
		int m_colorScheme;
	public:
		MyCellLayer(const QStringList & renderPos, qreal zVal, const DataPtr & data);
		virtual ~MyCellLayer() {}
		virtual bool render(Marble::GeoPainter *painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
	public:
		void clear();
		void addCell(uint32_t cellId);
		void removeCell(uint32_t cellId);
	public:
		inline void setColorScheme(int colorScheme) { m_colorScheme = colorScheme; }
		inline int colorScheme() const { return m_colorScheme; }
	};
	
	class MyPathLayer: public MyLockableBaseLayer {
	private:
		Marble::GeoDataLineString m_path;
	public:
		MyPathLayer(const QStringList & renderPos, qreal zVal, const DataPtr & trs);
		virtual ~MyPathLayer() {}
		virtual bool render(Marble::GeoPainter *painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
		void changePath(const sserialize::spatial::GeoWay & p);
	};
	
	class MyGeometryLayer: public MyLockableBaseLayer {
	public:
		MyGeometryLayer(const QStringList & renderPos, qreal zVal, const DataPtr & data);
		virtual ~MyGeometryLayer();
	public:
		virtual bool render(Marble::GeoPainter *painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
	private:
		std::shared_ptr<SearchGeometryState> m_sgs;
	};

private:
	Marble::MarbleWidget * m_map;
	MyTriangleLayer * m_triangleLayer;
	MyCellLayer * m_cellLayer;
	MyGeometryLayer * m_geometryLayer;
	MyPathLayer * m_pathLayer;
	DataPtr m_data;
	int m_cellOpacity;
	int m_colorScheme;
	double m_lastRmbClickLat;
	double m_lastRmbClickLon;
public:
	MarbleMap(const liboscar::Static::OsmKeyValueObjectStore & store, const States & states);
	virtual ~MarbleMap();
public slots:
	void zoomTo(const Marble::GeoDataLatLonBox & bbox);
public slots:
	void zoomToCell(uint32_t cellId);
	void addCell(uint32_t cellId);
	void removeCell(uint32_t cellId);
	void clearCells();
public slots:
	void zoomToTriangle(uint32_t triangleId);
	void addTriangle(uint32_t triangleId);
	void removeTriangle(uint32_t triangleId);
	void clearTriangles();
public slots:
	void showPath(const sserialize::spatial::GeoWay & p);
public slots:
	void geometryDataChanged();
public slots:
	void setCellOpacity(int cellOpacity);
	void setColorScheme(int colorScheme);
signals:
	void toggleCellClicked(uint32_t cellId);
private slots:
	void rmbRequested(int x, int y);
	void toggleCellTriggered();
};

}//end namespace oscar_gui

#endif