#include <iostream>
#include <memory>
#include <vector>
#include <eda/BufferGate.h>
#include <eda/ComponentFactory.h>
#include <eda/ComponentVisitor.h>
#include <eda/Signal.h>

// ============================================================================
// main.cpp — console driver for Phase 10: Factory, Visitor, refined
// Observer (Signal<Args...>), all demonstrated against Phase 2's
// Component hierarchy (reused) plus this phase's own BufferGate.
// ============================================================================

using namespace eda;

namespace {

void demonstrateFactory() {
    std::cout << "\n--- Factory: ComponentFactory ---\n";

    ComponentFactory factory = makeStandardGateFactory();
    factory.registerType("BUFFER", [] { return std::make_unique<BufferGate>(); });

    std::cout << "Registered types: ";
    for (const std::string& type : factory.registeredTypes()) {
        std::cout << type << " ";
    }
    std::cout << "\n";

    for (const char* type : {"AND", "BUFFER"}) {
        std::unique_ptr<Component> gate = factory.create(type);
        std::cout << "  created " << gate->name() << ": evaluate(true, false) = "
                  << gate->evaluate(true, false) << "\n";
    }

    std::cout << "Attempting factory.create(\"UNKNOWN\")...\n";
    try {
        factory.create("UNKNOWN");
    } catch (const std::exception& e) {
        std::cout << "  caught: " << e.what() << "\n";
    }

    std::cout << "Attempting to re-register \"AND\" (duplicate)...\n";
    try {
        factory.registerType("AND", [] { return std::make_unique<AndGate>(); });
    } catch (const std::exception& e) {
        std::cout << "  caught: " << e.what() << "\n";
    }
    std::cout << "  factory still works afterward -- creating AND again: "
              << factory.create("AND")->name() << "\n";
}

class GateCountVisitor : public ComponentVisitor {
public:
    int andCount = 0, orCount = 0, notCount = 0, nandCount = 0, norCount = 0, xorCount = 0, bufferCount = 0, unknownCount = 0;

    void visitAnd(const AndGate&) override { ++andCount; }
    void visitOr(const OrGate&) override { ++orCount; }
    void visitNot(const NotGate&) override { ++notCount; }
    void visitNand(const NandGate&) override { ++nandCount; }
    void visitNor(const NorGate&) override { ++norCount; }
    void visitXor(const XorGate&) override { ++xorCount; }
    void visitBuffer(const BufferGate&) override { ++bufferCount; }
    void visitUnknown(const Component&) override { ++unknownCount; }
};

void demonstrateVisitor() {
    std::cout << "\n--- Visitor: ComponentVisitor (external dispatch, no accept() needed) ---\n";

    std::vector<std::unique_ptr<Component>> gates;
    gates.push_back(std::make_unique<AndGate>());
    gates.push_back(std::make_unique<AndGate>());
    gates.push_back(std::make_unique<NandGate>());
    gates.push_back(std::make_unique<XorGate>());
    gates.push_back(std::make_unique<BufferGate>());

    GateCountVisitor counter;
    for (const auto& gate : gates) {
        acceptVisitor(*gate, counter);
    }

    std::cout << "  AND=" << counter.andCount << " NAND=" << counter.nandCount
              << " XOR=" << counter.xorCount << " BUFFER=" << counter.bufferCount
              << " unknown=" << counter.unknownCount << "\n";
}

void demonstrateSignal() {
    std::cout << "\n--- Refined Observer: Signal<Args...> ---\n";
    std::cout << "(Phase 4's EventSimulator::addWatcher is this SAME pattern,\n"
                 " specialized for 'a wire's bool value changed.' Signal<Args...>\n"
                 " is the pattern generalized to any event shape at all.)\n";

    Signal<bool> wireChanged;
    int emitCount = 0;

    std::size_t loggerId = wireChanged.connect([](bool value) {
        std::cout << "  [logger] wire changed to " << value << "\n";
    });
    wireChanged.connect([&emitCount](bool) { ++emitCount; });

    std::cout << "emit(true), emit(false) with both slots connected:\n";
    wireChanged.emit(true);
    wireChanged.emit(false);

    wireChanged.disconnect(loggerId);
    std::cout << "disconnected the logger; emit(true) again:\n";
    wireChanged.emit(true);

    std::cout << "  counter slot saw " << emitCount << " emissions total (logger saw fewer)\n";

    // A different shape entirely -- same class, no changes needed.
    Signal<std::string, int> renamed;
    renamed.connect([](const std::string& name, int id) {
        std::cout << "  [renamed] #" << id << " is now called \"" << name << "\"\n";
    });
    renamed.emit("OUT", 3);
}

} // namespace

int main() {
    std::cout << "=== EDA Logic Simulator — Phase 10: Design patterns ===\n";

    demonstrateFactory();
    demonstrateVisitor();
    demonstrateSignal();

    return 0;
}
