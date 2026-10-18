#include "eda/BufferGate.h"

namespace eda {

bool BufferGate::evaluate(bool a, bool /*b*/) const { return a; }
const char* BufferGate::name() const { return "BUFFER"; }

} // namespace eda
