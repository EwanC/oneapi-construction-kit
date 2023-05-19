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

// REQUIRES: half; parameters;
#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#ifndef TYPE
#define TYPE half
#endif

__kernel void half_async_strided_copy(__local TYPE *tmpA, __local TYPE *tmpB,
                                      __local TYPE *tmpC, __global TYPE *A,
                                      __global TYPE *B, __global TYPE *C) {
  size_t lid = get_local_id(0);
  size_t group = get_group_id(0);
  size_t size = get_local_size(0);

  // Copy the input data into local buffers.  The copies are asynchronous, but
  // we immediately wait for the copies to complete.
  event_t events[2];
  events[0] = async_work_group_strided_copy(tmpA, &A[group * size * 2], size, 2, 0);
  events[1] = async_work_group_strided_copy(tmpB, &B[group * size * 2], size, 2, 0);
  wait_group_events(2, events);

  // Calculate the result for this work item, and then wait for all work items
  // to do this so that all work items enter the async copy with the same
  // state.
  tmpC[lid] = fmax(tmpA[lid], tmpB[lid]);
  barrier(CLK_LOCAL_MEM_FENCE);

  // Copy the result out, again this is asynchronous, but we immediately wait
  // for the result.
  event_t event =
      async_work_group_strided_copy(&C[group * size], tmpC, size, 1, 0);
  wait_group_events(1, &event);
}
