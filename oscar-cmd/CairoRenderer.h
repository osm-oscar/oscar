#ifndef OSCAR_CMD_CAIRO_RENDERER_H
#define OSCAR_CMD_CAIRO_RENDERER_H
#include <sserialize/spatial/GeoRect.h>
#include <sserialize/spatial/GeoPoint.h>
#include <cairo/cairo.h>

namespace oscarcmd {

class CairoRenderer {
public:
	struct Color {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t o; //opacity
		Color() : r(0), g(0), b(0), o(0) {}
		Color(uint8_t r, uint8_t g, uint8_t b, uint8_t o = 255) : r(r), g(g), b(b), o(o) {}
	};
public:
	CairoRenderer();
	virtual ~CairoRenderer();
	void init(const sserialize::spatial::GeoRect & rect, int latpix);
	void toPng(const std::string & path);
	//Draw Triangle
	void draw(const sserialize::spatial::GeoPoint & v1, const sserialize::spatial::GeoPoint & v2, const sserialize::spatial::GeoPoint & v3, const Color & c);
protected:
	void toCairoCoords(const sserialize::spatial::GeoPoint & p, double & x, double & y);
	void setColor(const Color & c);
private:
	cairo_surface_t * m_surface;
	cairo_t * m_ctx;
	sserialize::spatial::GeoRect m_bounds;
	int m_latpix;
	int m_lonpix;
};

}//end namespace oscar_cmd

#endif