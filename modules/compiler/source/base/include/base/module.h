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

/// @file
///
/// @brief Compiler program module API.

#ifndef BASE_MODULE_H_INCLUDED
#define BASE_MODULE_H_INCLUDED

#include <base/context.h>
#include <base/target.h>
#include <builtins/printf.h>
#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/optional.h>
#include <cargo/small_vector.h>
#include <cargo/string_view.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <compiler/kernel.h>
#include <compiler/module.h>
#include <compiler/utils/pass_machinery.h>
#include <llvm/IR/PassManager.h>
#include <multi_llvm/optional_helper.h>
#include <mux/mux.hpp>

#include <mutex>

namespace compiler {

namespace utils {
class PassMachinery;
}

/// @addtogroup compiler
/// @{

/// @brief A class that drives the compilation process and stores the compiled
/// binary.
class BaseModule : public Module {
 public:
  enum class MacroDefType { Def, Undef };

  BaseModule(compiler::BaseTarget &target, compiler::BaseContext &context,
             uint32_t &num_errors, std::string &log);
  ~BaseModule();

  BaseModule(const BaseModule &) = delete;
  BaseModule &operator=(const BaseModule &) = delete;

  /// @brief Clear out the stored data.
  void clear() override;

  /// @brief Get a reference to the compiler options that will be used by this
  /// module.
  ///
  /// @return A mutable reference to the compiler options.
  Options &getOptions() override final;

  /// @brief Get a reference to the compiler options that will be used by this
  /// module.
  ///
  /// @return A const reference to the compiler options.
  const Options &getOptions() const override final;

  /// @brief Populate `options` from a given string.
  ///
  /// @param[in] input_options The string of options to parse.
  /// @param[in] mode Determines the OpenCL error code to return in case options
  /// are invalid
  ///
  /// @return Returns a status code.
  /// @retval `Result::SUCCESS` when compilation was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_BUILD_OPTIONS` when invalid options were set and
  /// `mode` is `compiler::Options::Mode::BUILD`.
  /// @retval `Result::INVALID_COMPILER_OPTIONS` when invalid options were set
  /// and `mode` is `compiler::Options::Mode::COMPILE`.
  /// @retval `Result::INVALID_LINK_OPTIONS` when invalid options were set and
  /// `mode` is `compiler::Options::Mode::LINK`.
  Result parseOptions(cargo::string_view input_options,
                      compiler::Options::Mode mode) override final;

  /// @brief Loads a SPIR program.
  ///
  /// @param[in] buffer Serialized SPIR binary to parse.
  ///
  /// @return True if loading the SPIR module was successful, false otherwise.
  bool loadSPIR(cargo::array_view<const std::uint8_t> buffer) override;

  /// @brief Compiles a previously loaded SPIR program.
  ///
  /// @param[out] output_options The compilation options parsed from SPIR
  /// metadata will be output here.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when compilation was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_COMPILER_OPTIONS` when invalid options were set.
  /// @retval `Result::COMPILE_PROGRAM_FAILURE` if `compileSPIR` was called
  /// before `loadSPIR`.
  Result compileSPIR(std::string &output_options) override;

  /// @brief Compiles a SPIR-V program.
  ///
  /// @param[in] buffer View of the SPIR-V binary stream memory.
  /// @param[in] spirv_device_info Target device information.
  /// @param[in] spirv_spec_info Information about constants to be specialized.
  ///
  /// @return Returns either a SPIR-V module info object on success, or a status
  /// code otherwise.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_COMPILER_OPTIONS` when invalid options were set.
  /// @retval `Result::BUILD_PROGRAM_FAILURE` if compilation failed and `mode`
  /// is `compiler::Options::Mode::BUILD`.
  /// @retval `Result::COMPILE_PROGRAM_FAILURE` if compilation failed and `mode`
  /// is `compiler::Options::Mode::COMPILE`.
  cargo::expected<spirv::ModuleInfo, Result> compileSPIRV(
      cargo::array_view<const std::uint32_t> buffer,
      const spirv::DeviceInfo &spirv_device_info,
      cargo::optional<const spirv::SpecializationInfo &> spirv_spec_info)
      override;

  /// @brief Compile an OpenCL C program.
  ///
  /// @param[in] device_profile Device profile string. Should be either
  /// FULL_PROFILE or EMBEDDED_PROFILE.
  /// @param[in] source OpenCL C source code string.
  /// @param[in] input_headers List of headers to be included.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when compilation was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_COMPILER_OPTIONS` when invalid options were set.
  /// @retval `Result::COMPILE_PROGRAM_FAILURE` when compilation failed.
  Result compileOpenCLC(
      cargo::string_view device_profile, cargo::string_view source,
      cargo::array_view<compiler::InputHeader> input_headers) override;

  /// @brief Link a set of program binaries together into the current program.
  ///
  /// @param[in] input_modules List of input modules to link.
  ///
  /// @return Returns a status code.
  /// @retval `Result::SUCCESS` when linking was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_LINKER_OPTIONS` when invalid options were set.
  /// @retval `Result::LINK_PROGRAM_FAILURE` when linking failed.
  Result link(cargo::array_view<Module *> input_modules) override;

  /// @brief Generates a binary from the current program.
  ///
  /// @param[out] kernel_info_callback Kernel info callback.
  /// @param[out] printf_calls Output printf descriptor list.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when finalization was successful.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::FINALIZE_PROGRAM_FAILURE` when finalization failed. See
  /// the error log for more information.
  Result finalize(
      KernelInfoCallback kernel_info_callback,
      std::vector<builtins::printf::descriptor> &printf_calls) override;

  /// @brief Returns an object that represents a kernel contained within this
  /// module.
  ///
  /// @param[in] name The name of the kernel.
  ///
  /// @return An object that represents a kernel contained within this module.
  /// The lifetime of the `Kernel` object will be managed by `Module`.
  Kernel *getKernel(const std::string &name) override;

  /// @brief Compute the size of the serialized module.
  ///
  /// @return The size of the module (in bytes).
  std::size_t size() override;

  /// @brief Serialize the module.
  ///
  /// @param[in] output_buffer The buffer to write the serialized LLVM module
  /// to. The buffer must be at least size() bytes.
  ///
  /// @return The number of bytes written to the output buffer (in bytes).
  std::size_t serialize(std::uint8_t *output_buffer) override;

  /// @brief Deserialize a serialized module.
  ///
  /// @param[in] buffer Serialized module to parse.
  ///
  /// @return A boolean indicating whether deserialization was successful.
  bool deserialize(cargo::array_view<const std::uint8_t> buffer) override;

  /// @brief Enables a snapshot callback to be triggered when a compilation
  /// stage is reached.
  ///
  /// @param[in] stage Snapshot stage to trigger.
  /// @param[in] callback Snapshot callback.
  /// @param[in] user_data Snapshot callback user data.
  /// @param[in] format Snapshot format to use. Defaults to
  /// `SnapshotFormat::DEFAULT`.
  ///
  /// @return Return a status code.
  /// @retval `Result::SUCCESS` when snapshot successfully set.
  /// @retval `Result::OUT_OF_MEMORY` if an allocation failed.
  /// @retval `Result::INVALID_VALUE` if an invalid argument was passed.
  Result setSnapshotCallback(
      const char *stage, compiler_snapshot_callback_t callback, void *user_data,
      SnapshotFormat format = SnapshotFormat::DEFAULT) override final;

  /// @brief Returns the current state of the compiler module.
  ModuleState getState() const override final { return state; }

  /// @brief Return a new pass machinery to be used for the compilation pipeline
  virtual std::unique_ptr<compiler::utils::PassMachinery> createPassMachinery();

  /// @brief Initialize a pass machinery for running in BaseModule's frontend
  /// pipelines.
  virtual void initializePassMachineryForFrontend(
      compiler::utils::PassMachinery &, const clang::CodeGenOptions &) const;

  /// @brief Initialize a pass machinery for running in BaseModule::finalize.
  virtual void initializePassMachineryForFinalize(
      compiler::utils::PassMachinery &) const;

  /// @brief Custom diagnostic handler which intercepts ComputeMux diagnostics
  /// and logs them in the build log.
  ///
  /// If a diagnostic has an 'error' severity kind, BaseModule::num_errors is
  /// incremented.
  struct DiagnosticHandler : public llvm::DiagnosticHandler {
    /// @brief A function allowing filtering of diagnostic kinds.
    /// @return true if the diagnostic should be handled, false if it should be
    /// ignored.
    using DiagnosticFilterTy =
        std::function<bool(const llvm::DiagnosticInfo &DI)>;

    DiagnosticHandler(BaseModule &base_module, DiagnosticFilterTy filter_fn)
        : base_module(base_module), filter_fn(filter_fn) {}

    bool handleDiagnostics(const llvm::DiagnosticInfo &DI) override;

    BaseModule &base_module;
    DiagnosticFilterTy filter_fn;
  };

  /// @brief Allows temporary installation of a BaseModule diagnostic handler,
  /// restoring the old one on scope exit.
  struct ScopedDiagnosticHandler {
    ScopedDiagnosticHandler(
        BaseModule &base_module,
        DiagnosticHandler::DiagnosticFilterTy filter_fn = nullptr)
        : base_module(base_module),
          old_handler(base_module.context.llvm_context.getDiagnosticHandler()) {
      base_module.context.llvm_context.setDiagnosticHandler(
          std::make_unique<DiagnosticHandler>(base_module, filter_fn));
    }
    ~ScopedDiagnosticHandler() {
      // Reinstate the old diagnostic handler
      base_module.context.llvm_context.setDiagnosticHandler(
          std::move(old_handler));
    }
    BaseModule &base_module;
    std::unique_ptr<llvm::DiagnosticHandler> old_handler;
  };

  /// @brief Helper class for compiler snapshots.
  struct SnapshotDetails {
    const char *stage;
    compiler_snapshot_callback_t callback;
    SnapshotFormat format;
    void *user_data;

    SnapshotDetails()
        : stage(nullptr),
          callback(nullptr),
          format(SnapshotFormat::DEFAULT),
          user_data(nullptr) {}
  };

  /// @brief Helper method to aid in identifying snapshots.
  /// @param stage Snapshot stage to query for
  /// @param snapshots Array of currently enabled snapshots
  static cargo::optional<SnapshotDetails> shouldTakeSnapshot(
      const char *stage, cargo::array_view<const SnapshotDetails> snapshots);

  /// @brief Helper method to aid in identifying target snapshots. The snapshot
  /// stage queried is computed by calling getTargetSnapshotName with the two
  /// stage name parameters.
  /// @param snapshot_target_name Target-configurable component of snapshot
  /// stage
  /// @param snapshot_stage_suffix Snapshot suffix
  /// @param snapshots Array of currently enabled snapshots
  static cargo::optional<SnapshotDetails> shouldTakeTargetSnapshot(
      const char *snapshot_target_name, const char *snapshot_stage_suffix,
      cargo::array_view<const SnapshotDetails> snapshots);

  /// @brief Helper method to compute a target snapshot stage name
  /// The queried snapshot name is computed as the formatted string:
  /// "cl_snapshot_{snapshot_target_name}_{snapshot_stage_suffix}"
  /// @param snapshot_target_name Target-configurable component of snapshot
  /// stage
  /// @param snapshot_stage_suffix Snapshot suffix
  static std::string getTargetSnapshotName(const char *snapshot_target_name,
                                           const char *snapshot_stage_suffix);

  /// @brief Takes a snapshot of the parameterised module and invokes the
  /// snapshot callback with that data. Can be used by targets
  /// to help take "standard" snapshots.
  ///
  /// @param snapshot Snapshot details.
  /// @param module LLVM module to take a snapshot of.
  static void takeSnapshot(const SnapshotDetails &snapshot,
                           const llvm::Module *module);

  /// @brief Adds a compiler::utils::SimpleCallbackPass pass that invokes
  /// BaseModule::takeSnaphot to the PassManager if the target snapshot is
  /// enabled.
  /// @param pm PassManager to add the snapshot pass to
  /// @param snapshot_target_name Target-configurable component of snapshot
  /// stage
  /// @param snapshot_stage_suffix Snapshot suffix
  /// @param snapshots Array of currently enabled snapshots
  static void addSnapshotPassIfEnabled(
      llvm::ModulePassManager &pm, const char *snapshot_target_name,
      const char *snapshot_stage_suffix,
      cargo::array_view<const SnapshotDetails> snapshots);

 protected:
  /// @brief Create a module pass manager populated with target-specific
  /// middle-end compiler passes.
  ///
  /// These passes are added to the very end of the pipeline created by
  /// `BaseModule::finalize`.
  ///
  /// @return LLVM module pass manager populated with target passes.
  virtual llvm::ModulePassManager getLateTargetPasses(
      compiler::utils::PassMachinery &) = 0;

  /// @brief Creates a compiler::Kernel object from the module.
  ///
  /// Called by compiler::BaseModule::getKernel in the case that there is no
  /// already existing cached kernel.
  ///
  /// @param[in] name The name of the kernel.
  ///
  /// @return An object that represents a kernel contained within this module.
  virtual Kernel *createKernel(const std::string &name) = 0;

  /// @brief Compares the parameterized compilation stage with the snapshot
  /// stage the user has specified in member variable snapshot_stage.
  ///
  /// @param stage Compilation stage to compare against.
  ///
  /// @return The snapshot details to take if the snapshot stages match, none
  /// otherwise.
  cargo::optional<SnapshotDetails> shouldTakeSnapshot(const char *stage) const;

  /// @brief Add a diagnostic message to the log.
  ///
  /// @param[in] message Message to add to the build log.
  void addDiagnostic(cargo::string_view message);

  /// @brief Add an error message to the log.
  ///
  /// @param[in] message Message to add to the build log.
  void addBuildError(cargo::string_view message);

  /// @brief A function that can be used as an LLVM fatal error handler
  ///
  /// It assumes that `user_data` is a valid pointer to a `BaseModule`, on
  /// which it calls `addBuildError`.
  ///
  /// It can be installed e.g.
  ///
  /// ```cpp
  /// llvm::install_fatal_error_handler(
  ///     llvmFatalErrorHandler, /* user_data */ this);
  /// ```
  static void llvmFatalErrorHandler(void *user_data, const char *reason,
                                    bool gen_crash_diag);

  /// @brief Add a macro definition to be added to the preprocessor options to
  /// be used by clang.
  ///
  /// @param[in] macro Macro definition to be added.
  inline void addMacroDef(const std::string &macro) {
    macro_defs.emplace_back(MacroDefType::Def, macro);
  }

  /// @brief A custom BaseModule diagnostic printer for logging front-end
  /// diagnostics.
  ///
  /// This wraps clang's built-in TextDiagnosticPrinter but forwards
  /// diagnostics on to the build log and the BaseTarget's notify callback
  /// function.
  ///
  /// It does so by owning the string stream and that the TextDiagnosticPrinter
  /// emits to, but clearing the backing string on each diagnostic. The string
  /// is then emitted into the build log and passed to the notify callback if
  /// set.
  class FrontendDiagnosticPrinter : public clang::TextDiagnosticPrinter {
   public:
    FrontendDiagnosticPrinter(BaseModule &base_module,
                              clang::DiagnosticOptions *diags)
        : clang::TextDiagnosticPrinter(TempOS, diags,
                                       /*OwnsOutputStream*/ false),
          base_module(base_module),
          TempOS(TempStr) {}

    void HandleDiagnostic(clang::DiagnosticsEngine::Level Level,
                          const clang::Diagnostic &Info) override;

    BaseModule &base_module;
    std::string TempStr;
    llvm::raw_string_ostream TempOS;
  };

  /// @brief Add a macro definition to be added to the preprocessor options to
  /// be used by clang.
  ///
  /// @param[in] macro Macro definition to be added as an undef.
  inline void addMacroUndef(const std::string &macro) {
    macro_defs.emplace_back(MacroDefType::Undef, macro);
  }

  /// @brief Add an OpenCL option to be added to the clang OpenCL options.
  ///
  /// @param[in] opt OpenCL option to be added to clang.
  inline void addOpenCLOpt(const std::string &opt) {
    opencl_opts.emplace_back(opt);
  }

  /// @brief Populate a clang codegen options object with options needed for
  /// compiling OpenCL code.
  ///
  /// @param[in] codeGenOpts Clang codegen options to be populated.
  void populateCodeGenOpts(clang::CodeGenOptions &codeGenOpts);

  /// @brief Add default preprocessor options to the module, to be passed to
  /// clang on compile. This function currently populates the list of macro
  /// defs and undefs, as well as the list of options to be passed to OpenCL.
  ///
  /// @param[in] device_profile Device profile string. Should be either
  /// FULL_PROFILE or EMBEDDED_PROFILE.
  void addDefaultOpenCLPreprocessorOpts(cargo::string_view device_profile);

  /// @brief Populate clang lang options with sensible defaults for OpenCL,
  /// based on the compiler::Options set on this module.
  ///
  /// @param[in] lang_opts Reference to clang lang options to be populated.
  void setDefaultOpenCLLangOpts(clang::LangOptions &lang_opts);

  /// @brief Set correct OpenCL version on a clang lang options object, as well
  /// as return the appropriate lang standard kind to be passed to calls to
  /// clang::CompilerInvocation::setLangDefaults.
  ///
  /// @param[in] lang_opts Reference to clang lang options to be modified.
  /// @return Clang language standard matching the OpenCL version in use.
  clang::LangStandard::Kind setClangOpenCLStandard(
      clang::LangOptions &lang_opts);

  /// @brief Populate clang preprocessor options with the macro directives
  /// specified in macro_defs.
  ///
  /// @param[in] instance Clang instance.
  void populatePPOpts(clang::CompilerInstance &instance);

  /// @brief Populate clang OpenCL options with the options specified in
  /// opencl_opts.
  ///
  /// @param[in] instance Clang instance.
  void populateOpenCLOpts(clang::CompilerInstance &instance);

  /// @brief Dump kernel source code with macro definitions into a unique file.
  ///
  /// @param[in] source Kernel source code.
  /// @param[in] definitions List of macro definitions.
  /// @return Name of unique file containing the source code output.
  std::string debugDumpKernelSource(llvm::StringRef source,
                                    llvm::ArrayRef<std::string> definitions);

  /// @brief Write OpenCL kernel source to disk and set appropriate clang
  /// codegen options for debugging info purposes.
  ///
  /// @param[in] source Kernel source code.
  /// @param[in] path Absolute path to output file.
  /// @param[in] codeGenOpts Clang codegen options to update.
  /// @return Filename of kernel on module. This may not match the absolute path
  /// passed in to this function.
  std::string printKernelSource(llvm::StringRef source, llvm::StringRef path,
                                clang::CodeGenOptions codeGenOpts);

  /// @brief Create a module pass manager populated with early passes required
  /// for OpenCL C compilation.
  ///
  /// @return LLVM module pass manager populated with early passes.
  llvm::ModulePassManager getEarlyOpenCLCPasses();

  /// @brief Create a module pass manager populated with early passes required
  /// for SPIR and SPIR-V compilation.
  ///
  /// @param[in] is_spirv True if SPIR-V-specific passes, false if SPIR 1.2.
  ///
  /// @return LLVM module pass manager populated with early passes.
  llvm::ModulePassManager getEarlySPIRPasses(bool is_spirv);

  /// @brief Set up a clang compiler instance with default settings required for
  /// OpenCL, including language options and SPIR target triple.
  ///
  /// @param[in] instance Clang compiler instance.
  /// @return Result of attempting to choose an appropriate SPIR target triple.
  Result setOpenCLInstanceDefaults(clang::CompilerInstance &instance);

  /// @brief Create a clang frontend input file ready for compilation from
  /// OpenCL source. The clang instance target is initialized and populated with
  /// OpenCL compilation options added by addOpenCLOpt, and input_headers are
  /// force-included in the resulting file.
  ///
  /// @param[in] instance Clang compiler instance.
  /// @param[in] source OpenCL source code.
  /// @param[in] kernel_file_name Kernel file name to be passed to clang and
  /// ultimately LLVM as the module name.
  /// @param[in] input_headers List of headers to be included.
  /// @return Clang frontend input file to be used for compilation.
  clang::FrontendInputFile prepareOpenCLInputFile(
      clang::CompilerInstance &instance, llvm::StringRef source,
      std::string kernel_file_name,
      cargo::array_view<compiler::InputHeader> input_headers);

  /// @brief Load the precompiled OpenCL builtins header into the specified
  /// clang compiler instance. The clang codegen action for the OpenCL source
  /// file must already have been prepared before calling this function.
  ///
  /// @param[in] instance Clang compiler instance.
  void loadBuiltinsPCH(clang::CompilerInstance &instance);

  /// @brief Execute a clang OpenCL code gen action.
  ///
  /// @param[in] instance Clang compiler instance for diagnostics.
  /// @param[in] action Clang code gen action to be executed.
  /// @return Result of attempting to execute the clang action.
  Result executeOpenCLAction(clang::CompilerInstance &instance,
                             clang::CodeGenAction &action);

  /// @brief Run this module through the OpenCL frontend pipeline, optionally
  /// running early and late LLVM passes as part of this pipeline. A late fast
  /// math pass is additionally run if the options.fast_math flag is set.
  ///
  /// @param[in] codeGenOpts Clang codegen options.
  /// @param[in] early_passes Module pass manager populated with early passes.
  /// @param[in] late_passes Module pass manager populated with late passes.
  void runOpenCLFrontendPipeline(
      const clang::CodeGenOptions &codeGenOpts,
      multi_llvm::Optional<llvm::ModulePassManager> early_passes =
          multi_llvm::None,
      multi_llvm::Optional<llvm::ModulePassManager> late_passes =
          multi_llvm::None);

  /// @brief LLVM module produced by the `Module::finalize` method.
  std::unique_ptr<llvm::Module> finalized_llvm_module;

  /// @brief Reference on the implementation of the `compiler::Target` class.
  compiler::BaseTarget &target;

  /// @brief Snapshot details for capturing state of module during compilation.
  std::vector<SnapshotDetails> snapshots;

  /// @brief Compiler options populated by `Module::parseOptions` and passed to
  /// LLVM.
  compiler::Options options;

  /// @brief Reference on the context this module belongs to.
  compiler::BaseContext &context;

  /// @brief Macro definitions to be added to clang preprocessor macro defs.
  std::vector<std::pair<MacroDefType, std::string>> macro_defs;

  /// @brief OpenCL options to be added to clang.
  std::vector<std::string> opencl_opts;

 private:
  /// @brief Check if the opencl.kernels metadata exists in the binary's module,
  /// and create them if they don't.
  ///
  /// If a module contains no functions, then the opencl.kernels metadata will
  /// not exist. However, many parts of CA use this metadata for various
  /// purposes, so in the zero kernel case create the metadata entry, but don't
  /// tag any functions.  This means that now any code that iterates or counts
  /// OpenCL kernels will know that there are none rather than segfaulting
  /// because there is no metadata.
  void createOpenCLKernelsMetadata();

  ModuleState state;

  std::unique_ptr<llvm::Module> llvm_module;

  // Diagnostics state.
  uint32_t &num_errors;
  std::string &log;

  // We only need to guard against creating kernels in parallel, in case they
  // are called on the same name. If there are compiler resource conflicts
  // between creating kernels and scheduled kernels those are locked directly.
  std::mutex kernel_mutex;
  std::map<std::string, std::unique_ptr<Kernel>> kernel_map;
};  // class Module

/// @}
}  // namespace compiler
#endif  // BASE_MODULE_H_INCLUDED
