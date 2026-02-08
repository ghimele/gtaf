#pragma once

#include "command_base.h"

namespace gtaf::cli {

class ImportCsvCommand final : public CommandBase {
public:
    [[nodiscard]] std::string name() const override { return "importcsv"; }

    [[nodiscard]] std::string description() const override {
        return "Import CSV into .dat store: importcsv <in.csv> <out.dat> [--table=name] [--key-col=N] [--delimiter=,]";
    }

    Result execute(const Command& cmd, Session& session) override;
};

} // namespace gtaf::cli
