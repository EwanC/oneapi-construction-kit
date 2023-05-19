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

__kernel void shuffle_copy(__global int *source, __global int *dest) {
  if (get_global_id(0) == 0) return;

  int3 tmp;

  tmp = (int3)(10);
  tmp.s2 = source[0];
  vstore3(tmp, 0, dest);

  tmp = (int3)(11);
  tmp.s0 = source[1];
  vstore3(tmp, 1, dest);

  tmp = (int3)(12);
  tmp.s1 = source[2];
  vstore3(tmp, 2, dest);
}
