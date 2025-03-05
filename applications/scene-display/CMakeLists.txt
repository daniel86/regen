
SET(_HEADERS scene-display-widget.h)
QT5_WRAP_CPP(_HEADERS_MOC ${_HEADERS})

QT5_WRAP_UI(UI_HEADERS scene-display-gui.ui)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(
    ${PROJECT_NAME}-scene-display
    scene-display.cpp
    scene-display-widget.cpp
    ${_HEADERS_MOC}
    ${UI_HEADERS}
)
target_compile_options(
        ${PROJECT_NAME}-scene-display
        PRIVATE -fPIC)
target_link_libraries(
    ${PROJECT_NAME}-scene-display
    qt-application
    ${PROJECT_NAME}
    Qt5::Core Qt5::Gui Qt5::OpenGL
)

set(HEADER_INSTALL_PATH include/${PROJECT_NAME}/applications)
file(GLOB HEADER_FILES *.h)

install(TARGETS ${PROJECT_NAME}-scene-display RUNTIME DESTINATION bin)
install(FILES ${HEADER_FILES} DESTINATION ${HEADER_INSTALL_PATH}/scene-display)
install(DIRECTORY examples DESTINATION share/${PROJECT_NAME})
