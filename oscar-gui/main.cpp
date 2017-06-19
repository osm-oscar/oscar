#include <QtGui/QApplication>
#include "MainWindow.h"

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	QStringList cmdline_args = QCoreApplication::arguments();
	QString initialCmpFileName;
	if (cmdline_args.size() > 1) {
		initialCmpFileName = cmdline_args.at(1);
	}
// 	qRegisterMetaType<uint64_t>("uint64_t");
	oscar_gui::MainWindow mainWindow();
	mainWindow.show();
	return app.exec();
}
