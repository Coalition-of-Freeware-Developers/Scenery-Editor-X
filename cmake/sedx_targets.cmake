# Grouping into solution folders (only if targets exist)
FUNCTION(SEDX_GROUP_TARGETS)
	FOREACH(T IN ITEMS CrashHandler TracyClient)
		IF(TARGET ${T})
			SET_PROPERTY(TARGET ${T} PROPERTY FOLDER "Tools")
		ENDIF()
	ENDFOREACH()

    FOREACH(T IN ITEMS MemoryAllocatorTests MathTests RefTests SettingsTest ConversionTests EdxTests EdxDemoGenerator)
        IF(TARGET ${T})
            SET_PROPERTY(TARGET ${T} PROPERTY FOLDER "Tests")
        ENDIF()
    ENDFOREACH()

    FOREACH(T IN ITEMS edX XPSceneryLib)
        IF(TARGET ${T})
            SET_PROPERTY(TARGET ${T} PROPERTY FOLDER "File Formats")
        ENDIF()
    ENDFOREACH()

	FOREACH(T IN ITEMS xMath uninstall imgui ktx json-cpp-gen nlohmann_json)
        IF(TARGET ${T})
            SET_PROPERTY(TARGET ${T} PROPERTY FOLDER "Dependency")
        ENDIF()
    ENDFOREACH()

	FOREACH(T IN ITEMS SDL3 SDL3-static SDL3_test SDL3-shared SDL_uclibc)
        IF(TARGET ${T})
            SET_PROPERTY(TARGET ${T} PROPERTY FOLDER "Dependency/SDL3")
        ENDIF()
    ENDFOREACH()

    FOREACH(T IN ITEMS libconfig libconfig++)
        IF(TARGET ${T})
            SET_PROPERTY(TARGET ${T} PROPERTY FOLDER "Dependency/LibConfig")
        ENDIF()
    ENDFOREACH()

    FOREACH(T IN ITEMS Catch2 Catch2WithMain)
        IF(TARGET ${T})
            SET_PROPERTY(TARGET ${T} PROPERTY FOLDER "Dependency/Catch2")
        ENDIF()
    ENDFOREACH()
ENDFUNCTION()

# VS debugger working directory
FUNCTION(SEDX_SET_DEBUGGER_WD target)
    IF(TARGET ${target})
        SET_TARGET_PROPERTIES(${target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")
    ENDIF()
ENDFUNCTION()

# --------------------------------
# Output Directory Configuration
# --------------------------------
SET(SEDX_OUTPUT_TARGETS

	# Main project targets
	Launcher SceneryEditorX AppCore CrashHandler edX XPSceneryLib

	# Project Tests
	MathTests MemoryAllocatorTests ConversionTests RefTests SettingsTest EdxTests EdxDemoGenerator

	# Libraries
	nlohmann_json json-cpp-gen imgui xMath libconfig libconfig++ ktx SDL3_test SDL3 SDL3 SDL3-static
	Catch2 Catch2WithMain uninstall $<$<AND:$<CONFIG:Debug>,$<BOOL:${TRACY_ENABLE}>>:TracyClient>
)

FOREACH(TARGET IN ITEMS ${SEDX_OUTPUT_TARGETS})
	IF(TARGET ${TARGET})
		SET_TARGET_PROPERTIES(${TARGET} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY ${LIBS_DIR}
			LIBRARY_OUTPUT_DIRECTORY ${LIBS_DIR}
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
		)

		# Apply PDB configuration to each target
		SEDX_CONFIGURE_PDB_OUTPUT(${TARGET})
	ENDIF()
ENDFOREACH()
