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

// REQUIRES: half
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

__kernel void half_atan2_zeros(__global half* x,
                               __global half* y,
                               __global ushort* out_atan2,
                               __global ushort* out_atan2pi) {
  size_t id = get_global_id(0);
  half result = atan2(x[id], y[id]);
  out_atan2[id] = as_ushort(result);

  result = atan2pi(x[id], y[id]);
  out_atan2pi[id] = as_ushort(result);
}
