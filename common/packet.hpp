#pragma once

#include <stdlib.h>
#include <strings.h>

#include <cstdint>

namespace MyEcho {
// 二进制包封装
class Packet {
 public:
  ~Packet() {
    if (data_) free(data_);
  }
  void Alloc(size_t len) {
    if (data_) free(data_);
    data_ = (uint8_t *)malloc(len);
    len_ = len;
    use_len_ = 0;
    parse_len_ = 0;
  }
  void Reset() {
    use_len_ = 0;
    parse_len_ = 0;
  }
  void ReAlloc(size_t len) {
    if (len < len_) return;  // 指定的缓冲区长度小于当前缓冲区的长度，则直接返回
    data_ = (uint8_t *)realloc(data_, len);
    len_ = len;
  }
  uint8_t *Data() { return data_; }  // 原始缓冲区的开始地址
  uint8_t *Data4Write() { return data_ + use_len_; }  // 用于写入的缓冲区开始地址
  uint8_t *Data4Parse() { return data_ + parse_len_; }  // 用于解析的缓冲区开始地址
  size_t UseLen() { return use_len_; }  //缓冲区已经使用的容量
  size_t CanWriteLen() { return len_ - use_len_; }  //缓存区中还可以写入的数据长度
  size_t NeedParseLen() { return use_len_ - parse_len_; }  //还需要解析的长度
  void UpdateUseLen(size_t add_len) { use_len_ += add_len; }
  void UpdateParseLen(size_t add_len) { parse_len_ += add_len; }

 public:
  uint8_t *data_{nullptr};  // 二进制缓冲区
  size_t len_{0};  // 缓冲区的总长度
  size_t use_len_{0};  // 缓冲区中已经使用的长度
  size_t parse_len_{0};  // 缓冲区中已经完成解析的长度
};
}  // namespace MyEcho