# SEDX Reference Probe: X-Plane 12 plugin for the parity harness (plan TASK-001).
#
# Built from the main tree with -DSEDX_BUILD_XP_PROBE=ON, or on its own:
#   cmake -S tools/xp12_reference_probe -B build_probe -DXPLANE_SDK_DIR=<XPSDK folder>
#   cmake --build build_probe --config Release
# The SDK (XPLM 4.x, https://developer.x-plane.com/sdk/plugin-sdk-downloads/) is found through XPLANE_SDK_DIR or the
# XPLANE_12_SDK environment variable. The plugin lands in <build>/SEDXReferenceProbe/64/{win,lin,mac}.xpl next to
# scenarios.json and art_controls.txt: copy the SEDXReferenceProbe folder to X-Plane 12/Resources/plugins/.

cmake_minimum_required(VERSION 3.24)
if(NOT DEFINED PROJECT_NAME)
	project(SEDXReferenceProbe LANGUAGES CXX)
endif()

set(XPLANE_SDK_DIR "$ENV{XPLANE_12_SDK}" CACHE PATH "X-Plane SDK folder (holding CHeaders/ and Libraries/)")
find_path(XPLM_INCLUDE_DIR XPLMPlugin.h PATHS "${XPLANE_SDK_DIR}/CHeaders/XPLM" NO_DEFAULT_PATH)
if(NOT XPLM_INCLUDE_DIR)
	message(FATAL_ERROR "SEDXReferenceProbe: no X-Plane SDK at '${XPLANE_SDK_DIR}'. Set XPLANE_12_SDK or XPLANE_SDK_DIR to the unpacked XPSDK folder.")
endif()

set(SEDX_PROBE_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(SEDX_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(SEDX_PROBE_OUT "${CMAKE_BINARY_DIR}/SEDXReferenceProbe")

add_library(SEDXReferenceProbe MODULE "${SEDX_PROBE_ROOT}/src/sedx_reference_probe.cpp")
target_compile_features(SEDXReferenceProbe PRIVATE cxx_std_20)
target_include_directories(SEDXReferenceProbe PRIVATE
	"${XPLM_INCLUDE_DIR}"
	"${SEDX_REPO_ROOT}/dependency/json/include")
target_compile_definitions(SEDXReferenceProbe PRIVATE XPLM200=1 XPLM210=1 XPLM300=1 XPLM301=1 XPLM303=1 XPLM400=1)

if(WIN32)
	target_compile_definitions(SEDXReferenceProbe PRIVATE IBM=1 NOMINMAX WIN32_LEAN_AND_MEAN)
	target_link_libraries(SEDXReferenceProbe PRIVATE "${XPLANE_SDK_DIR}/Libraries/Win/XPLM_64.lib")
	set(SEDX_PROBE_FILE win)
elseif(APPLE)
	target_compile_definitions(SEDXReferenceProbe PRIVATE APL=1)
	target_link_options(SEDXReferenceProbe PRIVATE "-F${XPLANE_SDK_DIR}/Libraries/Mac" "-framework" "XPLM")
	set(SEDX_PROBE_FILE mac)
else()
	# Linux: XPLM symbols resolve against X-Plane when the plugin loads
	target_compile_definitions(SEDXReferenceProbe PRIVATE LIN=1)
	set_target_properties(SEDXReferenceProbe PROPERTIES CXX_VISIBILITY_PRESET hidden)
	set(SEDX_PROBE_FILE lin)
endif()

# <out>/SEDXReferenceProbe/64/<platform>.xpl, the layout X-Plane loads fat plugins from (no per-config subfolder: the
# generator expression keeps multi-config generators from adding one)
set_target_properties(SEDXReferenceProbe PROPERTIES
	PREFIX ""
	SUFFIX ".xpl"
	OUTPUT_NAME "${SEDX_PROBE_FILE}"
	LIBRARY_OUTPUT_DIRECTORY "$<1:${SEDX_PROBE_OUT}/64>"
	RUNTIME_OUTPUT_DIRECTORY "$<1:${SEDX_PROBE_OUT}/64>")

# The scenario sets (the KSEA parity set and the test airports) and the art-control names travel with the plugin
add_custom_command(TARGET SEDXReferenceProbe POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SEDX_PROBE_ROOT}/scenarios/xp12_parity.json" "${SEDX_PROBE_OUT}/scenarios.json"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SEDX_PROBE_ROOT}/scenarios/xp12_airports.json" "${SEDX_PROBE_OUT}/airports.json"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SEDX_REPO_ROOT}/docs/xplane/xp12_art_controls.txt" "${SEDX_PROBE_OUT}/art_controls.txt"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SEDX_PROBE_ROOT}/README.md" "${SEDX_PROBE_OUT}/README.md")
