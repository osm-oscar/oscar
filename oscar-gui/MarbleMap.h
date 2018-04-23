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
		std::shared_ptr<ItemGeometryState> igs;
		std::shared_ptr<ResultListState> rls;
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
		struct DrawData {
			CFGraph graph;
			float alpha; //between 0.0 and 1.0
		};
		typedef std::unordered_map<uint32_t, DrawData> GraphMap;
	private:
		GraphMap m_cgi;
		int m_colorScheme;
	public:
		MyCellLayer(const QStringList & renderPos, qreal zVal, const DataPtr & data);
		virtual ~MyCellLayer() {}
		virtual bool render(Marble::GeoPainter *painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
	public:
		void clear();
		void addCell(uint32_t cellId, float alpha = 1.0);
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
		virtual bool render(
			Marble::GeoPainter *painter,
			Marble::ViewportParams * viewport,
			const QString & renderPos,
			Marble::GeoSceneLayer * layer,
			QPen pen,
			const QBrush & brush,
			sserialize::AbstractArrayIterator<const GeometryState::Entry&> begin,
			sserialize::AbstractArrayIterator<const GeometryState::Entry&> end);
	};

	class MyInputSearchGeometryLayer: public MyBaseLayer {
	public:
		MyInputSearchGeometryLayer(const QStringList & renderPos, qreal zVal, const DataPtr & trs);
		virtual ~MyInputSearchGeometryLayer();
	public:
		virtual bool render(Marble::GeoPainter *painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
	public:
		void update(const Marble::GeoDataLineString & d);
	private:
		Marble::GeoDataLineString m_d;
	};
	
	class MyItemLayer: public MyGeometryLayer {
	public:
		MyItemLayer(const QStringList & renderPos, qreal zVal, const DataPtr & trs);
		virtual ~MyItemLayer() {}
		virtual bool render(Marble::GeoPainter *painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
	public:
		void clear();
		void addItem(uint32_t itemId);
		void removeItem(uint32_t itemId);
	private:
		std::unordered_set<uint32_t> m_items;
	};
	
	class MyCellQueryResultLayer: public MyCellLayer {
	public:
		MyCellQueryResultLayer(const QStringList & renderPos, qreal zVal, const DataPtr & trs);
		virtual ~MyCellQueryResultLayer() {}
	public:
		void disable();
		void enable();
		void dataChanged();
	private:
		bool m_enabled;
	};
	
public:
	MarbleMap(const liboscar::Static::OsmKeyValueObjectStore & store, const States & states);
	virtual ~MarbleMap();
public slots:
	void zoomTo(const Marble::GeoDataLatLonBox & bbox);
public slots:
	void zoomToItem(uint32_t itemId);
	void addItem(uint32_t itemId);
	void removeItem(uint32_t itemId);
	void clearItems();
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
	void geometryDataChanged(int p);
public slots:
	void setCellOpacity(int cellOpacity);
	void setColorScheme(int colorScheme);
public slots:
	void displayCqrCells(bool enable);
	void cqrDataChanged();
private slots:
	void rmbRequested(int x, int y);
	void beginSearchGeometryTriggered();
	void addToSearchGeometryTriggered();
	void endSearchGeometryAsPointTriggered();
	void endSearchGeometryAsRectTriggered();
	void endSearchGeometryAsPathTriggered();
	void endSearchGeometryAsPolygonTriggered();
private:
	Marble::MarbleWidget * m_map;
	MyTriangleLayer * m_triangleLayer;
	MyCellLayer * m_cellLayer;
	MyItemLayer * m_itemLayer;
	MyGeometryLayer * m_geometryLayer;
	MyPathLayer * m_pathLayer;
	MyInputSearchGeometryLayer * m_isgLayer;
	MyCellQueryResultLayer * m_cqrLayer;
	DataPtr m_data;
	int m_cellOpacity;
	int m_colorScheme;
	double m_lastRmbClickLat; //in radian
	double m_lastRmbClickLon; // in radian
	Marble::GeoDataLineString m_isg;
};

}//end namespace oscar_gui

#endif
