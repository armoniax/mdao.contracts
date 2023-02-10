add_contract(tokenfactory mdao.token.factory ${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.token.factory.cpp)

target_include_directories(mdao.token.factory
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.conf/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.thirdparty
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.info/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.stake/include

   )
set_target_properties(mdao.token.factory
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( mdao.token.factory PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
