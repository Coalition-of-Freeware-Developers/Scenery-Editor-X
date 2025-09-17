# Grouping into solution folders (only if targets exist)
FUNCTION(SEDX_GROUP_TARGETS)
    IF(TARGET CrashHandler)
        SET_PROPERTY(TARGET CrashHandler PROPERTY FOLDER "Tools")
    ENDIF()

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

    FOREACH(T IN ITEMS xMath imgui json-cpp-gen nlohmann_json)
        IF(TARGET ${T})
            SET_PROPERTY(TARGET ${T} PROPERTY FOLDER "Dependency")
        ENDIF()
    ENDFOREACH()

	FOREACH(T IN ITEMS glfw uninstall update_mappings docs)
        IF(TARGET ${T})
            SET_PROPERTY(TARGET ${T} PROPERTY FOLDER "Dependency/GLFW")
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

############################################
# Output directories (base paths)
############################################
# Use base dirs without ${CMAKE_CFG_INTDIR}; multi-config generators will
# append the config name automatically unless a config-specific property is set.
IF(NOT DEFINED BIN_DIR)
    SET(BIN_DIR ${CMAKE_SOURCE_DIR}/bin)
ENDIF()
IF(NOT DEFINED LIBS_DIR)
    SET(LIBS_DIR ${CMAKE_SOURCE_DIR}/lib)
ENDIF()

# Apply common output directories (guarded)
FUNCTION(SEDX_APPLY_OUTPUT_DIRS LIBS_DIR BIN_DIR)
    SET(CANDIDATES
        Launcher SceneryEditorX AppCore CrashHandler
        MathTests MemoryAllocatorTests ConversionTests RefTests SettingsTest EdxTests EdxDemoGenerator
        nlohmann_json json-cpp-gen imgui xMath libconfig libconfig++ edX XPSceneryLib glfw
        Catch2 Catch2WithMain
    )
    FOREACH(t ${CANDIDATES})
        IF(TARGET ${t})
            SET_TARGET_PROPERTIES(${t} PROPERTIES
                # Base (used by single-config generators)
                ARCHIVE_OUTPUT_DIRECTORY ${LIBS_DIR}
                LIBRARY_OUTPUT_DIRECTORY ${LIBS_DIR}
                RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}

                # Explicit per-config (avoids accidental Debug/Debug nesting)
                ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${LIBS_DIR}/Debug
                ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${LIBS_DIR}/Release
                LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${LIBS_DIR}/Debug
                LIBRARY_OUTPUT_DIRECTORY_RELEASE ${LIBS_DIR}/Release
                RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${BIN_DIR}/Debug
                RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR}/Release
            )
        ENDIF()
    ENDFOREACH()
ENDFUNCTION()

# VS debugger working directory
FUNCTION(SEDX_SET_DEBUGGER_WD target)
    IF(TARGET ${target})
        SET_TARGET_PROPERTIES(${target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")
    ENDIF()
ENDFUNCTION()
