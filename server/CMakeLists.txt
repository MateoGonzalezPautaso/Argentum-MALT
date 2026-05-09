file(GLOB SERVER_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB SERVER_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(taller_server
    PRIVATE
    ${SERVER_SRC}
    PUBLIC
    ${SERVER_HEADERS}
)