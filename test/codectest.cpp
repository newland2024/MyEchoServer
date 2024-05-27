#include "codec.hpp"
#include "packet.hpp"
#include "unittestcore.hpp"

TEST_CASE(Codec_Init) {
  MyEcho::Codec codec;
  ASSERT_EQ(codec.Len(), 4);
}

TEST_CASE(Codec_EnCodeAndDeCode) {
  MyEcho::Codec codec;
  MyEcho::Packet pkt;
  std::string message = "hello world";
  codec.EnCode(message, pkt);
  ASSERT_EQ(pkt.UseLen(), message.length() + 4);
  for (size_t i = 0; i < pkt.UseLen(); i++) {
    uint8_t* data = pkt.Data();
    *codec.Data() = *(data + i);
    codec.DeCode(1);
  }
  std::string* temp = codec.GetMessage();
  ASSERT_TRUE(temp != nullptr);
  ASSERT_EQ((*temp), message);
  delete temp;
}