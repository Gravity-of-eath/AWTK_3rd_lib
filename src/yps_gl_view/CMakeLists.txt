# Copyright (c) 2025 - 2025
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# License file for more details.

# 检查是否启用YPS_GL_VIEW控件
if(NOT DEFINED WITH_YPS_GL_VIEW)
  set(WITH_YPS_GL_VIEW ON)
endif()

if(${WITH_YPS_GL_VIEW})
  project(yps_gl_view)
  cmake_minimum_required(VERSION 3.10)

  # 搜索源文件
  aux_source_directory(./src SRC_FILES)
  aux_source_directory(./include INC_FILES)

  # 添加到父项目
  if(NOT ${MATERIAL_DESIGN})
    list(APPEND ${ROOT_PROJECT}_PRIVATE_SRC ${SRC_FILES})
    list(APPEND ${ROOT_PROJECT}_PRIVATE_INC ./include)
  endif()

endif()