#pragma once

#include "command_base.h"

namespace gtaf::cli {

class ImportCsvCommand final : public CommandBase {
public:
    [[nodiscard]] std::string name() const override { return "import"; }

    [[nodiscard]] std::string description() const override {
        return "Import data into .dat store: import <in.file> <out.dat> --format=csv [--table=name] [--key-col=N] [--delimiter=,]";
    }

    Result execute(const Command& cmd, Session& session) override;
};

} // namespace gtaf::cli
