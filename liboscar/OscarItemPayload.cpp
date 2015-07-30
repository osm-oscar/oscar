#include "OscarItemPayload.h"
#include <iostream>
#include <sserialize/storage/pack_unpack_functions.h>
#include <sserialize/utility/log.h>


namespace liboscar {
namespace Static {
	
OscarItemPayload::OscarItemPayload() : 
m_poiCount(0)
{}

OscarItemPayload::OscarItemPayload(const sserialize::UByteArrayAdapter& data) :
m_poiCount(data.at(0)),
m_poiData(data + 1)
{}

uint16_t OscarItemPayload::getPoI(uint8_t pos) const {
	if (pos >= poiCount())
		return 0;
	return m_poiData.getUint16(2*pos);
}

uint8_t OscarItemPayload::poiCount(const sserialize::UByteArrayAdapter& data) {
	return data.at(0);
}

uint16_t OscarItemPayload::getPoI(const sserialize::UByteArrayAdapter& data, uint8_t pos) {
	if (data.at(0) <= pos)
		return 0;
	return data.getUint16(1+2*pos);
}



}}//END NAMESPACE