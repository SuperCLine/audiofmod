file(GLOB SOURCES src/*.cpp)

if (BUILD_AUDIOFMOD_STATIC)
    add_library(audiofmod STATIC ${SOURCES})
else ()
    add_library(audiofmod SHARED ${SOURCES})
endif ()

add_library(SuperCLine::audiofmod ALIAS audiofmod)

add_definitions(-DAUDIOFMOD_API=__DLL_EXPORT)
add_definitions(-DCORE_API=__DLL_IMPORT)
if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif ()

# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_REQUIRED_FLAGS -std=c++17)
# set_property(TARGET audio PROPERTY CXX_STANDARD 17)

add_dependencies(audiofmod core)
target_link_libraries(audiofmod core fmod_event fmodex_vc)

target_link_directories(audiofmod PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fmod/lib")
target_include_directories(audiofmod
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/inc/private" "${CMAKE_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fmod/include" "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/fmod/include/designer"
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc/public"
)

set_target_properties(audiofmod PROPERTIES
    SOVERSION ${SOVERSION}
    VERSION ${VERSION}
    COTIRE_CXX_PREFIX_HEADER_INIT "${CMAKE_CURRENT_SOURCE_DIR}/inc/private/stdafx.h"
)
cotire(audiofmod)

install(TARGETS audiofmod
    ARCHIVE DESTINATION lib/audiofmod
    LIBRARY DESTINATION lib/audiofmod
    RUNTIME DESTINATION bin
    COMPONENT library
)

file(GLOB INCLUDES inc/public/*.h)
install(FILES ${INCLUDES} DESTINATION include/audiofmod)