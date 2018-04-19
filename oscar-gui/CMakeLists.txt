project(oscar-gui)
cmake_minimum_required(VERSION 3.7)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 14)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Marble REQUIRED)
find_package(LIBRT)

IF (MARBLE_FOUND)

	include_directories(
		${LIBOSCAR_INCLUDE_DIR}
		${QT_INCLUDES}
		${MARBLE_INCLUDE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
	)

	set(SOURCES_MOC_H
		MainWindow.h
		MarbleMap.h
		SidebarWidget.h
		GeometryInputWidget.h
		ItemDetailsWidget.h
		SearchInputWidget.h
		SearchResultsWidget.h
		VisualizationOptionsWidget.h
		States.h
		SearchGeometryModel.h
		StateHandlers.h
		ResultsTableModel.h
	)

	set(SOURCES_CPP
		main.cpp
		MainWindow.cpp
		MarbleMap.cpp
		SidebarWidget.cpp
		GeometryInputWidget.cpp
		ItemDetailsWidget.cpp
		SearchInputWidget.cpp
		SearchResultsWidget.cpp
		VisualizationOptionsWidget.cpp
		States.cpp
		SearchGeometryModel.cpp
		SearchGeometryHelper.cpp
		StateHandlers.cpp
		ResultsTableModel.cpp
	)

	qt5_wrap_cpp(SOURCES_MOC_CPP ${SOURCES_MOC_H})


	SET(OSCAR_LINK_LIBS
		${LIBOSCAR_LINK_LIBRARIES}
		${MARBLE_LIBRARIES}
		Qt5::Widgets
		Qt5::Gui
	)

	add_executable(${PROJECT_NAME} ${SOURCES_CPP} ${SOURCES_MOC_CPP})
	add_dependencies(${PROJECT_NAME} liboscar sserialize)
	target_link_libraries(${PROJECT_NAME} ${OSCAR_LINK_LIBS})

ENDIF(MARBLE_FOUND)
