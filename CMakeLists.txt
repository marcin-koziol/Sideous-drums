cmake_minimum_required(VERSION 3.15)
project(sideous-drums)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# dpf/ is a vendored submodule; we carry a small patch on top of it (adds a
# Plugin::getMidiNoteName() hook, used to expose fixed drum-note names to hosts via VST3
# IUnitInfo pitch names and Ardour's LV2 MIDNAM extension - see patches/README.md).
# Applied automatically and idempotently so a fresh `git submodule update` just works.
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/dpf/distrho/DistrhoPlugin.hpp")
  message(FATAL_ERROR "dpf/ submodule not found - run: git submodule update --init --recursive")
endif()

file(READ "${CMAKE_SOURCE_DIR}/dpf/distrho/DistrhoPlugin.hpp" DPF_PLUGIN_HPP_CONTENTS)
if(NOT DPF_PLUGIN_HPP_CONTENTS MATCHES "getMidiNoteName")
  message(STATUS "Applying patches/dpf-midi-note-names.patch to dpf/")
  execute_process(
    COMMAND git apply "${CMAKE_SOURCE_DIR}/patches/dpf-midi-note-names.patch"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/dpf"
    RESULT_VARIABLE DPF_PATCH_RESULT
  )
  if(NOT DPF_PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to apply patches/dpf-midi-note-names.patch to dpf/ (result ${DPF_PATCH_RESULT})")
  endif()
endif()

add_subdirectory(dpf)
add_subdirectory(plugins/SideousDrums)
add_subdirectory(plugins/SideousDrumsMulti)
