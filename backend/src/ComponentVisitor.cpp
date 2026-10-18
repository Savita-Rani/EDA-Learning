#include "eda/ComponentVisitor.h"

namespace eda {

void acceptVisitor(const Component& component, ComponentVisitor& visitor) {
    if (const auto* gate = dynamic_cast<const AndGate*>(&component)) {
        visitor.visitAnd(*gate);
    } else if (const auto* gate = dynamic_cast<const OrGate*>(&component)) {
        visitor.visitOr(*gate);
    } else if (const auto* gate = dynamic_cast<const NotGate*>(&component)) {
        visitor.visitNot(*gate);
    } else if (const auto* gate = dynamic_cast<const NandGate*>(&component)) {
        visitor.visitNand(*gate);
    } else if (const auto* gate = dynamic_cast<const NorGate*>(&component)) {
        visitor.visitNor(*gate);
    } else if (const auto* gate = dynamic_cast<const XorGate*>(&component)) {
        visitor.visitXor(*gate);
    } else if (const auto* gate = dynamic_cast<const BufferGate*>(&component)) {
        visitor.visitBuffer(*gate);
    } else {
        visitor.visitUnknown(component);
    }
}

} // namespace eda
