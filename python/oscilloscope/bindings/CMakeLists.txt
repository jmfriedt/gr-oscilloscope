# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT oscilloscope_sources)
    message(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT oscilloscope_sources)

########################################################################
# Check for pygccxml
########################################################################
gr_python_check_module_raw("pygccxml" "import pygccxml" PYGCCXML_FOUND)

include(GrPybind)

########################################################################
# Python Bindings
########################################################################

list(APPEND oscilloscope_python_files
    oscilloscope_python.cc python_bindings.cc)

gr_pybind_make_oot(oscilloscope ../../.. gr::oscilloscope "${oscilloscope_python_files}")

# copy bindings extension for use in QA test module
add_custom_command(
    TARGET oscilloscope_python
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:oscilloscope_python>
            ${PROJECT_BINARY_DIR}/test_modules/gnuradio/oscilloscope/)

install(
    TARGETS oscilloscope_python
    DESTINATION ${GR_PYTHON_DIR}/gnuradio/oscilloscope
    COMPONENT pythonapi)
