#pragma once

#include "common.h"
#include "mycoroutine.h"

namespace MyCoroutine {
// 协程本地变量模版类封装
template <typename Type>
class CoroutineLocal {
 public:
  CoroutineLocal(Schedule &schedule) : schedule_(schedule) {}
  static void free(void *data) {
    if (data) delete (Type *)data;
  }

  Type &Get() {
    MyCoroutine::LocalVariable local_variable;
    bool result = schedule_.LocalVariableGet(this, local_variable);
    assert(result == true);
    return *(Type *)local_variable.data;
  }
  // 重载类型转换操作符，实现协程本地变量直接给Type类型的变量赋值的功能
  operator Type() { return Get(); }
  // 重载赋值操作符，实现Type类型的变量直接给协程本地变量赋值的功能
  CoroutineLocal &operator=(const Type &value) {
    Set(value);
    return *this;
  }

 private:
  void Set(Type value) {
    Type *data = new Type(value);
    MyCoroutine::LocalVariable local_variable;
    local_variable.data = data;
    local_variable.free = free;
    schedule_.LocalVariableSet(this, local_variable);
  }

 private:
  Schedule &schedule_;
};
}  // namespace MyCoroutine