add_contract(hotpottoken hotpottoken ${CMAKE_CURRENT_SOURCE_DIR}/src/hotpottoken.cpp)

target_include_directories(hotpottoken
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(hotpottoken
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/hotpottoken.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/hotpottoken.contracts.md @ONLY )

target_compile_options( hotpottoken PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
