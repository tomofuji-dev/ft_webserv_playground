#include "ConfigParser.hpp"
#include "utils.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

ConfigParser::ConfigParser(const char *filepath)
    : file_content_(LoadFile(filepath)), it_(file_content_.begin()) {}

ConfigParser::~ConfigParser() {}

std::string ConfigParser::LoadFile(const char *filepath) {
  std::string dest;
  std::ifstream ifs(filepath);
  std::stringstream buffer;
  if (!ifs || ifs.fail()) {
    throw ParserException("Failed to open config file: %s", filepath);
  }
  buffer << ifs.rdbuf();
  ifs.close();
  dest = buffer.str();
  return dest;
}

// parser
void ConfigParser::Parse(Config &config) {
  while (!IsEof()) {
    SkipSpaces();
    if (GetWord() != "server") {
      throw ParserException("Unexpected block");
    }
    ParseServer(config);
  }
}

void ConfigParser::ParseServer(Config &config) {
  Server server;

  SkipSpaces();
  Expect('{');
  while (!IsEof() && *it_ != '}') {
    SkipSpaces();
    std::string token = GetWord();
    if (token == "listen") {
      ParseListen(server);
    } else if (token == "server_name") {
      ParseServerName(server);
    } else if (token == "location") {
      ParseLocation(server);
    } else {
      throw ParserException("Unexpected token: %s", token.c_str());
    }
    SkipSpaces();
  }
  Expect('}');
  AssertServer(server);
  config.AddServer(server);
}

void ConfigParser::ParseListen(Server &server) {
  SkipSpaces();
  std::string port_str = GetWord();
  SkipSpaces();
  Expect(';');
  AssertPort(server.listen_, port_str);
  std::cout << "listen: " << server.listen_ << std::endl;
}

void ConfigParser::ParseServerName(Server &server) {
  std::cout << "server_name" << std::endl;
}

void ConfigParser::ParseLocation(Server &server) {
  std::cout << "location" << std::endl;
}

void ConfigParser::ParseMatch(Location &location) {}

void ConfigParser::ParseAllowMethod(Location &location) {}

void ConfigParser::ParseMaxBodySize(Location &location) {}

void ConfigParser::ParseRoot(Location &location) {}

void ConfigParser::ParseIndex(Location &location) {}

void ConfigParser::ParseIsCgi(Location &location) {}

void ConfigParser::ParseCgiPath(Location &location) {}

void ConfigParser::ParseErrorPages(Location &location) {}

void ConfigParser::ParseAutoIndex(Location &location) {}

void ConfigParser::ParseReturn(Location &location) {}

// validator
void ConfigParser::AssertServer(const Server &server) {}

void ConfigParser::AssertPort(int &dest_port, const std::string &src_str) {
  if (!ws_strtoi(&dest_port, src_str)) {
    throw ParserException("Invalid port number: %s", src_str.c_str());
  }
  if (dest_port < 0 || dest_port > kMaxPortNumber) {
    throw ParserException("Invalid port number: %d", dest_port);
  }
}

// utils
char ConfigParser::GetC() {
  if (IsEof()) {
    throw ParserException("Unexpected EOF");
  }
  return *it_++;
}

void ConfigParser::Expect(char c) {
  if (IsEof()) {
    throw ParserException("Unexpected EOF");
  }
  if (*it_ != c) {
    throw ParserException("Expected %c, but unexpected char: %c", c, *it_);
  }
  it_++;
}

void ConfigParser::SkipSpaces() {
  while (!IsEof() && isspace(*it_)) {
    it_++;
  }
}

std::string ConfigParser::GetWord() {
  std::string word;
  while (!IsEof() && !IsDelim() && !isspace(*it_)) {
    word += *it_++;
  }
  if (word.empty()) {
    throw ParserException("Empty Word");
  }
  return word;
}

bool ConfigParser::IsEof() { return it_ == file_content_.end(); }

bool ConfigParser::IsDelim() {
  return *it_ == ';' || *it_ == '{' || *it_ == '}';
}

// ParserException
ConfigParser::ParserException::ParserException(const char *errfmt, ...) {
  va_list args;
  va_start(args, errfmt);

  vsnprintf(errmsg_, MAX_ERROR_LEN, errfmt, args);
  va_end(args);
}

const char *ConfigParser::ParserException::what() const throw() {
  return errmsg_;
}
