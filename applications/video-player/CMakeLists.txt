
set(_HEADERS video-player-widget.h)
QT5_WRAP_CPP(_HEADERS_MOC ${_HEADERS})

add_executable(
    ${PROJECT_NAME}-video-player
    video-player.cpp
    video-player-widget.cpp
    ${_HEADERS_MOC}
)
target_compile_options(
        ${PROJECT_NAME}-video-player
        PRIVATE -fPIC)
target_link_libraries(
    ${PROJECT_NAME}-video-player
    qt-application
    ${PROJECT_NAME}
    Qt5::Core Qt5::Gui Qt5::OpenGL
)
