cmake_minimum_required(VERSION 3.20)
project(WPFestivalSapi5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(SPEECHSDK51_ROOT
  "${CMAKE_CURRENT_SOURCE_DIR}/../tools/SpeechSDK51_admin/program files/Microsoft Speech SDK 5.1"
)

if(NOT EXISTS "${SPEECHSDK51_ROOT}/Include/sapi.h")
  set(SPEECHSDK51_ROOT
    "C:/Users/turek/Desktop/blackbox/tools/SpeechSDK51_admin/program files/Microsoft Speech SDK 5.1"
  )
endif()

if(NOT EXISTS "${SPEECHSDK51_ROOT}/Include/sapi.h")
  message(FATAL_ERROR "Nie znaleziono Speech SDK 5.1 headers.")
endif()

function(add_wp_festival_variant target_name dll_output_name exe_output_name)
  set(profile_macro)
  if(target_name STREQUAL "WPFestivalSapi5Female")
    set(profile_macro WP_FESTIVAL_PROFILE_FEMALE)
  endif()

  add_library(${target_name} SHARED
    engine/wp_festival_sapi5.cpp
    engine/wp_festival_sapi5.def
  )

  target_compile_definitions(${target_name} PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    ${profile_macro}
  )
  target_include_directories(${target_name} PRIVATE "${SPEECHSDK51_ROOT}/Include" "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_options(${target_name} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHsc /utf-8>
  )
  target_link_libraries(${target_name} PRIVATE ole32 uuid)
  set_target_properties(${target_name} PROPERTIES OUTPUT_NAME "${dll_output_name}")

  add_executable(${target_name}Config WIN32
    config/wp_festival_sapi5_config.cpp
  )

  target_compile_definitions(${target_name}Config PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    ${profile_macro}
  )
  target_include_directories(${target_name}Config PRIVATE "${SPEECHSDK51_ROOT}/Include" "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_options(${target_name}Config PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /EHsc /utf-8>
  )
  target_link_libraries(${target_name}Config PRIVATE comctl32 ole32)
  set_target_properties(${target_name}Config PROPERTIES OUTPUT_NAME "${exe_output_name}")
endfunction()

add_wp_festival_variant(WPFestivalSapi5 "WPFestivalSapi5" "WPFestivalSapi5Config")
add_wp_festival_variant(WPFestivalSapi5Female "WPFestivalSapi5Female" "WPFestivalSapi5FemaleConfig")
