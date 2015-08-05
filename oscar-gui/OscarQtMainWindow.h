#ifndef OSCAR_QT_MAIN_WINDOW_H
#define OSCAR_QT_MAIN_WINDOW_H
#include "ItemIndexTableModel.h"
#include "CompletionDispatcher.h"
#include "MarbleMap.h"
#include "SubSetModel.h"

#include <QtGui/QMainWindow>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QTableView>
#include <QTreeView>
#include <QHBoxLayout>

namespace oscar_gui {

class SubSetNodeFlattener;

class OscarQtMainWindow : public QMainWindow {
Q_OBJECT
private:
	CompletionDispatcher * m_dispatcher;
	ItemIndexTableModel * m_itemSetView;
	SubSetModel * m_subSetModel;
private://GUI
	QWidget * m_tblMapWidget;
	QComboBox * m_selectedTextSearchType;
	QComboBox * m_selectedCompleterType;
	QHBoxLayout * m_tblMapLayout;
	QCheckBox * m_sparseSubSetCB;
	QLineEdit * m_completionText;
	QLabel * m_statsLabel;
	QLabel * m_flattenStatsLabel;
	QTreeView * m_subSetView;
	QTableView * m_tbl;
	MarbleMap * m_map;
	std::shared_ptr< liboscar::Static::OsmCompleter > m_cmp;
	std::unordered_map<uint64_t, SubSetNodeFlattener*> m_runningFlatteners;
	uint64_t m_subSetFlattenerRunId;
private:
	void setCompleter(QString fileName);
	void updateTextSearchTypes();
private slots:
	void viewSetCalcCompleted(const sserialize::ItemIndex& resultIdx, const sserialize::ItemIndex & cells, uint64_t runId, qlonglong timeInUsec);
public:
	OscarQtMainWindow(QString completerPath);
	virtual ~OscarQtMainWindow();
public slots:
	void selectCompleter();
	void updateStats(const oscar_gui::CompletionDispatcher::CompletionStats& stats);
	void tableViewChanged();
	void tableSelectedRowChanged(const QItemSelection & selected, const QItemSelection & deselected);
	void subSetSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
	void completionIndexChanged(const sserialize::ItemIndex & idx);
	void completionSetChanged();
	void selectCellTextCompletion();
	void selectItemsCompletion();
	void selectGeoHierarchyCompletion();
	void selectedTextSearchTypeChanged(int index);
	void selectedCompleterTypeChanged(int index);
Q_SIGNALS:
	void viewSetChanged(const sserialize::ItemIndex & resultIdx);
	void viewSetChanged(uint32_t begin, uint32_t end);
	void selectedTableItemChanged(uint32_t pos);
	void selectedSubSetItemChanged(const liboscar::Static::OsmKeyValueObjectStore::Item & item);
	void activeCellsChanged(const sserialize::ItemIndex & cells);
};

}//end namespace

#endif // oscar_gui_H
