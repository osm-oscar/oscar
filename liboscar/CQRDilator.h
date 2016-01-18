#ifndef LIBOSCAR_CQR_DILATOR_H
#define LIBOSCAR_CQR_DILATOR_H
#include <sserialize/Static/TracGraph.h>
#include <sserialize/Static/Array.h>
#include <sserialize/Static/GeoPoint.h>

#include <sserialize/spatial/CellQueryResult.h>

namespace liboscar {

/**
  * ------------------------------------------------------------
  * Array<GeoPoint>
  * ------------------------------------------------------------
  * Static::Array<Static::GeoPoint>
  * ------------------------------------------------------------
  *
  *
  *
  */

//Dilate a CQR
class CQRDilator {
public:
private:
	typedef sserialize::Static::Array<sserialize::Static::spatial::GeoPoint> CellInfo;
public:
	///@param d the weight-center of cells
	CQRDilator(const CellInfo & d, const sserialize::Static::spatial::TracGraph & tg);
	~CQRDilator();
	///@param amount in meters, @return list of cells that are part of the dilated area (input cells are NOT part of this list)
	sserialize::ItemIndex dilate(const sserialize::CellQueryResult & src, double diameter) const;
private:
	double distance(const sserialize::Static::spatial::GeoPoint & gp1, const sserialize::Static::spatial::GeoPoint & gp2) const;
	double distance(const sserialize::Static::spatial::GeoPoint & gp, uint32_t cellId) const;
	double distance(uint32_t cellId1, uint32_t cellId2) const;
private:
	CellInfo m_ci;
	sserialize::Static::spatial::TracGraph m_tg;
};


}//end namespace liboscar

#endif