add_contract(xdao.stg xdao.stg
${CMAKE_CURRENT_SOURCE_DIR}/src/xdao.stg.cpp
)

target_include_directories(xdao.stg
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(xdao.stg
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/xdao.stg.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/xdao.stg.contracts.md @ONLY )

target_compile_options( xdao.stg PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
