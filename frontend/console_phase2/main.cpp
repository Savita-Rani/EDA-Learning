#include <iostream>
#include <memory>
#include <vector>
#include <eda/Component.h>

// ============================================================================
// main.cpp — console driver for Phase 2.
//
// Separate from frontend/console/main.cpp (Phase 1) entirely — that file
// is untouched. Same visible output (truth tables for all six gates), but
// built on the polymorphic Component hierarchy instead of the switch-based
// Gate.
//
// Concepts introduced here:
//  - std::unique_ptr<Component>   : exclusive-ownership smart pointer.
//    Component is abstract, so it can't live by value in the vector; a
//    unique_ptr owns a heap-allocated AndGate/OrGate/... and deletes it
//    automatically (via Component's virtual destructor) when the vector
//    is destroyed. No manual `delete` anywhere in this file.
//  - std::make_unique<T>(...)     : allocates a T and wraps it in a
//    unique_ptr in one step — prefer this over `new T(...)` wrapped by
//    hand, mainly for exception-safety reasons covered properly in
//    Phase 7.
//  - std::vector<std::unique_ptr<Component>> : a container of DIFFERENT
//    concrete types (AndGate, OrGate, NotGate, ...), all accessed through
//    one common Component interface. This is the payoff of polymorphism:
//    printTruthTable() below doesn't know or care which gate it's holding.
//  - Passing `const Component&`   : printTruthTable takes a base-class
//    reference. Whichever derived object you pass in, `component.evaluate()`
//    calls the CORRECT override via the vtable — this is dynamic dispatch,
//    the direct replacement for Gate.cpp's `switch` statement.
// ============================================================================

using namespace eda;

void printTruthTable(const Component& component) {
    std::cout << "\n" << component.name() << " gate truth table:\n";
    std::cout << "  A  B  | OUT\n";
    std::cout << "  -------------\n";

    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            bool out = component.evaluate(a, b);
            std::cout << "  " << a << "  " << b << "  |  " << out << "\n";
        }
    }
}

int main() {
    std::vector<std::unique_ptr<Component>> components;
    components.push_back(std::make_unique<AndGate>());
    components.push_back(std::make_unique<OrGate>());
    components.push_back(std::make_unique<NotGate>());
    components.push_back(std::make_unique<NandGate>());
    components.push_back(std::make_unique<NorGate>());
    components.push_back(std::make_unique<XorGate>());

    std::cout << "=== EDA Logic Simulator — Phase 2: OOP (polymorphic gates) ===\n";

    for (const auto& component : components) {
        // `*component` dereferences the unique_ptr to get a Component&;
        // printTruthTable has no idea which concrete class it actually is.
        printTruthTable(*component);
    }

    return 0;
}
