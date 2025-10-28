find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_OSCILLOSCOPE gnuradio-oscilloscope)

FIND_PATH(
    GR_OSCILLOSCOPE_INCLUDE_DIRS
    NAMES gnuradio/oscilloscope/api.h
    HINTS $ENV{OSCILLOSCOPE_DIR}/include
        ${PC_OSCILLOSCOPE_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_OSCILLOSCOPE_LIBRARIES
    NAMES gnuradio-oscilloscope
    HINTS $ENV{OSCILLOSCOPE_DIR}/lib
        ${PC_OSCILLOSCOPE_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-oscilloscopeTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_OSCILLOSCOPE DEFAULT_MSG GR_OSCILLOSCOPE_LIBRARIES GR_OSCILLOSCOPE_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_OSCILLOSCOPE_LIBRARIES GR_OSCILLOSCOPE_INCLUDE_DIRS)
