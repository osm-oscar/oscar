#include "StateHandlers.h"

namespace oscar_gui {

SearchGeometryStateHandler::SearchGeometryStateHandler(const oscar_gui::States& states) :
m_sgs(states.sgs),
m_sgh(states.cmp)
{
	connect(m_sgs.get(), SIGNAL(dataChanged(int)), this, SLOT(dataChanged(int)));
}

SearchGeometryStateHandler::~SearchGeometryStateHandler() {}

void SearchGeometryStateHandler::dataChanged(int p) {
	{
		auto lock(m_sgs->readLock());
		if (m_sgs->size() <= p) {
			return;
		}
		auto at = m_sgs->active(p);
		if ((at & SearchGeometryState::AT_CELLS) && !m_sgs->cells(p).size()) {
			sserialize::ItemIndex cells;
			const Marble::GeoDataLineString & d = m_sgs->data(p);
			switch (m_sgs->type(p)) {
			case SearchGeometryState::DT_POINT:
				if (d.size() > 0) {
					cells = m_sgh.cells(Marble::GeoDataPoint(d.first()));
				}
				break;
			case SearchGeometryState::DT_RECT:
				if (d.size() > 0) {
					cells = m_sgh.cells(d.latLonAltBox());
				}
				break;
			case SearchGeometryState::DT_PATH:
				if (d.size() > 0) {
					cells = m_sgh.cells(d);
				}
				break;
			case SearchGeometryState::DT_POLYGON:
				if (d.size() > 0) {
					cells = m_sgh.cells( Marble::GeoDataLinearRing(d) );
				}
				break;
			default:
				break;
			};
			
			if (cells.size()) {
				//TODO: race condition
				lock.unlock();
				m_sgs->setCells(p, cells);
				lock.lock();
			}
		}
		
		if ((at & SearchGeometryState::AT_TRIANGLES) && !m_sgs->triangles(p).size()) {
			sserialize::ItemIndex triangles;
			const Marble::GeoDataLineString & d = m_sgs->data(p);
			switch (m_sgs->type(p)) {
			case SearchGeometryState::DT_POINT:
				if (d.size() > 0) {
					triangles = m_sgh.triangles(Marble::GeoDataPoint(d.first()));
				}
				break;
			case SearchGeometryState::DT_PATH:
				if (d.size() > 0) {
					triangles = m_sgh.triangles(d);
				}
				break;
			default:
				break;
			};
			
			if (triangles.size()) {
				//TODO: race condition
				lock.unlock();
				m_sgs->setTriangles(p, triangles);
				lock.lock();
			}
		}
	}
}

StateHandlers::StateHandlers(const States& states) :
sgsh(new SearchGeometryStateHandler(states))
{}


}//end namespace