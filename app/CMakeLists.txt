
cmake_minimum_required(VERSION 3.4.1)

#project (Droid2600)

add_definitions ( -DBSPF_UNIX -D_GLIBCXX_USE_CXX11_ABI=1 -DHAVE_GETTIMEOFDAY -D__ANDROID__ -DTHUMB_SUPPORT -DSOUND_SUPPORT )

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-ignored-qualifiers -Wno-multichar -Wunused -fno-rtti -Woverloaded-virtual -Wnon-virtual-dtor -std=c++11 -fomit-frame-pointer")

include_directories (
      src/main/cpp/emu/adapter
      src/main/cpp/emu/common
      src/main/cpp/emu/emucore
      src/main/cpp/emu/gui
)

file( GLOB srcfiles
      "src/main/cpp/*.cpp"
      "src/main/cpp/emu/*.cpp"
      "src/main/cpp/emu/adapter/*.cpp"
      "src/main/cpp/emu/emucore/*.cxx"
      "src/main/cpp/emu/common/*.cxx"
      "src/main/cpp/emu/gui/*.cxx"
)

find_library( LIB_NDK_LOG_LIBRARY log )

add_library( Droid2600 SHARED ${srcfiles} )

target_link_Libraries( Droid2600 LINK_PRIVATE ${LIB_NDK_LOG_LIBRARY} )
