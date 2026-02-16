# ###########################################
# Math Library (xMath via FetchContent)
# ###########################################
IF(SEDX_USE_FETCHCONTENT_TRACY AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
	FETCHCONTENT_DECLARE(
		tracy
		GIT_REPOSITORY https://github.com/wolfpld/tracy.git
		GIT_TAG master
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
	)
	FETCHCONTENT_GETPROPERTIES(tracy)

	IF(NOT tracy_POPULATED)
		FETCHCONTENT_POPULATE(tracy)
		ADD_SUBDIRECTORY("${tracy_SOURCE_DIR}" "${tracy_BINARY_DIR}")
	ENDIF()
ENDIF()

# ###########################################
# Tracy Profiler Options
# ###########################################
OPTION(TRACY_ON_DEMAND "Enable Tracy on-demand profiling" ON)
OPTION(TRACY_NO_EXIT "Enable Tracy profiler even without exit" OFF)
OPTION(TRACY_NO_BROADCAST "Disable Tracy broadcast" OFF)

# Use a STRING cache entry for callstack depth instead of OPTION to allow numeric values
SET(SEDX_TRACY_CALLSTACK "48" CACHE STRING "Size of the collected call stacks for Tracy (numeric)")
OPTION(SEDX_ENABLE_TRACY_CPU_MEMORY "Enable CPU memory profiling with Tracy" ON)
OPTION(SEDX_ENABLE_TRACY_GPU "Enable GPU profiling with Tracy" ON)

# ###########################################
# Tracy Profiler Setup
# ###########################################
IF(TRACY_ENABLE)
	# If both mimalloc and Tracy CPU memory are requested, abort early
	IF(SEDX_ENABLE_TRACY_CPU_MEMORY AND USE_MIMALLOC)
		MESSAGE(FATAL_ERROR "Tracy cannot be used with mimalloc")
	ENDIF()

	# --------------------------------
	IF(NOT SEDX_TRACY_CALLSTACK OR SEDX_TRACY_CALLSTACK STREQUAL "OFF")
		SET(SEDX_TRACY_CALLSTACK "48")
	ENDIF()

	# --------------------------------
	ADD_COMPILE_DEFINITIONS(
		TRACY_ENABLE
		TRACY_CALLSTACK=${SEDX_TRACY_CALLSTACK}
		TRACY_DELAYED_INIT
		TRACY_MANUAL_LIFETIME
	)

	# --------------------------------
	IF(SEDX_ENABLE_TRACY_CPU_MEMORY)
		ADD_COMPILE_DEFINITIONS(SEDX_TRACY_CPU_MEMORY)
	ENDIF()

	# --------------------------------
	IF(SEDX_ENABLE_TRACY_GPU)
		ADD_COMPILE_DEFINITIONS(
			SEDX_TRACY_GPU
			TRACY_VK_USE_SYMBOL_TABLE
		)
	ENDIF()

	# --------------------------------
	IF(TRACY_ON_DEMAND)
		ADD_COMPILE_DEFINITIONS(TRACY_ON_DEMAND)
	ENDIF()

	# --------------------------------
	IF(TRACY_NO_EXIT)
		ADD_COMPILE_DEFINITIONS(TRACY_NO_EXIT)
	ENDIF()

	# --------------------------------
	IF(TRACY_NO_BROADCAST)
		ADD_COMPILE_DEFINITIONS(TRACY_NO_BROADCAST)
	ENDIF()

# --------------------------------
ENDIF()
