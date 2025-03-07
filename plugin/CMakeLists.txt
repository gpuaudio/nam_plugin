set(PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_FILES
        "${PROJECT_DIR}/config.h"
        "${PROJECT_DIR}/architecture.hpp"
        "${PROJECT_DIR}/choc_DisableAllWarnings.h"
        "${PROJECT_DIR}/choc_ReenableAllWarnings.h"
        "${PROJECT_DIR}/Colors.h"
        "${PROJECT_DIR}/NeuralAmpModeler.cpp"
        "${PROJECT_DIR}/NeuralAmpModeler.h"
        "${PROJECT_DIR}/NeuralAmpModelerControls.h"
        "${PROJECT_DIR}/ToneStack.cpp"
        "${PROJECT_DIR}/ToneStack.h"
)
source_group(TREE ${PROJECT_DIR} FILES ${SRC_FILES})

# Patch: nam_plugin/deps/iPlug2/Scripts/cmake/FindiPlug2.cmake
# function(iplug_target_bundle_resources target res_dir)
if(APPLE)
    file(GLOB_RECURSE FONTS CONFIGURE_DEPENDS "${PROJECT_DIR}/resources/fonts/*")
    file(GLOB_RECURSE IMAGES CONFIGURE_DEPENDS "${PROJECT_DIR}/resources/img/*")
    set(RESOURCES "${FONTS}" "${IMAGES}")
else()
    set(RESOURCES
        "${PROJECT_DIR}/resources/fonts/Roboto-Regular.ttf"
        "${PROJECT_DIR}/resources/fonts/Michroma-Regular.ttf"
    )
endif()

add_library(_base INTERFACE)
iplug_target_add(_base INTERFACE
        INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources
        LINK iPlug2_NanoVG_GL2
        FEATURE cxx_std_20)

# Patch: nam_plugin/deps/iPlug2/Scripts/cmake/IGraphics.cmake
if(APPLE)
    set(_src ${IGRAPHICS_SRC}/Drawing/IGraphicsNanoVG_src.m)
    set_source_files_properties(${_src} PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
endif()

if (CMAKE_GENERATOR MATCHES "Visual Studio")
    target_link_libraries(_base INTERFACE AudioDSPTools namlib_mt)
else()
    target_link_libraries(_base INTERFACE AudioDSPTools namlib)
endif()

# For typing convenience the TARGET name is put into a variable.
set(TARGET app)
add_executable(${TARGET} WIN32 MACOSX_BUNDLE ${SRC_FILES})
iplug_target_add(${TARGET} PUBLIC LINK iPlug2_APP _base RESOURCE ${RESOURCES})
iplug_configure_target(${TARGET} app)

set(TARGET vst3)
add_library(${TARGET} MODULE ${SRC_FILES})
iplug_target_add(${TARGET} PUBLIC LINK iPlug2_VST3 _base RESOURCE ${RESOURCES})
iplug_configure_target(${TARGET} vst3)

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(TARGET auv2)
    add_library(${TARGET} MODULE ${SRC_FILES})
    iplug_target_add(${TARGET} PUBLIC LINK iPlug2_AUv2 _base RESOURCE ${RESOURCES})
    iplug_configure_target(${TARGET} auv2)
endif ()

if (WIN32)
    set(CMAKE_RC_FLAGS "/I${PROJECT_DIR}/resources /I${PROJECT_DIR}/resources/fonts /I${PROJECT_DIR}/resources/img ${CMAKE_RC_FLAGS}")
endif ()
