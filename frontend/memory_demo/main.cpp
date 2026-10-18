#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <eda/Component.h>
#include <eda/FileHandle.h>
#include <eda/OwningPtr.h>
#include <eda/SharedOwningPtr.h>

// ============================================================================
// main.cpp — console driver for Phase 7.
//
// Demonstrates OwningPtr<T> and SharedOwningPtr<T> OWNING Phase 2's
// Component — reusing that type as something concrete to own, not
// reimplementing it. FileHandle demonstrates RAII survives an exception,
// the actual reason RAII is more than a style preference.
// ============================================================================

using namespace eda;

namespace {

void demonstrateUniqueOwnership() {
    std::cout << "\n--- OwningPtr<Component>: unique ownership ---\n";

    OwningPtr<Component> gate = makeOwning<AndGate>();
    std::cout << gate->name() << "(1, 1) = " << gate->evaluate(true, true) << "\n";

    OwningPtr<Component> moved = std::move(gate);
    std::cout << "after std::move: original owns "
              << (gate ? "something (bug!)" : "nothing") << "\n";
    std::cout << "moved-to now owns: " << moved->name() << "\n";
    // gate->evaluate(...) here would be a use-after-move bug — OwningPtr
    // can't stop you from writing that, same as std::unique_ptr can't.
}

void demonstrateSharedOwnership() {
    std::cout << "\n--- SharedOwningPtr<Component>: shared ownership ---\n";

    // Unlike OwningPtr, SharedOwningPtr has no converting constructor
    // (kept simpler on purpose — see SharedOwningPtr.h) — construct it
    // already typed as Component directly, instead of via makeShared.
    SharedOwningPtr<Component> first(new OrGate());
    std::cout << "useCount after construction: " << first.useCount() << "\n";

    SharedOwningPtr<Component> second = first; // copy — increments the count
    std::cout << "useCount after copy: " << first.useCount()
              << " (both point at the same control block)\n";

    SharedOwningPtr<Component> moved = std::move(first);
    std::cout << "useCount after std::move (not copy): " << moved.useCount()
              << " — move transfers, never increments\n";
    std::cout << "original now owns: " << (first ? "something (bug!)" : "nothing") << "\n";

    std::cout << second->name() << "(1, 0) = " << second->evaluate(true, false) << "\n";
}

void writeTraceThenThrow(FileHandle& log) {
    log.writeLine("n1 = AND(1, 1) = 1");
    log.writeLine("n2 = OR(1, 0) = 1");
    throw std::runtime_error("simulated failure partway through a run");
    log.writeLine("n3 = NOT(0) = 1"); // never reached
}

void demonstrateRaiiSurvivesExceptions(const std::string& path) {
    std::cout << "\n--- RAII survives exceptions ---\n";

    try {
        FileHandle log(path);
        writeTraceThenThrow(log);
    } catch (const std::exception& e) {
        std::cout << "caught: " << e.what() << "\n";
        std::cout << "(FileHandle's destructor already ran while the exception\n"
                     " was unwinding the stack — the file was flushed and\n"
                     " closed before we even got to this catch block)\n";
    }

    std::ifstream check(path);
    std::ostringstream contents;
    contents << check.rdbuf();
    std::cout << "file contents, read back after the exception:\n" << contents.str();
}

} // namespace

int main() {
    std::cout << "=== EDA Logic Simulator — Phase 7: Memory management ===\n";

    demonstrateUniqueOwnership();
    demonstrateSharedOwnership();
    demonstrateRaiiSurvivesExceptions("phase7_trace.log");

    return 0;
}
