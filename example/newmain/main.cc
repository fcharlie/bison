#include <cstdio>
#include <bison/argv.hpp>
#include <bison/codecvt.hpp>
#ifdef _WIN32
#include <SDKDDKVer.h>
#ifndef _WINDOWS_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //
#endif
#endif
#include <Windows.h>
#endif

int cmd_main(int argc, char **argv) {
  //
  return 0;
}

#ifdef _WIN32
int wmain(int argc, wchar_t **) {
  auto cmdline = GetCommandLineW();
  auto u8cmdline = bison::encode_into<wchar_t, char>(cmdline);
  bison::Tokenizer tokenizer;
  if (!tokenizer.Tokenize(u8cmdline)) {
    return 127;
  }
  return cmd_main(static_cast<int>(tokenizer.Argc()), tokenizer.Argv());
}
#else
int main(int argc, char **argv) {
  //
  return cmd_main(argc, argv);
}
#endif