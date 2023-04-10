// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef REFSI_G1_INFO_H
#define REFSI_G1_INFO_H

#include <riscv/device_info_get.h>
#include <riscv/info.h>

#include <vector>

namespace refsi_g1_wi {

struct RefSiG1Info : riscv::RiscvInfo {
  RefSiG1Info(mux_device_info_t mux_device_info,
              const riscv::hal_device_info_riscv_t *hal_device_info);

  std::unique_ptr<compiler::Target> createTarget(
      compiler::Context *context,
      compiler::NotifyCallbackFn callback) const override;

  static void get(compiler::AddCompilerFn add_compiler) {
    static std::vector<RefSiG1Info> infos;
    static std::once_flag compilers_initialized;
    std::call_once(compilers_initialized, [] {
      for (auto &device_info : riscv::GetDeviceInfosArray()) {
        auto *riscv_hal_device_info =
            static_cast<const riscv::hal_device_info_riscv_t *>(
                device_info.hal_device_info);
        infos.push_back(RefSiG1Info(&device_info, riscv_hal_device_info));
      }
    });
    for (const auto &info : infos) {
      add_compiler(&info);
    }
  }
};
}  // namespace refsi_g1_wi

#endif  // REFSI_G1_INFO_H