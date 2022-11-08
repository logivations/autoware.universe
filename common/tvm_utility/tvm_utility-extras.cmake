# Copyright 2021-2022 Arm Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Get user-provided variables
set(DOWNLOAD_ARTIFACTS OFF CACHE BOOL "enable artifacts download")
set(MODELZOO_VERSION "1.3.0-20220902" CACHE STRING "targeted ModelZoo version")

#
# Download the selected neural network if it is not already present on disk.
# Make inference_engine_tvm_config.hpp available under "data/models/${MODEL_NAME}/".
# Install the TVM artifacts to "share/${PROJECT_NAME}/models/".
# Return the name of the custom target in the DEPENDENCY parameter.
#
# :param MODEL_NAME: the name of the targeted neural network
# :type MODEL_NAME: string
# :param MODEL_BACKEND: the name of the targeted backend
# :type MODEL_BACKEND: string
# :param DEPENDENCY: output parameter; name of the ExternalProject top level target
# :type DEPENDENCY: string
#
function(get_neural_network MODEL_NAME MODEL_BACKEND DEPENDENCY)
  set(DATA_PATH ${CMAKE_CURRENT_SOURCE_DIR}/data)
  set(EXTERNALPROJECT_NAME ${MODEL_NAME}_${MODEL_BACKEND})

  # Prioritize user-provided models.
  if(IS_DIRECTORY "${DATA_PATH}/user/${MODEL_NAME}")
    message(STATUS "Using user-provided model from ${DATA_PATH}/user/${MODEL_NAME}")
    file(REMOVE_RECURSE "${DATA_PATH}/models/${MODEL_NAME}/")
    configure_file(
      "${DATA_PATH}/user/${MODEL_NAME}/inference_engine_tvm_config.hpp"
      "${DATA_PATH}/models/${MODEL_NAME}/inference_engine_tvm_config.hpp"
      COPYONLY
    )
    install(
      DIRECTORY "${DATA_PATH}/user/${MODEL_NAME}"
      DESTINATION "share/${PROJECT_NAME}/models/"
      USE_SOURCE_PERMISSIONS
    )
  else()
    set(ARCHIVE_NAME "${MODEL_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${MODEL_BACKEND}-${MODELZOO_VERSION}.tar.gz")

    # Use previously-downloaded archives if available.
    if(EXISTS "${DATA_PATH}/downloads/${ARCHIVE_NAME}")
      set(URL "${DATA_PATH}/downloads/${ARCHIVE_NAME}")
    elseif(DOWNLOAD_ARTIFACTS)
      message(STATUS "Downloading ${ARCHIVE_NAME} ...")
      set(URL "https://autoware-modelzoo.s3.us-east-2.amazonaws.com/models/${MODELZOO_VERSION}/${ARCHIVE_NAME}")
    else()
      message(WARNING "Skipped download for ${MODEL_NAME} (enable by setting DOWNLOAD_ARTIFACTS)")
      set(${DEPENDENCY} "" PARENT_SCOPE)
      return()
    endif()

    include(ExternalProject)
    externalproject_add(${EXTERNALPROJECT_NAME}
      DOWNLOAD_DIR "${DATA_PATH}/downloads"
      SOURCE_DIR "${DATA_PATH}/models/${MODEL_NAME}"
      URL ${URL}
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      BUILD_BYPRODUCTS "${DATA_PATH}/models/${MODEL_NAME}/inference_engine_tvm_config.hpp"
      INSTALL_COMMAND ""
    )
    install(
      DIRECTORY "${DATA_PATH}/models/${MODEL_NAME}"
      DESTINATION "share/${PROJECT_NAME}/models/"
      USE_SOURCE_PERMISSIONS
    )
  endif()

  set(${DEPENDENCY} ${EXTERNALPROJECT_NAME} PARENT_SCOPE)

endfunction()
