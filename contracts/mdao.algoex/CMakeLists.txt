add_contract(mdao.algoex mdao.algoex
   ${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.algoex.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.algoexdb.cpp
)

target_include_directories(mdao.algoex
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.conf/include   

 )

set_target_properties(mdao.algoex
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/mdao.algoex.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/mdao.algoex.contracts.md @ONLY )

target_compile_options( mdao.algoex PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
