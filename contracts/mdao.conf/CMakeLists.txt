add_contract(mdao.conf mdao.conf ${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.conf.cpp)

target_include_directories(mdao.conf
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include/mdao.conf
)

set_target_properties(mdao.conf
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( mdao.conf PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
