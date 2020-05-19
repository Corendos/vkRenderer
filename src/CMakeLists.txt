file(
    GLOB_RECURSE
    src
    *.cpp
)

file(
    GLOB_RECURSE
    header
    ${PROJECT_SOURCE_DIR}/include/*.hpp
)

set(CURRENT_PROJECT ${CMAKE_PROJECT_NAME})

# Declare executable
add_executable(${CURRENT_PROJECT} main.cpp)

# Include local include files
target_include_directories(${CURRENT_PROJECT} PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Include Vulkan
target_include_directories(${CURRENT_PROJECT} PUBLIC ${Vulkan_INCLUDE_DIR})
target_link_libraries(${CURRENT_PROJECT} PUBLIC ${Vulkan_LIBRARIES})

# Include GLFW
target_include_directories(${CURRENT_PROJECT} PUBLIC ${glfw3_INCLUDE_DIR})
target_link_libraries(${CURRENT_PROJECT} PUBLIC glfw)

# Include Assimp
target_include_directories(${CURRENT_PROJECT} PUBLIC ${assimp_INCLUDE_DIR})
target_link_libraries(${CURRENT_PROJECT} PUBLIC assimp)

# Include GLM
target_include_directories(${CURRENT_PROJECT} PUBLIC /usr/include/glm)

# Set output location
set_target_properties(${CURRENT_PROJECT}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
)

add_definitions(-DPROGRAM_ROOT="${PROJECT_SOURCE_DIR}")

add_custom_target(shaders
	glslc ${PROJECT_SOURCE_DIR}/resources/shaders/basic.vert -o ${PROJECT_SOURCE_DIR}/resources/shaders/basic.vert.spv
	COMMAND glslc ${PROJECT_SOURCE_DIR}/resources/shaders/basic.frag -o ${PROJECT_SOURCE_DIR}/resources/shaders/basic.frag.spv
	COMMAND glslc ${PROJECT_SOURCE_DIR}/resources/shaders/gui.vert -o ${PROJECT_SOURCE_DIR}/resources/shaders/gui.vert.spv
	COMMAND glslc ${PROJECT_SOURCE_DIR}/resources/shaders/gui.frag -o ${PROJECT_SOURCE_DIR}/resources/shaders/gui.frag.spv)
