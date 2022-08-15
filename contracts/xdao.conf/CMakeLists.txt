add_contract(xdaoconftest xdao.conf ${CMAKE_CURRENT_SOURCE_DIR}/src/xdao.conf.cpp)

target_include_directories(xdao.conf
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include/xdao.conf
)

set_target_properties(xdao.conf
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( xdao.conf PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
