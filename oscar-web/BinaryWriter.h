#ifndef OSCAR_WEB_BINARY_WRITER_H
#define OSCAR_WEB_BINARY_WRITER_H
#include <ostream>
#include <string.h>

namespace oscar_web {

class BinaryWriter {
private:
	std::ostream & m_out;
private:
	template<typename TPOD>
	inline void putPod(TPOD p) {
		char buf[sizeof(TPOD)];
		::memmove(buf, &p, sizeof(TPOD));
		m_out.write(buf, sizeof(TPOD));
	}
public:
	BinaryWriter(std::ostream & out);
	~BinaryWriter();
	void putU8(uint8_t src);
	void putU16(uint16_t src);
	void putU32(uint32_t src);
};

};


#endif