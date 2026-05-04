#ifndef _ACC_OPERATIONS_H
#define _ACC_OPERATIONS_H

#include "acc_pkg.h"
#include <cstdint>
#include <functional>
#include <unordered_map>

class Acc;

namespace ACC_Operations {

std::unordered_map<ACC_PKG::ACC_MODE, std::function<void()>>
createHandlers(Acc *acc, bool include_multiplier);

namespace Impl {
void handleIdle(Acc *acc);
void handleAdd(Acc *acc);
void handleSub(Acc *acc);
void handleVadd(Acc *acc);
void handleVsub(Acc *acc);
void handleCapture(Acc *acc);
void handleVmacAdd(Acc *acc);
void handleVmacSub(Acc *acc);
void handleSquareAdd(Acc *acc);
void handleSquareSub(Acc *acc);
} // namespace Impl

} // namespace ACC_Operations

#endif // _ACC_OPERATIONS_H
