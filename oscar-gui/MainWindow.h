#ifndef OSCAR_GUI_MAIN_WINDOW_H
#define OSCAR_GUI_MAIN_WINDOW_H
#include <QMainWindow>

#include <liboscar/StaticOsmCompleter.h>

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
	
	void triangleAdded(uint32_t triangleId);
	void triangleRemoved(uint32_t triangleId);
	
	void cellAdded(uint32_t cellId);
	void cellRemoved(uint32_t cellId);
	
	void itemAdded(uint32_t itemId);
	void itemRemoved(uint32_t itemId);
private://GUI
	MarbleMap * m_map;
	SidebarWidget * m_sidebar;
private slots:
	void changeColorScheme(int index);
	void toggleCell(uint32_t cellId);
private:
	std::shared_ptr<liboscar::Static::OsmCompleter> m_completer;
};

}//end namespace

#endif