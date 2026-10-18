#include "eda/ComponentFactory.h"

#include <stdexcept>

namespace eda {

void ComponentFactory::registerType(const std::string& name, Creator creator) {
    if (creators_.find(name) != creators_.end()) {
        throw std::invalid_argument("ComponentFactory: '" + name + "' is already registered");
    }
    creators_[name] = std::move(creator);
}

std::unique_ptr<Component> ComponentFactory::create(const std::string& name) const {
    auto it = creators_.find(name);
    if (it == creators_.end()) {
        throw std::out_of_range("ComponentFactory: no type registered under '" + name + "'");
    }
    return it->second();
}

bool ComponentFactory::isRegistered(const std::string& name) const noexcept {
    return creators_.find(name) != creators_.end();
}

std::vector<std::string> ComponentFactory::registeredTypes() const {
    std::vector<std::string> types;
    types.reserve(creators_.size());
    for (const auto& [name, creator] : creators_) {
        types.push_back(name);
    }
    return types;
}

ComponentFactory makeStandardGateFactory() {
    ComponentFactory factory;
    factory.registerType("AND",  [] { return std::make_unique<AndGate>(); });
    factory.registerType("OR",   [] { return std::make_unique<OrGate>(); });
    factory.registerType("NOT",  [] { return std::make_unique<NotGate>(); });
    factory.registerType("NAND", [] { return std::make_unique<NandGate>(); });
    factory.registerType("NOR",  [] { return std::make_unique<NorGate>(); });
    factory.registerType("XOR",  [] { return std::make_unique<XorGate>(); });
    return factory;
}

} // namespace eda
