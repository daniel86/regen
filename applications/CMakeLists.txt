
set(USE_QT_TEST_APPLICATIONS 1)
set(USE_QT_FLUID_APPLICATION 0)

configure_file( application-config.h.cmake application-config.h )

add_library( glut-application STATIC
       test-render-tree.cpp
       test-camera-manipulator.cpp
       ogle-application.cpp
       glut-ogle-application.cpp
       )

##################

SET(QT_USE_QTOPENGL TRUE)

find_package(Qt4 REQUIRED)

QT4_WRAP_CPP(HEADERS_MOC qt-gl-widget.h)
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

add_library( qt-application STATIC
       test-render-tree.cpp
       test-camera-manipulator.cpp
       ogle-application.cpp
       qt-gl-widget.cpp
       qt-ogle-application.cpp
       ${HEADERS_MOC}
       )

add_subdirectory(test)
add_subdirectory(eulerian-fluid)
