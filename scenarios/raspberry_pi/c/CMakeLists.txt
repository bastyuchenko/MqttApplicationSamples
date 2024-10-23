# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(CMAKE_CACHEFILE_DIR ${CMAKE_CURRENT_LIST_DIR}/build)

# raspberry_pi
add_executable (raspberry_pi_client
  ${MOSQUITTO_CLIENT_EXTENSIONS}
  ${CMAKE_CURRENT_LIST_DIR}/raspberry_pi_client/main.cpp
)

# server_client
add_executable (server_client
  ${MOSQUITTO_CLIENT_EXTENSIONS}
  ${CMAKE_CURRENT_LIST_DIR}/server_client/main.c
)
