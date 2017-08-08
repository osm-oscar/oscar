project(oscar-gui)
cmake_minimum_required(VERSION 2.8)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

find_package(Qt4)
find_package(Marble REQUIRED)
find_package(LIBRT)

IF (QT4_FOUND AND MARBLE_FOUND)

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
	)

	qt4_wrap_cpp(SOURCES_MOC_CPP ${SOURCES_MOC_H})


	SET(OSCAR_LINK_LIBS
		${LIBOSCAR_LINK_LIBRARIES}
		${MARBLE_LIBRARIES}
		${QT_QTCORE_LIBRARY}
		${QT_QTGUI_LIBRARY}
	)

	add_executable(${PROJECT_NAME} ${SOURCES_CPP} ${SOURCES_MOC_CPP})
	add_dependencies(${PROJECT_NAME} liboscar sserialize)
	target_link_libraries(${PROJECT_NAME} ${OSCAR_LINK_LIBS})


ENDIF(QT4_FOUND AND MARBLE_FOUND)
