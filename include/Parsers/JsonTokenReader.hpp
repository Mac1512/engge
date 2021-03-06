#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include "GGPack.hpp"

namespace ng::Json {
enum class TokenId {
  None,
  NewLine,
  Number,
  Whitespace,
  Colon,
  Comma,
  String,
  Comment,
  StartHash,
  EndHash,
  End
};

struct Token {
  TokenId id;
  std::streampos start;
  std::streampos end;

  friend std::ostream &operator<<(std::ostream &os, const Token &obj);

private:
  [[nodiscard]] std::string readToken() const;
};

class TokenReader {
public:
  class Iterator : public std::iterator<std::forward_iterator_tag, Token> {
  private:
    TokenReader &_reader;
    std::streampos _pos;
    Token _token;

  public:
    Iterator(TokenReader &reader, std::streampos pos);
    Iterator(const Iterator &it);
    Iterator &operator++();
    Iterator operator++(int);

    TokenReader &getReader();

    bool operator==(const Iterator &rhs) const { return _pos == rhs._pos; }
    bool operator!=(const Iterator &rhs) const { return _pos != rhs._pos; }
    Token &operator*();
    const Token &operator*() const;
    Token *operator->();
  };

  typedef Iterator iterator;

public:
  TokenReader();

  bool load(const std::string &path);
  void parse(const std::vector<char> &content);
  iterator begin();
  iterator end();
  bool readToken(Token &token);
  std::string readText(std::streampos pos, std::streamsize size);
  std::string readText(const Token &token);

private:
  TokenId readTokenId();
  TokenId readNumber();
  TokenId readString();
  TokenId readQuotedString();

private:
  GGPackBufferStream _stream;
};

class Parser {
public:
  Parser();
  static void load(const std::string &path, GGPackValue &value);
  static void parse(const std::vector<char> &buffer, GGPackValue &value);

  static GGPackValue parse(const std::vector<char> &buffer);

private:
  static void parse(ng::Json::TokenReader &reader, GGPackValue &value);
  static void parse(TokenReader::iterator &it, GGPackValue &value);
  static void readHash(TokenReader::iterator &it, GGPackValue &value);
};
} // namespace ng
