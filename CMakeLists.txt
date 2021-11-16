#========================================================================
#
# cmake-xpdf.txt
#
# CMake script for the Xpdf package.
#
# Copyright 2015 Glyph & Cog, LLC
#
#========================================================================

cmake_minimum_required(VERSION 2.8.8)

project(xpdf)

include(cmake-config.txt)
include(common.cmake)
include(FetchContent)
#FetchContent_Declare(
#  gRPC
#  GIT_REPOSITORY https://github.com/grpc/grpc
#  GIT_TAG        v1.41.0  # e.g v1.28.0
#)
#set(FETCHCONTENT_QUIET OFF)
#FetchContent_MakeAvailable(gRPC)

get_filename_component(hw_proto "xpdf/src/proto/" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

add_subdirectory(goo)
add_subdirectory(fofi)
add_subdirectory(splash)
add_subdirectory(xpdf)
# sunil add_subdirectory(xpdf-qt)

add_library(hw_grpc_proto
  "xpdf/src/cpp/pdftohtml.pb.cc"
  "xpdf/src/cpp/pdftohtml.grpc.pb.cc"
  "xpdf/src/cpp/pdftohtml.pb.h"
  "xpdf/src/cpp/pdftohtml.grpc.pb.h"
)
target_link_libraries(hw_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

if (NOT HAVE_FREETYPE_H)
  message(WARNING "Couldn't find FreeType -- will not build pdftoppm, pdftopng, pdftohtml, or xpdf.")
endif ()

if (NOT PNG_FOUND)
  message(WARNING "Couldn't find libpng -- will not build pdftopng or pdftohtml.")
endif ()

# if (NOT QT4_FOUND AND NOT Qt5Widgets_FOUND)
#  message(WARNING "Couldn't find Qt4 or Qt5 -- will not build xpdf.")
#endif ()

if (NOT MULTITHREADED)
  message(WARNING "Multithreading is disabled -- will not build xpdf.")
endif ()

if (NOT USE_EXCEPTIONS)
  message(WARNING "C++ exceptions are disabled -- will not build xpdf.")
endif ()
