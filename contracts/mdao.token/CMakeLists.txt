add_contract(mdao.token mdao.token ${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.token.cpp)

target_include_directories(mdao.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.conf/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.stg/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.thirdparty
   )
set_target_properties(mdao.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( mdao.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
