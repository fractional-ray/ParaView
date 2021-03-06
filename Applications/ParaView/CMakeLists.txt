cmake_dependent_option(PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION
  "Build the ParaView help menu documentation for ParaView and its plugins" ON
  "PARAVIEW_BUILD_QT_GUI" ON)
mark_as_advanced(PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)

set(xmls
  "${CMAKE_CURRENT_SOURCE_DIR}/ParaViewFilters.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/ParaViewSources.xml")

if (PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)
  #------------------------------------------------------------------------------
  # Build Online-Help (aka Embedded Help) for the ParaView application.
  # This is done after the above piece of code that sets the application_gui_xmls
  # variable. Documentation/CMakeLists.txt depends on it.
  add_subdirectory(Documentation)
endif ()

if (DEFINED PARAVIEW_VERSION_NICKNAME)
  set(paraview_version "${PARAVIEW_VERSION_NICKNAME}")
else ()
  set(paraview_version "${PARAVIEW_VERSION_FULL}")
endif ()
set(paraview_title "")

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTOUIC 1)

set(sources
  ParaViewMainWindow.cxx
  ParaViewMainWindow.h
  ParaViewMainWindow.ui)

set(paraview_forward_executable OFF)
if (UNIX AND NOT APPLE)
  set(paraview_forward_executable ON)
endif ()

find_package(Qt5 REQUIRED QUIET COMPONENTS Core Widgets)

paraview_client_add(
  NAME              paraview
  NAMESPACE         "ParaView"
  EXPORT            "ParaView"
  APPLICATION_NAME  "ParaView"
  TITLE             "ParaView ${paraview_version}"
  ORGANIZATION      "ParaView"
  VERSION           "${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}.${PARAVIEW_VERSION_PATCH}"
  MAIN_WINDOW_CLASS "ParaViewMainWindow"
  BUNDLE_ICON       "${CMAKE_CURRENT_SOURCE_DIR}/pvIcon.icns"
  APPLICATION_ICON  "${CMAKE_CURRENT_SOURCE_DIR}/pvIcon.ico"
  SOURCES           ${sources}
  PLUGINS_TARGET    paraview_plugins
  APPLICATION_XMLS  ${xmls}
  FORWARD_EXECUTABLE "${paraview_forward_executable}")

find_package(Qt5 REQUIRED QUIET COMPONENTS Widgets)

target_link_libraries(paraview
  PRIVATE
    ParaView::ServerManagerDefault
    Qt5::Core
    Qt5::Widgets)

if (PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)
  target_compile_definitions(paraview
    PRIVATE
      PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION)
  target_link_libraries(paraview
    PRIVATE
      vtkParaViewDocumentation)
endif ()

if (PARAVIEW_ENABLE_PYTHON)
  target_link_libraries(paraview
    PRIVATE
      ParaView::pqPython
      ParaView::PythonInitializer)
endif ()

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()

if (UNIX AND NOT APPLE)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/org.paraview.ParaView.desktop.in"
    "${CMAKE_CURRENT_BINARY_DIR}/org.paraview.ParaView.desktop"
    @ONLY)
  install(
    FILES       "${CMAKE_CURRENT_BINARY_DIR}/org.paraview.ParaView.desktop"
    DESTINATION share/applications
    COMPONENT   runtime)
  foreach (iconsize IN ITEMS 22x22 32x32 96x96)
    install(
      FILES       "pvIcon-${iconsize}.png"
      DESTINATION "share/icons/hicolor/${iconsize}/apps"
      RENAME      paraview.png
      COMPONENT   runtime)
  endforeach ()
  install(
    FILES       org.paraview.ParaView.appdata.xml
    DESTINATION share/metainfo
    COMPONENT   runtime)
endif ()
