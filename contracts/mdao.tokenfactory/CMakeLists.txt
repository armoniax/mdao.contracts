add_contract(tokenfactory mdao.tokenfactory ${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.tokenfactory.cpp)

target_include_directories(mdao.tokenfactory
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.conf/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.thirdparty
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.info/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.stake/include

   )
set_target_properties(mdao.tokenfactory
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( mdao.tokenfactory PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
