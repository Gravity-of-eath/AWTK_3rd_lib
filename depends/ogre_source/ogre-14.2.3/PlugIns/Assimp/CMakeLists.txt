file(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
list(APPEND HEADER_FILES ${PROJECT_BINARY_DIR}/include/OgreAssimpExports.h)

include_directories(Codec_Assimp  /mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/ScreenCopy/awtk-ogre/include)
add_library(Codec_Assimp ${OGRE_LIB_TYPE} ${HEADER_FILES} src/AssimpLoader.cpp)
target_include_directories(Codec_Assimp PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    $<INSTALL_INTERFACE:include/OGRE/Plugins/Assimp>  )
target_link_directories(Codec_Assimp PUBLIC "/mnt/a57a7843-7ae2-415a-9125-4c61fa4163d9/ScreenCopy/awtk-ogre/3rd/build_assimp/bin")

target_link_libraries(Codec_Assimp PUBLIC OgreMain  assimp)

if(OGRE_BUILD_COMPONENT_RTSHADERSYSTEM)
    target_link_libraries(Codec_Assimp PUBLIC OgreRTShaderSystem)
endif()

ogre_config_framework(Codec_Assimp)
ogre_config_plugin(Codec_Assimp)
generate_export_header(Codec_Assimp
    EXPORT_MACRO_NAME _OgreAssimpExport
    EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/OgreAssimpExports.h)
install(FILES ${HEADER_FILES} DESTINATION include/OGRE/Plugins/Assimp)
