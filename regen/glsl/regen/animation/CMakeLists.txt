
# list of all shader files in this directory
file(GLOB SHADER_FILES *.glsl)
# name of this directory
get_filename_component(DIRNAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
# inst all all shader files
install(
    FILES ${SHADER_FILES}
    DESTINATION ${SHADER_INSTALL_PATH}/glsl/regen/${DIRNAME}
)
