cmake_minimum_required(VERSION 3.15)

project(VocalWriterVoice VERSION 0.1.0 LANGUAGES C CXX)

# Static MSVC runtime keeps the plugin self-contained: no VC++ redistributable
# to install before Reaper can load it.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libs/JUCE" CACHE PATH "Path to JUCE")
set(VW_ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../engine" CACHE PATH
    "The VocalWriterC submodule: its include/ and src/ are compiled in")

if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE not found at ${JUCE_PATH}.\n"
        "Run: git clone --depth 1 --branch 8.0.13 "
        "https://github.com/juce-framework/JUCE.git \"${JUCE_PATH}\"")
endif()
if(NOT EXISTS "${VW_ENGINE_DIR}/include/vw_editor.h")
    message(FATAL_ERROR
        "The engine is not there. It is a submodule: run\n"
        "  git submodule update --init engine")
endif()

add_subdirectory(${JUCE_PATH} _juce)

# -- the engine ------------------------------------------------------------
#
# Compiled into the plugin from source rather than linked as the prebuilt
# libvocalwriter.dll, which is built by GCC under msys. Mixing that with an
# MSVC plugin means two C runtimes and a DLL to ship beside the .vst3, and
# buys nothing: an MSVC build of these sources was checked halfword for
# halfword against the GCC one over a rendered phrase and is identical, so
# compiling them here is the same synthesiser with one binary fewer.
#
# vw.c is the command line tool's main(), and is the one file left out.
file(GLOB VW_ENGINE_SOURCES "${VW_ENGINE_DIR}/src/*.c")
list(FILTER VW_ENGINE_SOURCES EXCLUDE REGEX "/vw\\.c$")

add_library(vwengine STATIC ${VW_ENGINE_SOURCES})
target_include_directories(vwengine PUBLIC "${VW_ENGINE_DIR}/include")
set_target_properties(vwengine PROPERTIES C_STANDARD 11 POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    # /fp:precise is the default, and is what keeps a*b+c two roundings the
    # way the PowerPC code did it -- the engine's -ffp-contract=off. The
    # warnings are the original's own style, not problems to fix here: this
    # code is transcribed from a 2005 binary and is not ours to tidy.
    target_compile_options(vwengine PRIVATE /fp:precise /wd4311 /wd4312 /wd4715
                                            /wd4244 /wd4267 /wd4018 /wd4101)
else()
    target_compile_options(vwengine PRIVATE
        -ffp-contract=off -fwrapv -fexcess-precision=standard -fno-fast-math
        -Wno-unused-variable -Wno-unused-parameter -Wno-sign-compare)
endif()

# -- the plugin ------------------------------------------------------------

juce_add_plugin(VocalWriterVoice
    COMPANY_NAME "VocalWriter"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE "VcWr"
    PLUGIN_CODE "VwVo"
    FORMATS VST3 Standalone
    PRODUCT_NAME "VocalWriter Voice")

target_sources(VocalWriterVoice PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/VoiceEngine.cpp
    Source/VoiceEngine.h
    Source/Part.cpp
    Source/Part.h)

target_compile_definitions(VocalWriterVoice
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_include_directories(VocalWriterVoice PRIVATE Source)

target_link_libraries(VocalWriterVoice
    PRIVATE
        vwengine
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

juce_generate_juce_header(VocalWriterVoice)

# -- the render test -------------------------------------------------------
#
# The engine is proved exact by the differential tests in the engine
# repository. What this proves is the layer this plugin puts around it: the
# phoneme packing, the note feeding, and which way round the output buffer is
# read -- none of which a compiler can check, and all of which still make a
# sound when they are wrong.
juce_add_console_app(rendertest PRODUCT_NAME "rendertest")

target_sources(rendertest PRIVATE
    Test/rendertest.cpp
    Source/VoiceEngine.cpp
    Source/VoiceEngine.h
    Source/Part.cpp
    Source/Part.h)

target_compile_definitions(rendertest
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1)

target_include_directories(rendertest PRIVATE Source)
target_link_libraries(rendertest
    PRIVATE vwengine juce::juce_audio_formats juce::juce_gui_basics
    PUBLIC juce::juce_recommended_config_flags)
