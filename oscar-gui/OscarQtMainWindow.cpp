#include "OscarQtMainWindow.h"
#include "SubSetNodeFlattener.h"
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QColumnView>
#include <QtGui/QScrollBar>
#include <QtGui/QSpinBox>
#include <QThreadPool>
#include <QMessageBox>

#include <QFileDialog>
#include <qcombobox.h>
#include <qitemdelegate.h>
#include <QStyledItemDelegate>
#include <qpushbutton.h>
#include <QHeaderView>
#include <liboscar/StaticOsmCompleter.h>
#include <sserialize/storage/MmappedFile.h>
#include <sserialize/iterator/RangeGenerator.h>

#define OSCAR_GUI_MAP_STRETCH 2
#define OSCAR_GUI_SUBSET_STRETCH 2

namespace oscar_gui {

OscarQtMainWindow::OscarQtMainWindow(QString completerPath) : 
m_subSetFlattenerRunId(0)
{

	m_dispatcher = new CompletionDispatcher(this);
	m_itemSetView = new ItemIndexTableModel(this);
	m_completionText = new QLineEdit(this);
	m_subSetModel = new SubSetModel(this);
	m_selectedCompleterType = new QComboBox(this);
	m_selectedTextSearchType = new QComboBox(this);
	
	m_sparseSubSetCB = new QCheckBox("sparse SubSet", this);
	m_sparseSubSetCB->hide();
	
	QVBoxLayout * mainLayout = new QVBoxLayout(this);
	
	QHBoxLayout * cmpLayout = new QHBoxLayout(this);
	QGroupBox * cmpGroupBox = new QGroupBox(this);
	cmpGroupBox->setTitle("Completion");
	cmpGroupBox->setLayout(cmpLayout);
		
	QGroupBox * optionsGroupBox = new QGroupBox(this);
	optionsGroupBox->setTitle("Options");
	QVBoxLayout * optionsLayout = new QVBoxLayout(this);
	optionsGroupBox->setLayout(optionsLayout);
	
	QGroupBox * statsGroupBox = new QGroupBox(this);
	statsGroupBox->setTitle("Statistics");
	QVBoxLayout * statsLayout = new QVBoxLayout(this);
	m_statsLabel = new QLabel(this);
	m_flattenStatsLabel = new QLabel(this);
	statsLayout->addWidget(m_statsLabel);
	statsLayout->addWidget(m_flattenStatsLabel);
	statsGroupBox->setLayout(statsLayout);
	
	optionsLayout->addWidget(m_selectedTextSearchType);
	optionsLayout->addWidget(m_selectedCompleterType);
	optionsLayout->addWidget(m_sparseSubSetCB);
	optionsLayout->addWidget(m_completionText);
	optionsLayout->addWidget(statsGroupBox);
	
	m_subSetView = new QTreeView(this);
	m_subSetView->setModel(m_subSetModel);

	m_tbl = new QTableView(this);
	m_tbl->setModel(m_itemSetView);
	m_tbl->horizontalHeader()->setResizeMode(QHeaderView::ResizeMode::ResizeToContents);
	m_tbl->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_tbl->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	m_map = new MarbleMap();
	m_map->setMapThemeId("earth/openstreetmap/openstreetmap.dgml");
	
	m_tblMapWidget = new QWidget(this);
	m_tblMapLayout = new QHBoxLayout(m_tblMapWidget);
	m_tblMapLayout->addWidget(m_map);
	
	cmpLayout->addWidget(optionsGroupBox, 1);
	cmpLayout->addWidget(m_tbl, 2);
	
	mainLayout->addWidget(m_tblMapWidget, 2);
	mainLayout->addWidget(cmpGroupBox, 1);
	
	//setup menu
	QMenu * menu = new QMenu("File", menuBar());
	QAction* a = new QAction(this);
	a->setText( "Select Completer" );
	a->setShortcuts(QKeySequence::Open);
	connect(a, SIGNAL(triggered()), SLOT(selectCompleter()));
	menu->addAction(a);

	a = new QAction(this);
	a->setText( "Quit" );
	a->setShortcuts(QKeySequence::Close);
	connect(a, SIGNAL(triggered()), SLOT(close()) );
	menu->addAction(a);
	
	menuBar()->addMenu(menu);
	
	//connections
	connect(m_dispatcher, SIGNAL(subSetUpdated(const std::shared_ptr<sserialize::Static::spatial::GeoHierarchy::SubSet> &)), 
				m_subSetModel, SLOT(updateSubSet(const std::shared_ptr<sserialize::Static::spatial::GeoHierarchy::SubSet> &)));
	connect(m_dispatcher, SIGNAL(indexUpdated(const sserialize::ItemIndex &)), SLOT(completionIndexChanged(const sserialize::ItemIndex&)));
	connect(m_dispatcher, SIGNAL( statsUpdated(const oscar_gui::CompletionDispatcher::CompletionStats &) ), SLOT(updateStats(const oscar_gui::CompletionDispatcher::CompletionStats&) ));
	connect(m_sparseSubSetCB, SIGNAL(toggled(bool)), m_dispatcher, SLOT(setSubSetType(bool)));
	
	connect(m_selectedTextSearchType, SIGNAL(currentIndexChanged(int)), SLOT(selectedTextSearchTypeChanged(int)));
	connect(m_selectedCompleterType, SIGNAL(currentIndexChanged(int)), SLOT(selectedCompleterTypeChanged(int)));
	
	connect(m_completionText, SIGNAL( textChanged(const QString &) ), m_dispatcher, SLOT( completionTextUpdated(const QString &)));
	connect(m_completionText, SIGNAL( cursorPositionChanged(int,int)), m_dispatcher, SLOT( cursorPositionChanged(int, int)));
	connect(this, SIGNAL(viewSetChanged(uint32_t, uint32_t)), m_map, SLOT(viewSetChanged(uint32_t, uint32_t) ));
	connect(this, SIGNAL(viewSetChanged(const sserialize::ItemIndex &)), m_map, SLOT(viewSetChanged(const sserialize::ItemIndex &) ));
	connect(this, SIGNAL(viewSetChanged(const sserialize::ItemIndex &)), m_itemSetView, SLOT(setIndex(const sserialize::ItemIndex&))); 
	connect(m_tbl->verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(tableViewChanged()));
	connect(m_tbl->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(tableSelectedRowChanged(QItemSelection,QItemSelection)));
	connect(m_subSetView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(subSetSelectionChanged(QItemSelection,QItemSelection)));
	connect(this, SIGNAL(selectedTableItemChanged(uint32_t)), m_map, SLOT(zoomToItem(uint32_t)));
	connect(this, SIGNAL(selectedSubSetItemChanged(liboscar::Static::OsmKeyValueObjectStore::Item)), m_map, SLOT(drawAndZoomTo(liboscar::Static::OsmKeyValueObjectStore::Item)));
	
	//set the central widget
	QWidget * centralWidget = new QWidget(this);
	centralWidget->setLayout(mainLayout);
	setCentralWidget(centralWidget);
	
	//and energize
	setCompleter(completerPath);
}

OscarQtMainWindow::~OscarQtMainWindow()
{}

void OscarQtMainWindow::updateStats(const CompletionDispatcher::CompletionStats & stats) {
	const QLocale & cLocale = QLocale::c();
	QString size = cLocale.toString(stats.size);
	QString timeInUSecs = cLocale.toString(static_cast<qlonglong>(stats.timeInUsec));
	
	QString statsString =
	QString("Max size: %1 Items\nTime: %2 us").arg(size).arg(timeInUSecs);
	m_statsLabel->setText(statsString);
}

void OscarQtMainWindow::updateTextSearchTypes() {
	int indexOfBest = -1;
	m_selectedTextSearchType->clear();
	if (m_cmp->textSearch().hasSearch(liboscar::TextSearch::GEOCELL)) {
		m_selectedTextSearchType->addItem("GeoCell", liboscar::TextSearch::GEOCELL);
		indexOfBest = m_selectedTextSearchType->count()-1;
	}
	if (m_cmp->textSearch().hasSearch(liboscar::TextSearch::GEOHIERARCHY)) {
		m_selectedTextSearchType->addItem("GeoHierarchy", liboscar::TextSearch::GEOHIERARCHY);
		indexOfBest = m_selectedTextSearchType->count()-1;
	}
	if (m_cmp->textSearch().hasSearch(liboscar::TextSearch::ITEMS)) {
		m_selectedTextSearchType->addItem("Items", liboscar::TextSearch::ITEMS);
		indexOfBest = m_selectedTextSearchType->count()-1;
	}
	if (indexOfBest >= 0) {
		m_selectedTextSearchType->setCurrentIndex(indexOfBest);
	}
}

void OscarQtMainWindow::setCompleter(QString fileName) {
	if (!fileName.isNull()) {
		m_subSetFlattenerRunId = 0;
		std::string completerPath( fileName.toStdString());
		if (!sserialize::MmappedFile::isDirectory(completerPath)) {
			std::cout << "Failed to set completer dir" << std::endl;
			return;
		}
		completerPath += "/";

		m_cmp.reset(new liboscar::Static::OsmCompleter());
		
		m_cmp->setAllFilesFromPrefix(completerPath);
		try {
			m_cmp->energize();
			m_itemSetView->setStore(m_cmp->store());
			m_subSetModel->setStore(m_cmp->store());
			m_dispatcher->setCompleter(m_cmp);
			m_map->itemStoreChanged(m_cmp->store());
		}
		catch (sserialize::Exception & e) {
			std::cerr << "Catched an exception: " << e.what() << std::endl;
			m_cmp.reset();
			throw;
		}
		updateTextSearchTypes();
	}
}

void OscarQtMainWindow::selectCompleter() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Completer"), QDir::currentPath(), tr("Completer Files (*.trie)"));
	setCompleter(fileName);
}

void OscarQtMainWindow::selectCellTextCompletion() {
	int pos = m_tblMapLayout->indexOf(m_subSetView);
	if (pos < 0) {
		m_tblMapLayout->insertWidget(0, m_subSetView, 1);
		m_subSetView->show();
		m_sparseSubSetCB->show();
		m_tblMapLayout->setStretch(1, 2); //set stretch of the map
	}
	m_dispatcher->setTextSearchType(liboscar::TextSearch::GEOCELL);
}

void OscarQtMainWindow::selectItemsCompletion() {
	int pos = m_tblMapLayout->indexOf(m_subSetView);
	if (pos >= 0) {
		m_tblMapLayout->removeWidget(m_subSetView);
		m_subSetView->hide();
		m_sparseSubSetCB->hide();
		m_tblMapLayout->setStretch(0, 0); //set stretch of the map
	}
	liboscar::TextSearch::Type ts = (liboscar::TextSearch::Type) m_selectedTextSearchType->itemData(m_selectedTextSearchType->currentIndex()).toInt();
	m_selectedCompleterType->setCurrentIndex(m_cmp->textSearch().selectedTextSearcher(ts));
	m_dispatcher->setTextSearchType((liboscar::TextSearch::Type) ts);
}

void OscarQtMainWindow::selectGeoHierarchyCompletion() {
	int pos = m_tblMapLayout->indexOf(m_subSetView);
	if (pos >= 0) {
		m_tblMapLayout->removeWidget(m_subSetView);
		m_subSetView->hide();
		m_sparseSubSetCB->hide();
		m_tblMapLayout->setStretch(0, 0); //set stretch of the map
	}
	liboscar::TextSearch::Type ts = (liboscar::TextSearch::Type) m_selectedTextSearchType->itemData(m_selectedTextSearchType->currentIndex()).toInt();
	m_selectedCompleterType->setCurrentIndex(m_cmp->textSearch().selectedTextSearcher(ts));
	m_dispatcher->setTextSearchType((liboscar::TextSearch::Type) ts);
}

void OscarQtMainWindow::selectedCompleterTypeChanged(int index) {
	int ct = m_selectedCompleterType->itemData(index).toInt();
	int ts = m_selectedTextSearchType->itemData(m_selectedTextSearchType->currentIndex()).toInt();
	bool ok = m_cmp->setTextSearcher((liboscar::TextSearch::Type) ts, ct);
	if (!ok) {
		QMessageBox msgBox;
		msgBox.addButton(QMessageBox::Ok);
		msgBox.setText("Failed to select the requested completer: " + QString::number(ts) + ":" + QString::number(ct));
		msgBox.exec();
	}
}

void OscarQtMainWindow::selectedTextSearchTypeChanged(int index) {
	int ts = m_selectedTextSearchType->itemData(index).toInt();
	if (ts == liboscar::TextSearch::GEOCELL && m_cmp->textSearch().hasSearch(liboscar::TextSearch::GEOCELL)) {
		m_selectedCompleterType->clear();
		for(uint32_t i : sserialize::RangeGenerator::range(0, m_cmp->textSearch().size(liboscar::TextSearch::GEOCELL))) {
			m_selectedCompleterType->addItem(QString::fromStdString(m_cmp->textSearch().get<liboscar::TextSearch::GEOCELL>(i).trie().getName()), i);
		}
		selectCellTextCompletion();
	}
	else if (ts == liboscar::TextSearch::ITEMS && m_cmp->textSearch().hasSearch(liboscar::TextSearch::ITEMS)) {
		m_selectedCompleterType->clear();
		for(uint32_t i : sserialize::RangeGenerator::range(0, m_cmp->textSearch().size(liboscar::TextSearch::ITEMS))) {
			m_selectedCompleterType->addItem(QString::fromStdString(m_cmp->textSearch().get<liboscar::TextSearch::ITEMS>(i).getName()), i);
		}
		selectItemsCompletion();
	}
	else if (ts == liboscar::TextSearch::GEOHIERARCHY && m_cmp->textSearch().hasSearch(liboscar::TextSearch::GEOHIERARCHY)) {
		m_selectedCompleterType->clear();
		for(uint32_t i : sserialize::RangeGenerator::range(0, m_cmp->textSearch().size(liboscar::TextSearch::GEOHIERARCHY))) {
			m_selectedCompleterType->addItem(QString::fromStdString(m_cmp->textSearch().get<liboscar::TextSearch::GEOHIERARCHY>(i).getName()), i);
		}
		selectGeoHierarchyCompletion();
	}
	else {
		QMessageBox msgBox;
		msgBox.addButton(QMessageBox::Ok);
		msgBox.setText("Failed to select the requested TextSearch: " + QString::number(ts) );
		msgBox.exec();
	}
}



/*
void OscarQtMainWindow::acceptTagStoreSelection() {

	std::set<uint16_t> selection = m_tagStoreModel->getSelectedNodes();
	if (selection.size() > 0) {
		
		QString str = "$TAGID[";
		for(std::set<uint16_t>::const_iterator it = selection.begin(); it != selection.end(); it++) {
			str += QString::number(*it);
			str += ";";
		}
		str.chop(1); //remove the last ;
		str += "]";
		m_completionText->insert(str);
	}
}*/

void OscarQtMainWindow::tableViewChanged() {
	int rowBegin =  m_tbl->rowAt(0);
	int rowEnd =  m_tbl->rowAt( m_tbl->height());
	if (rowEnd == -1) {
		if (rowBegin == -1) {
			emit viewSetChanged(0, 0);
		}
		else {
			emit viewSetChanged(0, m_tbl->model()->rowCount());
		}
	}
	else {
		emit viewSetChanged(rowBegin, rowEnd);
	}
}

void OscarQtMainWindow::tableSelectedRowChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/) {
	if (selected.size()) {
		emit selectedTableItemChanged(  selected.first().top() );
	}
}

void OscarQtMainWindow::subSetSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/) {
	if (selected.size()) {
		QModelIndex index = selected.indexes().first();
		QVariant regionGhId = index.data(SubSetModel::RegionGhId);
		if (regionGhId.isValid()) {
			uint32_t rGhId = regionGhId.toUInt();
			emit selectedSubSetItemChanged(m_cmp->store().at(m_cmp->store().geoHierarchy().ghIdToStoreId(rGhId)));
			SubSetNodeFlattener * ssf;
			sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr ssnPtr(index.data(SubSetModel::SubSetNodePtr).value<sserialize::Static::spatial::GeoHierarchy::SubSet::NodePtr>());
			ssf = new SubSetNodeFlattener(m_subSetModel->subSet(), ssnPtr, m_subSetFlattenerRunId+1);
			m_runningFlatteners[m_subSetFlattenerRunId] = ssf;
			++m_subSetFlattenerRunId;
			ssf->setAutoDelete(false);
			connect(ssf->com(), SIGNAL(completed(const sserialize::ItemIndex &, uint64_t, qlonglong)), this, SLOT(viewSetCalcCompleted(const sserialize::ItemIndex &, uint64_t, qlonglong)));
			QThreadPool::globalInstance()->start(ssf);
		}
	}
}

void OscarQtMainWindow::completionIndexChanged(const sserialize::ItemIndex& idx) {
	emit viewSetChanged(idx);
}


void OscarQtMainWindow::completionSetChanged() {
	tableViewChanged();
}

void OscarQtMainWindow::viewSetCalcCompleted(const sserialize::ItemIndex& resultIdx, uint64_t runId, qlonglong timeInUsec) {
	if (m_subSetFlattenerRunId <= runId) {
		m_subSetFlattenerRunId = runId;
		m_flattenStatsLabel->setText(QString("Viewset size: %1\nTime to flatten: %2 us").arg(resultIdx.size()).arg(timeInUsec));
		emit viewSetChanged(resultIdx);
	}
	delete m_runningFlatteners[runId];
	m_runningFlatteners.erase(runId);
}


}//end namespace