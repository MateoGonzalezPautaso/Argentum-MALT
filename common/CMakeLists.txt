file(GLOB COMMON_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB COMMON_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(taller_common
    PRIVATE
    ${COMMON_SRC}
    PUBLIC
    ${COMMON_HEADERS}
)
