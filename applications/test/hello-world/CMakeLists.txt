
add_executable(hello-world hello-world.cpp)
target_link_libraries( hello-world glut-application ${OGLE_LIBS} )
