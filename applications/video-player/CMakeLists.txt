
SET(_HEADERS video-player-widget.h)
QT4_WRAP_CPP(_HEADERS_MOC ${_HEADERS})

set(APPLIBS qt-application ${QT_LIBRARIES} ${ENGINE_LIBS})

add_executable( ogle-video-player video-player.cpp video-player-widget.cpp ${_HEADERS_MOC} )
target_link_libraries( ogle-video-player ${APPLIBS} )
