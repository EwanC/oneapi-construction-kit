// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

__kernel void uniform_alloca(__global int *in, __global int *out) {
  size_t gid = get_global_id(0);
  if (gid == 0) {
    out[0] = ((__global int2 *)in)[0].x;
    out[1] = ((__global int2 *)in)[0].y;
  } else {
    out[gid * 2] = 11;
    out[gid * 2 + 1] = 13;
  }
}
