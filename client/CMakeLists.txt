file(GLOB CLIENT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB CLIENT_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(taller_client
    PRIVATE
    ${CLIENT_SRC}
    PUBLIC
    ${CLIENT_HEADERS}
)