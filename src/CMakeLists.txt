add_subdirectory(3rdparty)
add_subdirectory(base)
add_subdirectory(chess)
add_subdirectory(game)

add_executable(banksia
  main.cpp)
target_link_libraries(banksia
  cpptime json process fathom
  game chess base)
