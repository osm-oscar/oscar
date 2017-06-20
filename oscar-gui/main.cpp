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
	std::shared_ptr<liboscar::Static::OsmCompleter> cmp( new liboscar::Static::OsmCompleter() );
	
	cmp->setAllFilesFromPrefix(initialCmpFileName.toStdString());
	try {
		cmp->energize();
	}
	catch(const std::exception & e) {
		std::cerr << "Error:" << e.what() << std::endl;
		return -1;
	}
	
// 	qRegisterMetaType<uint64_t>("uint64_t");
	oscar_gui::MainWindow mainWindow(cmp);
	mainWindow.show();
	return app.exec();
}
