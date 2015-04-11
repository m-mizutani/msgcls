
#include "./gtest.h"
#include "../src/msgcls.hpp"

TEST(Sequence, ratio) {
  const std::string a1 = "abc123";
  const std::string a2 = "acb123";
  const std::string b = "egh123";
  const std::string c = "xyz345";

  EXPECT_GT(msgcls::ratio(a1, a2), 0.7);
  EXPECT_GT(msgcls::ratio(a1, b), 0.4);
  EXPECT_LT(msgcls::ratio(a1, b), 0.6);
  EXPECT_LT(msgcls::ratio(a1, c), 0.2);
}
