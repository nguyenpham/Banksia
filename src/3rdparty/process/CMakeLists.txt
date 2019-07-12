add_library(process OBJECT
  process.cpp process.hpp)
#target_include_directories(process .)

if(WIN32)
  target_sources(process
    PRIVATE process_win.cpp)
else()
  target_sources(process
    PRIVATE process_unix.cpp)
  set_source_files_properties(process_unix.cpp
    PROPERTIES COMPILE_DEFINITIONS _POSIX_C_SOURCE=1)
endif()
