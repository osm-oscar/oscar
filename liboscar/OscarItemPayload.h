#ifndef LIBOSCAR_ITEM_PAYLOAD_H
#define LIBOSCAR_ITEM_PAYLOAD_H
#include <ostream>
#include "constants.h"
#include  "StaticTagStore.h"
#include <sserialize/storage/UByteArrayAdapter.h>
#include <sserialize/search/ItemSet.h>
#include <sserialize/Static/GeoPoint.h>

namespace liboscar {
namespace Static {

/* Layout
 * 
 * -------------------------------------------------------------
 * PPPPPPPP|PoIPoIPoI*|
 * -------------------------------------------------------------
 *  1 byte |2 byte* |
 * 
 * P = count of PoIs
 * PoI = multiple point of interest, only present if P>0
 * ELMCOUNT = variable length positions count, only present if T>0 is set
 * LAT = latitude
 * LON = longitude
 */

class OscarItemPayload {
private:
	uint8_t m_poiCount;
	sserialize::UByteArrayAdapter m_poiData;
public:
	OscarItemPayload();
	OscarItemPayload(const sserialize::UByteArrayAdapter & data);
	inline uint8_t poiCount() const { return m_poiCount;}
	//Returns node id of selcted poi, use Static::TagStore to resolve
	uint16_t getPoI(uint8_t pos) const;
	inline bool isValid() const { return !m_poiData.isEmpty();}
	static uint8_t poiCount(const sserialize::UByteArrayAdapter & data);
	static uint16_t getPoI(const sserialize::UByteArrayAdapter & data, uint8_t pos);
};

}}//end namespace
#endif