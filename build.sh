#!/bin/sh
clear
if [ ${#} = 0 ];then
     PLATFROM="x86"
else
    PLATFROM=$1
fi
echo "compile $PLATFROM"

export INSTALL_PATH=$(pwd)/3rdlib/$PLATFROM
mkdir -p ${INSTALL_PATH}
rm -rf build
mkdir build
tool_prefix=../${PLATFROM}.cmake
export PLATFORM="${PLATFROM}"
echo "config:  ${tool_prefix}---${PLATFORM}"


export AWTK_INCLUDE=$(pwd)/include/$PLATFROM/awtk/src
export FRAMEWORK_INCLUDE=$(pwd)/include/$PLATFROM
cmake -DCMAKE_TOOLCHAIN_FILE=${tool_prefix} -S src -B build -DPLATFORM=${PLATFORM} -DAWTK_INCLUDE=${AWTK_INCLUDE} -DFRAMEWORK_INCLUDE=${FRAMEWORK_INCLUDE}

cmake --build build

echo cmake --install build --prefix ..
cmake --install build --prefix ${INSTALL_PATH}

# 编译测试程序
echo "Compiling test program... ${INSTALL_PATH}/conner_gradient"
# gcc -g -o test_conner_gradient test_conner_gradient.c \
#     -L${INSTALL_PATH}/conner_gradient -lconner_gradient \
#     -L./lib/${PLATFORM} -lawtk \
#     -I${AWTK_INCLUDE} -I${AWTK_INCLUDE}/ext_widgets \
#     -I. \
#     -lm -ldl
