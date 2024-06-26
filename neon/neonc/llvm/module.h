#pragma once

#include <neonc.h>

namespace neonc {
    struct Module {
        Module(
            std::shared_ptr<llvm::LLVMContext> context,
            std::shared_ptr<llvm::Module> module,
            const std::string target_cpu,
            const std::string target_features
        ): context(context), module(module), target_cpu(target_cpu), target_features(target_features) {
            dummy_builder = std::make_shared<llvm::IRBuilder<>>(*context);
        }

        void dump() const;
        void verify() const;

        std::shared_ptr<llvm::IRBuilder<>> dummy_builder;

        llvm::Function * get_function();
        std::map<std::string, llvm::Value *> & get_arguments();
        std::shared_ptr<llvm::IRBuilder<>> get_builder();
        llvm::Function * get_function(const std::string & id);
        std::map<std::string, llvm::Value *> & get_arguments(const std::string & id);
        std::shared_ptr<llvm::IRBuilder<>> get_builder(const std::string & id);

        std::map<std::string, llvm::Value *> local_variables;

        std::string pointer;
        // args ------------------------------------------------------|
        std::map<std::string, std::tuple<std::tuple<llvm::Function *, std::map<std::string, llvm::Value *>>, std::shared_ptr<llvm::IRBuilder<>>>> functions;

        std::shared_ptr<llvm::LLVMContext> context;
        std::shared_ptr<llvm::Module> module;

        const std::string target_cpu;
        const std::string target_features;
    };
}
