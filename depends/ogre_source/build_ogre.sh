rm -rf build_ogre
mkdir build_ogre
export SDL2_PATH=$(pwd)/SDL2-2.30.8
echo "SDL2_PATH= ${SDL2_PATH}"
cp -fv download/* build_ogre

tar -zxf build_ogre/freetype-2.13.2.tar.gz -C build_ogre/
tar -zxf build_ogre/SDL2-2.30.8.tar.gz -C build_ogre/
tar -zxf build_ogre/imgui.tar.gz -C build_ogre/
tar -zxf build_ogre/pugixml-1.14.tar.gz -C build_ogre/


cmake -DCMAKE_TOOLCHAIN_FILE=../t5.cmake -S ogre-14.2.3 -B build_ogre \
-DCMAKE_BUILD_TYPE=Debug \
-DOGRE_BUILD_SAMPLES=OFF \
-DOGRE_BUILD_PLUGIN_ASSIMP=ON \
-DOGRE_BUILD_PLUGIN_DOT_SCENE=ON \
-DOGRE_BUILD_LIBS_AS_FRAMEWORKS=OFF \
-DOGRE_BUILD_COMPONENT_CSHARP=OFF \
-DOGRE_BUILD_COMPONENT_JAVA=OFF \
-DOGRE_BUILD_COMPONENT_PYTHON=OFF \
-DOGRE_BUILD_COMPONENT_TERRAIN=OFF \
-DOGRE_BUILD_PLUGIN_CG=OFF \
-DOGRE_BUILD_PLUGIN_PCZ=OFF \
-DOGRE_CONFIG_ENABLE_ASTC=OFF \
-DOGRE_CONFIG_ENABLE_ETC=OFF \
-DOGRE_CONFIG_ENABLE_PVRTC=OFF \
-DOGRE_BUILD_COMPONENT_BITES=OFF \
-DOGRE_BUILD_TOOLS=OFF \
-DSDL2_PATH=${SDL2_PATH} \
-DOGRE_CONFIG_ENABLE_DDS=OFF 

cmake --build build_ogre

echo "install "
mkdir install
cmake --install build_ogre --prefix install 
ls -la install



