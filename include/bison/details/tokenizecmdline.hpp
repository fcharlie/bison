//////
#ifndef BISON_WIN_TOKENIZE_CMDLINE_HPP
#define BISON_WIN_TOKENIZE_CMDLINE_HPP
#include <string_view>
#include <cstring>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

namespace bison {
namespace cmdline_internal {
constexpr bool isWhitespace(char ch) { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; }

inline std::string_view StripTrailingWhitespace(std::string_view str) {
  auto it = std::find_if_not(str.rbegin(), str.rend(), isWhitespace);
  return str.substr(0, str.rend() - it);
}

constexpr bool isWhitespaceOrNull(char ch) { return isWhitespace(ch) || ch == L'\0'; }
constexpr bool isQuote(char ch) { return ch == '\"' || ch == '\''; }

inline void vector_fill_n(std::vector<char> &Token, size_t n, char ch) {
  for (size_t i = 0; i < n; i++) {
    Token.push_back(ch);
  }
}
inline size_t parseBackslash(std::string_view Src, size_t I, std::vector<char> &Token) {
  auto E = Src.size();
  int BackslashCount = 0;
  // Skip the backslashes.
  do {
    ++I;
    ++BackslashCount;
  } while (I != E && Src[I] == '\\');

  bool FollowedByDoubleQuote = (I != E && Src[I] == '"');
  if (FollowedByDoubleQuote) {
    vector_fill_n(Token, BackslashCount / 2, '\\');
    if (BackslashCount % 2 == 0) {
      return I - 1;
    }
    Token.push_back('"');
    return I;
  }
  vector_fill_n(Token, BackslashCount, '\\');
  return I - 1;
}
} // namespace cmdline_internal

class Tokenizer {
public:
  Tokenizer() = default;
  Tokenizer(const Tokenizer &) = delete;
  Tokenizer &operator=(const Tokenizer &) = delete;
  ~Tokenizer() {
    for (auto a : saver_) {
      if (a != nullptr) {
        free(a);
      }
    }
  }
  bool Tokenize(std::string_view cmdline);
  const char *const *Argv() const { return saver_.data(); };
  char **Argv() { return saver_.data(); }
  size_t Argc() const { return saver_.size(); }

private:
  std::vector<char *> saver_;
  void SaveArg(const char *data, size_t len);
  void SaveArg(const std::vector<char> &token) { SaveArg(token.data(), token.size()); }
};

void Tokenizer::SaveArg(const char *data, size_t len) {
  auto size = len + 1;
  auto mem = static_cast<char *>(malloc(size * sizeof(char)));
  if (mem != nullptr) {
    memcpy(mem, data, len * sizeof(char));
    mem[len] = '\0';
    saver_.push_back(mem);
  }
}

inline bool Tokenizer::Tokenize(std::string_view src) {
  src = cmdline_internal::StripTrailingWhitespace(src);
  if (src.empty()) {
    return false;
  }
  std::vector<char> token;
  token.reserve(128);
  // This is a small state machine to consume characters until it reaches the
  // end of the source string.
  enum { INIT, UNQUOTED, QUOTED } State = INIT;
  for (size_t I = 0, E = src.size(); I != E; ++I) {
    auto C = src[I];

    // INIT state indicates that the current input index is at the start of
    // the string or between tokens.
    if (State == INIT) {
      if (cmdline_internal::isWhitespaceOrNull(C)) {
        continue;
      }
      if (C == '"') {
        State = QUOTED;
        continue;
      }
      if (C == '\\') {
        I = cmdline_internal::parseBackslash(src, I, token);
        State = UNQUOTED;
        continue;
      }
      token.push_back(C);
      State = UNQUOTED;
      continue;
    }

    // UNQUOTED state means that it's reading a token not quoted by double
    // quotes.
    if (State == UNQUOTED) {
      // Whitespace means the end of the token.
      if (cmdline_internal::isWhitespaceOrNull(C)) {
        SaveArg(token);
        token.clear();
        State = INIT;
        continue;
      }
      if (C == '"') {
        State = QUOTED;
        continue;
      }
      if (C == '\\') {
        I = cmdline_internal::parseBackslash(src, I, token);
        continue;
      }
      token.push_back(C);
      continue;
    }

    // QUOTED state means that it's reading a token quoted by double quotes.
    if (State == QUOTED) {
      if (C == '"') {
        if (I < (E - 1) && src[I + 1] == '"') {
          // Consecutive double-quotes inside a quoted string implies one
          // double-quote.
          token.push_back('"');
          I = I + 1;
          continue;
        }
        State = UNQUOTED;
        continue;
      }
      if (C == '\\') {
        I = cmdline_internal::parseBackslash(src, I, token);
        continue;
      }
      token.push_back(C);
    }
  }
  SaveArg(token);
  return true;
}

} // namespace bison

#endif
