
IF(BUILD_GLUT_APPLICATION)
    add_subdirectory(glut)
ENDIF()

IF(BUILD_TESTS OR BUILD_VIDEO_PLAYER)
    FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
    INCLUDE(${QT_USE_FILE})
    ADD_DEFINITIONS(${QT_DEFINITIONS})
    add_subdirectory(qt)
ENDIF()

IF(BUILD_TESTS)
    add_subdirectory(test)
ENDIF()

IF(BUILD_VIDEO_PLAYER)
    add_subdirectory(video-player)
ENDIF()

IF(BUILD_TEXTURE_UPDATER)
    add_subdirectory(texture-updater)
ENDIF()
