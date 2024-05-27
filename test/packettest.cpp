#include "packet.hpp"
#include "unittestcore.hpp"

TEST_CASE(Packet_Alloc) {
  MyEcho::Packet pkt;
  pkt.Alloc(100);
  ASSERT_EQ(pkt.UseLen(), 0);
  ASSERT_EQ(pkt.CanWriteLen(), 100);
}

TEST_CASE(Packet_ReAlloc) {
  MyEcho::Packet pkt;
  pkt.Alloc(100);
  ASSERT_EQ(pkt.UseLen(), 0);
  ASSERT_EQ(pkt.CanWriteLen(), 100);
  pkt.ReAlloc(99);
  ASSERT_EQ(pkt.UseLen(), 0);
  ASSERT_EQ(pkt.CanWriteLen(), 100);
  pkt.ReAlloc(200);
  ASSERT_EQ(pkt.UseLen(), 0);
  ASSERT_EQ(pkt.CanWriteLen(), 200);
}

TEST_CASE(Packet_Used) {
  MyEcho::Packet pkt;
  pkt.Alloc(100);
  pkt.UpdateUseLen(10);
  pkt.UpdateParseLen(6);
  ASSERT_EQ(pkt.CanWriteLen(), 90);
  ASSERT_EQ(pkt.UseLen(), 10);
  ASSERT_EQ(pkt.NeedParseLen(), 4);
  uint8_t* data = pkt.Data();
  uint8_t* data4Write = pkt.Data4Write();
  uint8_t* data4Parse = pkt.Data4Parse();
  ASSERT_EQ(data + 10, data4Write);
  ASSERT_EQ(data + 6, data4Parse);
}