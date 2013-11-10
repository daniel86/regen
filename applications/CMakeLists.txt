
if(BUILD_GLUT_APPLICATION)
    add_subdirectory(glut)
endif()

if(BUILD_TESTS OR BUILD_VIDEO_PLAYER)
    find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
    include(${QT_USE_FILE})
    add_definitions(${QT_DEFINITIONS})
    add_subdirectory(qt)
endif()

if(BUILD_TESTS)
    add_subdirectory(scene-display)
endif()

if(BUILD_VIDEO_PLAYER)
    add_subdirectory(video-player)
endif()
