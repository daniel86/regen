set(_HEADERS mesh-viewer-widget.h)
QT5_WRAP_CPP(_HEADERS_MOC ${_HEADERS})

QT5_WRAP_UI(UI_HEADERS mesh-viewer-gui.ui)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_executable(
    ${PROJECT_NAME}-mesh-viewer
    mesh-viewer.cpp
    mesh-viewer-widget.cpp
    ${_HEADERS_MOC}
    ${UI_HEADERS}
)
target_compile_options(
        ${PROJECT_NAME}-mesh-viewer
        PRIVATE -fPIC)
target_link_libraries(
    ${PROJECT_NAME}-mesh-viewer
    qt-application
    ${PROJECT_NAME}
    Qt5::Core Qt5::Gui Qt5::OpenGL
)

set(HEADER_INSTALL_PATH include/${PROJECT_NAME}/applications)
file(GLOB HEADER_FILES *.h)

install(TARGETS ${PROJECT_NAME}-mesh-viewer RUNTIME DESTINATION bin)
install(FILES ${HEADER_FILES} DESTINATION ${HEADER_INSTALL_PATH}/mesh-viewer)
