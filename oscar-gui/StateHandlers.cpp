#include "StateHandlers.h"

#include <QtConcurrent/QtConcurrentRun>

namespace oscar_gui {
	
//BEGIN TextSearchStateHandler
	
	
SearchStateHandler::SearchStateHandler(const States & states) :
m_tss(states.tss),
m_rls(states.rls),
m_cmp(states.cmp),
m_pendingUpdate(false)
{
	connect(m_tss.get(), &TextSearchState::searchTextChanged, this, &SearchStateHandler::searchTextChanged);
	connect(this, &SearchStateHandler::searchResultsChanged, m_rls.get(), &ResultListState::setResult);
}

SearchStateHandler::~SearchStateHandler() {}

void SearchStateHandler::searchTextChanged(const QString & queryString) {
	auto l(writeLock());
	m_qs = queryString;
	if (!m_pendingUpdate) {
		m_pendingUpdate = true;
		l.unlock();
		computeResult();
	}
}

void SearchStateHandler::computationCompleted(const QString & queryString, const sserialize::CellQueryResult & cqr, const sserialize::ItemIndex & items) {
	emit(searchResultsChanged(queryString, cqr, items));
	computeResult();
}

void SearchStateHandler::computeResult2(const QString & queryString) {
	auto cqr = m_cmp->cqrComplete(queryString.toStdString(), true, 2);
	if (cqr.flags() & sserialize::CellQueryResult::FF_CELL_LOCAL_ITEM_IDS) {
		cqr = cqr.toGlobalItemIds();
	}
	sserialize::ItemIndex items = cqr.flaten(2);
	emit( computationCompleted(queryString, cqr, items) );
}

void SearchStateHandler::computeResult() {
	auto l(writeLock());
	if (m_pendingUpdate) {
		m_pendingUpdate = false;
		QString qs = m_qs;
		l.unlock();
		auto fb = std::bind(&SearchStateHandler::computeResult2, this, qs);
		QtConcurrent::run(fb);
	}
}
//END

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
		if (p < 0 || m_sgs->size() <= (std::size_t) p) {
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
sgsh(std::make_shared<SearchGeometryStateHandler>(states)),
ssh(std::make_shared<SearchStateHandler>(states))
{}


}//end namespace
