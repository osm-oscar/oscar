#ifndef LIBOSCAR_CQR_FROM_POLYGON_H
#define LIBOSCAR_CQR_FROM_POLYGON_H
#include "OsmKeyValueObjectStore.h"
#include <sserialize/spatial/GeoPolygon.h>

namespace liboscar {

class CQRFromPolygon final {
public:
	///AC_POLYGON_CELL is currently unimplemented and falls back to AC_POLYGON_CELL_BBOX
	enum Accuracy : uint32_t { AC_POLYGON_BBOX_CELL_BBOX, AC_POLYGON_CELL_BBOX, AC_POLYGON_CELL};
public:
	CQRFromPolygon(const Static::OsmKeyValueObjectStore & store, const sserialize::Static::ItemIndexStore & idxStore);
	~CQRFromPolygon();
	sserialize::ItemIndex intersectingCells(const sserialize::spatial::GeoPolygon & gp, Accuracy ac) const;
private:
	sserialize::ItemIndex intersectingCellsPolygonCellBBox(const sserialize::spatial::GeoPolygon & gp) const;
private:
	Static::OsmKeyValueObjectStore m_store;
	sserialize::Static::ItemIndexStore m_idxStore;
};


}//end namespace

#endif