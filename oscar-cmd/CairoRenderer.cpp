#include "CairoRenderer.h"
#include <cmath>

namespace oscarcmd {

double mercator_project_inverse_lon(double lon, double lon0) {
	return lon+lon0;
}

double mercator_project_inverse_lat(double lat) {
	return ::atan( ::sinh(lat) );
}

double mercator_project_lon(double lon, double lon0) {
	return lon-lon0;
}

double mercator_project_lat(double lat) {
	return ::log( ::tan(M_PI/4 + lat/4) ); 
}

CairoRenderer::CairoRenderer() :
m_surface(0),
m_ctx(0),
m_latpix(0),
m_lonpix(0)
{}

CairoRenderer::~CairoRenderer() {
	if (m_ctx) {
		::cairo_destroy(m_ctx);
		m_ctx = 0;
	}
	if (m_surface) {
		::cairo_surface_destroy(m_surface);
		m_surface = 0;
	}
}

void CairoRenderer::init(const sserialize::spatial::GeoRect& rect, int latpix) {
	if (latpix <= 0) {
		throw std::out_of_range("CairoRenderer::init: latpix or lonpix is out of range");
	}
	m_latpix = latpix;
	m_lonpix = latpix * (rect.maxLon()-rect.minLon())/(rect.maxLat()-rect.minLat());
	
	if (m_ctx) {
		::cairo_destroy(m_ctx);
	}
	if (m_surface) {
		::cairo_surface_destroy(m_surface);
	}
	m_surface = ::cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_latpix, m_lonpix);
	m_ctx = ::cairo_create(m_surface);
	m_bounds = rect;
}

void CairoRenderer::toPng(const std::string& path) {
	cairo_status_t status = ::cairo_surface_write_to_png(m_surface, path.c_str());
	if (status != CAIRO_STATUS_SUCCESS) {
		throw std::runtime_error(std::string("CairoRenderer::toPng: ") + std::string(::cairo_status_to_string(status)));
	}
}

void CairoRenderer::setColor(const CairoRenderer::Color& c) {
	::cairo_set_source_rgba(m_ctx, (double)c.r/255, (double)c.g/255, (double)c.b/255, (double)c.o/255);
}

void CairoRenderer::draw(const sserialize::spatial::GeoPoint& v1, const sserialize::spatial::GeoPoint& v2, const sserialize::spatial::GeoPoint& v3, const CairoRenderer::Color& c) {
	::cairo_new_path(m_ctx);
	double x1, y1, x2, y2, x3, y3;
	toCairoCoords(v1, x1, y1);
	toCairoCoords(v2, x2, y2);
	toCairoCoords(v3, x3, y3);
	setColor(c);
	::cairo_move_to(m_ctx, x1, y1);
	::cairo_line_to(m_ctx, x2, y2);
	::cairo_line_to(m_ctx, x3, y3);
	::cairo_close_path(m_ctx);
	::cairo_stroke_preserve(m_ctx);
	::cairo_fill(m_ctx);
}

void CairoRenderer::toCairoCoords(const sserialize::spatial::GeoPoint & p, double & x, double & y) {
	y = (p.lat()-m_bounds.minLat())/(m_bounds.maxLat()-m_bounds.minLat());
	x = (p.lon()-m_bounds.minLon())/(m_bounds.maxLon()-m_bounds.minLon());
	
// 	y = mercator_project_lat(p.lat()); 
	
// 	y -= mercator_project_lat(m_bounds.minLat()) / (mercator_project_lat(m_bounds.maxLat()) - mercator_project_lat(m_bounds.minLat()));

	//x,y should now be between 0..1
	y *= m_latpix;
	x *= m_lonpix;
}


}//end namespace oscar_cmd