MESSAGE(STATUS "Configuring project assets")

# --------------------------------
# DIRECTORIES
# --------------------------------
SET(ASSET_DIR   "${CMAKE_SOURCE_DIR}/resources")
SET(FONT_DIR    "${CMAKE_SOURCE_DIR}/resources/fonts")
SET(MODEL_DIR   "${CMAKE_SOURCE_DIR}/resources/models")
SET(SHADER_DIR  "${CMAKE_SOURCE_DIR}/resources/shaders")
SET(TEXTURE_DIR "${CMAKE_SOURCE_DIR}/resources/textures")

SET(FONT_FILES
    ${FONT_DIR}/industry/Industry-Bold.otf
    ${FONT_DIR}/industry/Industry-Book.otf
    ${FONT_DIR}/industry/Industry-Medium.otf
    ${FONT_DIR}/industry/Industry-Thin.otf
    ${FONT_DIR}/opensans/OpenSans-Bold.ttf
    ${FONT_DIR}/opensans/OpenSans-BoldItalic.ttf
    ${FONT_DIR}/opensans/OpenSans-ExtraBold.ttf
    ${FONT_DIR}/opensans/OpenSans-ExtraBoldItalic.ttf
    ${FONT_DIR}/opensans/OpenSans-Italic.ttf
    ${FONT_DIR}/opensans/OpenSans-Light.ttf
    ${FONT_DIR}/opensans/OpenSans-LightItalic.ttf
    ${FONT_DIR}/opensans/OpenSans-Regular.ttf
    ${FONT_DIR}/opensans/OpenSans-SemiBold.ttf
    ${FONT_DIR}/opensans/OpenSans-SemiBoldItalic.ttf
)

SET(REGISTRY_KEYS
    ${CMAKE_SOURCE_DIR}/source/Launcher/Registry/edXLibReg.rgs
    ${CMAKE_SOURCE_DIR}/source/Launcher/Registry/edXReg.rgs
    ${CMAKE_SOURCE_DIR}/source/Launcher/Registry/ProgramReg.rgs
)

SET(ICON_FILES
    ${ASSET_DIR}/icon.ico
    ${ASSET_DIR}/icon.png
    ${ASSET_DIR}/icon_edX.ico
    ${ASSET_DIR}/icon_testing.ico
    ${ASSET_DIR}/icon_testing.png
)
