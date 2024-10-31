# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

project(mlx5ctl LANGUAGES C)
cmake_minimum_required(VERSION 3.18.1 FATAL_ERROR)

include(CheckCCompilerFlag)

function(CTL_AddOptCFlag TO_VAR CACHE_VAR FLAG)
  CHECK_C_COMPILER_FLAG("${FLAG}" ${CACHE_VAR})
  if (${CACHE_VAR})
    SET(${TO_VAR} "${${TO_VAR}} ${FLAG}" PARENT_SCOPE)
  endif()
endfunction()

CTL_AddOptCFlag(CMAKE_C_FLAGS HAVE_C_WARNINGS
  "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter")
CTL_AddOptCFlag(CMAKE_C_FLAGS HAVE_C_WMISSING_PROTOTYPES "-Wmissing-prototypes")
CTL_AddOptCFlag(CMAKE_C_FLAGS HAVE_C_WMISSING_DECLARATIONS "-Wmissing-declarations")
CTL_AddOptCFlag(CMAKE_C_FLAGS HAVE_C_WWRITE_STRINGS "-Wwrite-strings")
CTL_AddOptCFlag(CMAKE_C_FLAGS HAVE_C_WFORMAT_2 "-Wformat=2")
CTL_AddOptCFlag(CMAKE_C_FLAGS HAVE_C_WCAST_FUNCTION "-Wcast-function-type")
CTL_AddOptCFlag(CMAKE_C_FLAGS HAVE_C_WFORMAT_NONLITERAL "-Wformat-nonliteral")
CTL_AddOptCFlag(CMAKE_C_FLAGS HAVE_C_WDATE_TIME "-Wdate-time")
CTL_AddOptCFlag(CMAKE_C_FLAGS HAVE_C_WNESTED_EXTERNS "-Wnested-externs")

set (MLX5CTL_MODULES
  devcaps.c
  diag_cnt.c
  mlx5ctlu.c
  mlx5lib.c
  query_obj.c
  reg.c
  rscdump.c
)

set (MLX5CTL_MISC_IOCTL
  mlx5ctl_misc.c
)

add_executable(mlx5ctl
  ${MLX5CTL_MODULES}
  ${MLX5CTL_MISC_IOCTL}
)

# Alias target to make mlx5ctl the default
add_custom_target(default ALL DEPENDS mlx5ctl)

install(TARGETS mlx5ctl DESTINATION "${CMAKE_INSTALL_BINDIR}")
