#ifndef OSCAR_GUI_MARBLE_MAP_H
#define OSCAR_GUI_MARBLE_MAP_H
#include <marble/MarbleWidget.h>
#include <marble/LayerInterface.h>
#include <liboscar/StaticOsmItemSet.h>
#include <QSemaphore>

namespace oscar_gui {

class MarbleMap : public Marble::MarbleWidget {
	Q_OBJECT
private:

	class MyBaseLayer: public Marble::LayerInterface {
	private:
		qreal m_zValue;
		QStringList m_renderPosition;
		std::unordered_map<int, QColor> m_shapeColor;
	protected:
		bool doRender(const liboscar::Static::OsmKeyValueObjectStore::Item & item, const QString & label, Marble::GeoPainter * painter);
	public:
		MyBaseLayer(const QStringList & renderPos, qreal zVal);
		virtual ~MyBaseLayer() {}
		virtual QStringList renderPosition() const;
		virtual qreal zValue() const;
		inline QColor & shapeColor(uint32_t geoShapeType) {
			return m_shapeColor[geoShapeType];
		}
	};
	
	class MyLockableBaseLayer: public MyBaseLayer {
	protected:
		enum { L_READ=0x1, L_WRITE=0x7FFFFFFF, L_FULL=0x7FFFFFFF};
	private:
		QSemaphore m_lock;
	protected:
		inline QSemaphore & lock() { return m_lock;}
	public:
		MyLockableBaseLayer(const QStringList & renderPos, qreal zVal) : MyBaseLayer(renderPos, zVal), m_lock(L_FULL) {}
		virtual ~MyLockableBaseLayer() {}
	};
	
	class MyItemSetLayer: public MyLockableBaseLayer {
	private:
		sserialize::ItemIndex m_set;
		liboscar::Static::OsmKeyValueObjectStore m_store;
		uint32_t m_showItemsBegin;
		uint32_t m_showItemsEnd;
	public:
		MyItemSetLayer(const QStringList & renderPos, qreal zVal);
		virtual ~MyItemSetLayer() {}
		virtual bool render(Marble::GeoPainter *painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
		void setItemSet(const sserialize::ItemIndex & idx);
		void setStore(const liboscar::Static::OsmKeyValueObjectStore & store);
		void setViewRange(uint32_t begin, uint32_t end);
	};
	
	class MySingleItemLayer: public MyLockableBaseLayer {
	private:
		liboscar::Static::OsmKeyValueObjectStore::Item m_item;
	public:
		MySingleItemLayer(const QStringList & renderPos, qreal zVal);
		virtual ~MySingleItemLayer() {}
		virtual bool render(Marble::GeoPainter * painter, Marble::ViewportParams * viewport, const QString & renderPos, Marble::GeoSceneLayer * layer);
		void setItem(const liboscar::Static::OsmKeyValueObjectStore::Item & item);
	};
	
private:
	sserialize::ItemIndex m_set;
	liboscar::Static::OsmKeyValueObjectStore m_store;
	MyItemSetLayer * m_baseItemLayer;
	MySingleItemLayer * m_highlightItemLayer;
	MySingleItemLayer * m_singleItemLayer;
public:
	MarbleMap();
	virtual ~MarbleMap();
public Q_SLOTS:
	void itemStoreChanged(const liboscar::Static::OsmKeyValueObjectStore & store);
	void viewSetChanged(uint32_t begin, uint32_t end);
	void viewSetChanged(const sserialize::ItemIndex & set);
	void zoomToItem(uint32_t itemPos);
	void drawAndZoomTo(const liboscar::Static::OsmKeyValueObjectStore::Item & item);
};

}

#endif