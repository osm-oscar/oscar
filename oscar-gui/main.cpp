#include <QApplication>

#include <liboscar/StaticOsmCompleter.h>

#include "MainWindow.h"

// Kaputt: cell anzeigen von :north-of #"Stuttgart" für bw Datensatz von plankton

void help() {
	std::cout << "prg -f <path to oscar data> -cd (annulus|sphere|minsphere|mass)" << std::endl;
}

int main(int argc, char** argv)
{
	qRegisterMetaType<sserialize::ItemIndex>("sserialize::ItemIndex");
	qRegisterMetaType<sserialize::CellQueryResult>("sserialize::CellQueryResult");
	qRegisterMetaType<sserialize::Static::spatial::Triangulation::FaceId>("sserialize::Static::spatial::Triangulation::FaceId");
	
	QApplication app(argc, argv);
	QStringList cmdline_args = QCoreApplication::arguments();
	QString initialCmpFileName;
	liboscar::Static::OsmCompleter::CellDistanceType cd = liboscar::Static::OsmCompleter::CDT_CENTER_OF_MASS;
	
	
	for(int i(1), s(cmdline_args.size()); i < s; ++i) {
		QString token(cmdline_args.at(i));
		
		if (token == "-f" && i+1 < s) {
			initialCmpFileName = cmdline_args.at(i+1);
			++i;
		}
		else if (token == "-cd" && i+1 < s) {
			token = cmdline_args.at(i+1);
			if (token == "annulus") {
				cd = liboscar::Static::OsmCompleter::CDT_ANULUS;
			}
			else if (token == "sphere") {
				cd = liboscar::Static::OsmCompleter::CDT_SPHERE;
			}
			else if (token == "minsphere") {
				cd = liboscar::Static::OsmCompleter::CDT_MIN_SPHERE;
			}
			else if (token == "mass") {
				cd = liboscar::Static::OsmCompleter::CDT_CENTER_OF_MASS;
			}
			else {
				std::cerr << "Unkown cell distance: " << token.toStdString() << std::endl;
				return -1;
			}
			++i;
		}
		else if (token == "--help" || token == "-h") {
			help();
			return 0;
		}
		else {
			std::cerr << "Unkown commandline argument: " << token.toStdString() << std::endl;
			return -1;
		}
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
	
	cmp->setCellDistance(cd, 0);
	
	oscar_gui::MainWindow mainWindow(cmp);
	mainWindow.show();
	return app.exec();
}
