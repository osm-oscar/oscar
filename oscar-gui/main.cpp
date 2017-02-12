#include <QtGui/QApplication>
#include "OscarQtMainWindow.h"

using namespace oscar_gui;

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	QStringList cmdline_args = QCoreApplication::arguments();
	QString initialCmpFileName;
	if (cmdline_args.size() > 1)
		initialCmpFileName = cmdline_args.at(1);
	qRegisterMetaType<sserialize::ItemIndex>("sserialize::ItemIndex");
	qRegisterMetaType<uint64_t>("uint64_t");
	OscarQtMainWindow mainWindow(initialCmpFileName);
	mainWindow.show();
	mainWindow.setMapThemeId("earth/openstreetmap/openstreetmap.dgml");
	return app.exec();
}
