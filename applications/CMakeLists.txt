
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(BUILD_GLUT_APPLICATION)
    add_subdirectory(glut)
endif()

if(BUILD_TESTS OR BUILD_VIDEO_PLAYER)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    #find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
    #find_package(Qt COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
    find_package(Qt5 COMPONENTS Core Gui OpenGL REQUIRED)
    #include(${QT_USE_FILE})
    include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5OpenGL_INCLUDE_DIRS})
    add_definitions(${QT_DEFINITIONS})
    add_subdirectory(qt)
endif()

if(BUILD_TESTS)
    add_subdirectory(scene-display)
    add_subdirectory(mesh-viewer)
    add_subdirectory(noise-gui)
endif()

if(BUILD_VIDEO_PLAYER AND HAS_AV_LIBS)
    add_subdirectory(video-player)
endif()
