cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto)

include_directories(.)

add_executable(
        transport_catalogue
        ${PROTO_SRCS}
        ${PROTO_HDRS}
        main.cpp
        geo/geo.h
        geo/geo.cpp
        json/json.h
        json/json.cpp
        json/json_builder/json_builder.cpp
        json/json_builder/json_builder.h
        svg/svg.cpp
        svg/svg.h
        transport_catalogue/domain.cpp
        transport_catalogue/domain.h
        transport_catalogue/transport_catalogue.cpp
        transport_catalogue/transport_catalogue.h
        router/router.h
        router/graph.h
        router/ranges.h
        service/transport_router/transport_router.cpp
        service/transport_router/transport_router.h
        service/json_reader/json_reader.cpp
        service/json_reader/json_reader.h
        service/map_renderer/map_renderer.cpp
        service/map_renderer/map_renderer.h
        service/serialization/serialization.cpp
        service/serialization/serialization.hpp
)

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(transport_catalogue ${Protobuf_LIBRARY} Threads::Threads)