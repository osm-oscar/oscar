#ifndef OSCAR_GUI_MAIN_WINDOW_H
#define OSCAR_GUI_MAIN_WINDOW_H
#include <QMainWindow>

#include <liboscar/StaticOsmCompleter.h>

#include "States.h"
#include "StateHandlers.h"
#include "MarbleMap.h"

class QTableView;
class QSpinBox;
class QPushButton;
class QComboBox;
class QSlider;
class QLineEdit;

namespace oscar_gui {

class MarbleMap;
class SidebarWidget;

class MainWindow : public QMainWindow {
Q_OBJECT
public:
	MainWindow(const std::shared_ptr<liboscar::Static::OsmCompleter> & cmp);
	virtual ~MainWindow();
signals:
	void colorSchemeChanged(int colorScheme);
	
	void triangleAdded(MarbleMap::FaceId triangleId);
	void triangleRemoved(MarbleMap::FaceId triangleId);
	
	void cellAdded(uint32_t cellId);
	void cellRemoved(uint32_t cellId);
	
	void itemAdded(uint32_t itemId);
	void itemRemoved(uint32_t itemId);
private slots:
	void searchResultsChanged(const QString &, const sserialize::CellQueryResult & cqr, const sserialize::ItemIndex &);
private://GUI
	MarbleMap * m_map;
	SidebarWidget * m_sidebar;
private:
	std::shared_ptr<liboscar::Static::OsmCompleter> m_completer;
	States m_states;
	StateHandlers m_stateHandlers;
};

}//end namespace

#endif
