#pragma once

#include <arpa/inet.h>
#include <string.h>

#include <string>

#include "packet.hpp"

namespace MyEcho {
#define TINY_ECHO_HEAD_LEN 4
enum DecodeStatus {
  HEAD = 1,  // 解析协议头（协议体长度）
  BODY = 2,  // 解析协议体
  FINISH = 3,  // 完成解析
};

class Codec {
 public:
  Codec() { pkt_.Alloc(TINY_ECHO_HEAD_LEN); }
  ~Codec() {
    if (msg_) delete msg_;
  }
  void Reset() {
    pkt_.Reset();
    body_len_ = 0;
    decode_status_ = HEAD;
    if (msg_) delete msg_;
    msg_ = nullptr;
  }
  DecodeStatus GetDecodeStatus() { return decode_status_; }
  uint8_t *Data() { return pkt_.Data4Write(); }
  size_t Len() { return pkt_.CanWriteLen(); }
  void EnCode(const std::string &msg, Packet &pkt) {
    pkt.Alloc(msg.length() + TINY_ECHO_HEAD_LEN);
    *(uint32_t *)pkt.Data() = htonl(msg.length());  // 协议体长度转换成网络字节序
    memmove(pkt.Data() + TINY_ECHO_HEAD_LEN, msg.data(), msg.length());
    pkt.UpdateUseLen(TINY_ECHO_HEAD_LEN + msg.length());
  }
  void DeCode(size_t len) {
    pkt_.UpdateUseLen(len);
    uint8_t *data = (uint8_t *)pkt_.Data4Parse();
    uint32_t need_parse_len = pkt_.NeedParseLen();  // 还有多少字节需要解析
    while (need_parse_len > 0) {  // 只要还有未解析的网络字节流，就持续解析
      bool decode_break = false;
      if (HEAD == decode_status_) {  // 解析协议头
        decodeHead(&data, need_parse_len, decode_break);
        if (decode_break) break;
      }
      if (BODY == decode_status_) {  // 解析完协议头，解析协议体
        decodeBody(&data, need_parse_len, decode_break);
        if (decode_break) break;
      }
    }
  }
  std::string *GetMessage() {
    if (nullptr == msg_) return nullptr;
    if (decode_status_ != FINISH) return nullptr;
    std::string *result = msg_;
    msg_ = nullptr;
    return result;
  }

 private:
  bool decodeHead(uint8_t **data, uint32_t &need_parse_len, bool &decode_break) {
    if (need_parse_len < TINY_ECHO_HEAD_LEN) {
      decode_break = true;
      return true;
    }
    body_len_ = ntohl(*(uint32_t *)(*data));
    need_parse_len -= TINY_ECHO_HEAD_LEN;
    (*data) += TINY_ECHO_HEAD_LEN;
    decode_status_ = BODY;
    pkt_.UpdateParseLen(TINY_ECHO_HEAD_LEN);
    pkt_.ReAlloc(TINY_ECHO_HEAD_LEN + body_len_);
    *data = (uint8_t *)pkt_.Data4Parse();  // 这里要注意，调用ReAlloc之后，缓冲区会变化，所以这里data要重新赋值
    return true;
  }
  bool decodeBody(uint8_t **data, uint32_t &need_parse_len, bool &decode_break) {
    if (need_parse_len < body_len_) {
      decode_break = true;
      return true;
    }
    if (msg_ == nullptr) {
      msg_ = new std::string(reinterpret_cast<const char *>(*data), body_len_);
    }
    need_parse_len -= body_len_;
    (*data) += body_len_;
    decode_status_ = FINISH;
    pkt_.UpdateParseLen(body_len_);
    return true;
  }

 private:
  DecodeStatus decode_status_{HEAD};  // 当前解析状态
  uint32_t body_len_{0};  // 当前消息的协议体长度
  std::string *msg_{nullptr};  // 解析的消息
  Packet pkt_;
};
}  // namespace MyEcho
