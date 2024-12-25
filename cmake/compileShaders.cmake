set(BUILD_SHADERS_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/build_shaders.py")
set(SHADERS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADERS_DEST_DIR "${CMAKE_BINARY_DIR}/shaders")

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Running build_shaders.py script..."
        COMMAND python ${BUILD_SHADERS_SCRIPT} 
                ${SHADERS_SOURCE_DIR}
                ${SHADERS_DEST_DIR}
        COMMENT "Running build_shaders.py")