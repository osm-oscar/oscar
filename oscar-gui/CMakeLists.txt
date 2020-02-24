project(oscar-gui)
cmake_minimum_required(VERSION 3.9)

find_package(Qt5 COMPONENTS Core Widgets Gui)
find_package(Marble)
find_package(LIBRT)

IF (Qt5Widgets_FOUND AND Qt5Gui_FOUND AND MARBLE_FOUND)

	set(MY_INCLUDE_DIRS
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


	SET(MY_LINK_LIBS
		liboscar
		sserialize
		${MARBLE_LIBRARIES}
		Qt5::Widgets
		Qt5::Gui
	)

	add_executable(${PROJECT_NAME} ${SOURCES_CPP} ${SOURCES_MOC_CPP})
	target_include_directories(${PROJECT_NAME} PRIVATE ${MY_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${MY_LINK_LIBS})
ELSE()
	message(STATUS "Will not build oscar-gui")
ENDIF()
