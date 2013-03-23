
SET(_HEADERS texture-updater-widget.h)
QT4_WRAP_CPP(_HEADERS_MOC ${_HEADERS})

set(APPLIBS qt-application ${QT_LIBRARIES} ${ENGINE_LIBS})

add_executable( ogle-texture-updater texture-updater.cpp texture-updater-widget.cpp ${_HEADERS_MOC} )
target_link_libraries( ogle-texture-updater ${APPLIBS} )
