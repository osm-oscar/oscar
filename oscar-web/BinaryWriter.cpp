#include "BinaryWriter.h"
#include <sserialize/storage/pack_funcs.h>

namespace oscar_web {

BinaryWriter::BinaryWriter(std::ostream& out) :
m_out(out)
{}

BinaryWriter::~BinaryWriter() {}

void BinaryWriter::putU8(uint8_t src) { putPod<uint8_t>(src); }
void BinaryWriter::putU16(uint16_t src) { putPod<uint16_t>(htole16(src)); }
void BinaryWriter::putU32(uint32_t src) { putPod<uint32_t>(htole32(src)); }

}//end namespace