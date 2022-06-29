add_contract(hotpotbancor hotpotbancor
   ${CMAKE_CURRENT_SOURCE_DIR}/src/hotpotbancor.cpp
)

target_include_directories(hotpotbancor
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../otcconf/include 
 )

set_target_properties(hotpotbancor
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/hotpotbancor.clauses.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/hotpotbancor.clauses.md @ONLY )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/hotpotbancor.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/hotpotbancor.contracts.md @ONLY )

target_compile_options( hotpotbancor PUBLIC -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
