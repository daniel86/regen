
set(_HEADERS video-player-widget.h)
QT4_WRAP_CPP(_HEADERS_MOC ${_HEADERS})

add_executable(
    ${PROJECT_NAME}-video-player
    video-player.cpp
    video-player-widget.cpp
    ${_HEADERS_MOC}
)
target_link_libraries(
    ${PROJECT_NAME}-video-player
    qt-application
    ${PROJECT_NAME}
    ${QT_LIBRARIES}
)

