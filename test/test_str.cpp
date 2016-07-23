// Copyright �2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in "bss_util.h"

#include "test.h"
#include "cStr.h"

using namespace bss_util;

TESTDEF::RETPAIR test_STR()
{
  BEGINTEST;
  cStr s("blah");
  TEST(!strcmp(s, "blah"));
  cStr s2(std::move(s));
  TEST(!strcmp(s.c_str(), ""));
  TEST(!strcmp(s2, "blah"));
  cStr s3(s2);
  TEST(!strcmp(s3, "blah"));
  s3 = std::move(s2);
  cStr s5("blah", 3);
  TEST(!strcmp(s5, "bla"));
  cStr s4;
  s4 = s3;
  TEST(!strcmp(s4, "blah"));
  TEST(!strcmp(s4 + ' ', "blah "));
  TEST(!strcmp(s4 + " ", "blah "));
  TEST(!strcmp(s4 + "", "blah"));
  TEST(!strcmp(s4 + s3, "blahblah"));
  s4 += ' ';
  TEST(!strcmp(s4, "blah "));
  s4 += " a";
  TEST(!strcmp(s4, "blah  a"));
  s4 += "";
  TEST(!strcmp(s4, "blah  a"));
  s4 += s3;
  TEST(!strcmp(s4, "blah  ablah"));
  TEST(!strcmp(cStr(0, "1 2", ' '), "1"));
  TEST(!strcmp(cStr(1, "1 2", ' '), "2"));
  TEST(!strcmp(cStrF("%s2", "place"), "place2"));
  TEST(!strcmp(cStrF("2", "place"), "2"));
#ifdef BSS_PLATFORM_WIN32 // We can only run this test meaningfully on windows, because its the only one where it actually makes a difference.
  TEST(!strcmp(cStr(BSS__L("T�rkylempij�vongahdus")), "Törkylempijävongahdus"));
#endif

  s4.GetChar(6) = 'b';
  TEST(!strcmp(s4, "blah  bblah"));
  s4.GetChar(0) = 'a';
  TEST(!strcmp(s4, "alah  bblah"));
  s4.resize(80);
  s4.GetChar(60) = '2';
  s4.RecalcSize();
  TEST(s4.size() == 11);
  s3 = "  \n  trim  \n";
  TEST(!strcmp(s3.Trim(), "trim"));
  s3 = " \n \n  trim";
  TEST(!strcmp(s3.Trim(), "trim"));
  s3 = "trim \n ";
  TEST(!strcmp(s3.Trim(), "trim"));
  s3 = "trim";
  TEST(!strcmp(s3.Trim(), "trim"));
  TEST(!strcmp(s3.ReplaceChar('r', 'x'), "txim"));
  TEST(!strcmp(cStr::StripChar(s3, 't'), "xim"));
  TEST(!strcmp(cStr::StripChar(s3, 'x'), "tim"));
  TEST(!strcmp(cStr::StripChar(s3, 'm'), "txi"));

  auto a = cStr::Explode(' ', "lots of words");
  TEST(a.size() == 3);
  TEST(!strcmp(a[0], "lots"));
  TEST(!strcmp(a[1], "of"));
  TEST(!strcmp(a[2], "words"));

#ifdef BSS_COMPILER_MSC
  TEST(!strcmp(s2.c_str(), "")); // This is only supposed to happen on VC++, other compilers don't have to do this (GCC in particular doesn't).
#endif
  cStr sdfderp(s + cStr("temp") + cStr("temp") + cStr("temp") + cStr("temp"));

  std::vector<int> vec1;
  cStr::ParseTokens<int>("", ",", vec1, &atoi);
  TEST(vec1.size() == 0);
  cStr::ParseTokens<int>("1234", ",", vec1, &atoi);
  TEST(vec1.size() == 1);
  TEST(vec1[0] == 1234);
  vec1.clear();
  cStr::ParseTokens<int>("1234,235,2,6,1,0,,39,ahjs", ",", vec1, &atoi);
  TEST(vec1.size() == 8);
  TEST(vec1[0] == 1234);
  TEST(vec1[1] == 235);
  TEST(vec1[2] == 2);
  TEST(vec1[3] == 6);
  TEST(vec1[4] == 1);
  TEST(vec1[5] == 0);
  TEST(vec1[6] == 39);
  TEST(vec1[7] == 0);
  vec1.clear();
#ifdef BSS_PLATFORM_WIN32
  cStrW::ParseTokens<int>(L"", L",", vec1, &_wtoi);
  TEST(vec1.size() == 0);
  cStrW::ParseTokens<int>(L"1234,235,2,6,1,0,,39,ahjs", L",", vec1, &_wtoi);
  TEST(vec1.size() == 8);
  TEST(vec1[0] == 1234);
  TEST(vec1[1] == 235);
  TEST(vec1[2] == 2);
  TEST(vec1[3] == 6);
  TEST(vec1[4] == 1);
  TEST(vec1[5] == 0);
  TEST(vec1[6] == 39);
  TEST(vec1[7] == 0);
  vec1.clear();
#endif
  cStr::ParseTokens<int>("1234,235,2,6,1,0,,39,ahjs", ",", vec1, [](const char* s)->int { return atoi(s) + 1; });
  TEST(vec1.size() == 8);
  TEST(vec1[0] == 1235);
  TEST(vec1[1] == 236);
  TEST(vec1[2] == 3);
  TEST(vec1[3] == 7);
  TEST(vec1[4] == 2);
  TEST(vec1[5] == 1);
  TEST(vec1[6] == 40);
  TEST(vec1[7] == 1);

  cStrT<int> u32("jkl");
  TEST(u32[0] == 'j');
  TEST(u32[1] == 'k');
  TEST(u32[2] == 'l');
  ENDTEST;
}