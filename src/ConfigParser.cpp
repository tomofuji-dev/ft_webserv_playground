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
  server.listen_ = -1;

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

  // for debug
  std::cout << "listen: " << server.listen_ << std::endl;
}

void ConfigParser::ParseServerName(Server &server) {
  std::vector<std::string> new_server_names;
  while (!IsEof() && *it_ != ';') {
    SkipSpaces();
    std::string server_name = GetWord();
    SkipSpaces();
    AssertServerName(server_name);
    new_server_names.push_back(server_name);
  }
  Expect(';');
  server.server_names_ = new_server_names;

  // for debug
  std::cout << "server_name: ";
  for (std::vector<std::string>::iterator it = server.server_names_.begin();
       it != server.server_names_.end(); ++it) {
    std::cout << *it << " ";
  }
  std::cout << std::endl;
}

void ConfigParser::ParseLocation(Server &server) {
  // for debug
  std::cout << "location" << std::endl;

  Location location;
  SetLocationDefault(location);

  SkipSpaces();
  location.path_ = GetWord();
  SkipSpaces();
  Expect('{');
  while (!IsEof() && *it_ != '}') {
    SkipSpaces();
    std::string token = GetWord();
    if (token == "match") {
      ParseMatch(location);
    } else if (token == "allow_method") {
      ParseAllowMethod(location);
    } else if (token == "max_body_size") {
      ParseMaxBodySize(location);
    } else if (token == "root") {
      ParseRoot(location);
    } else if (token == "index") {
      ParseIndex(location);
    } else if (token == "is_cgi") {
      ParseIsCgi(location);
    } else if (token == "cgi_path") {
      ParseCgiPath(location);
    } else if (token == "error_pages") {
      ParseErrorPages(location);
    } else if (token == "autoindex") {
      ParseAutoIndex(location);
    } else if (token == "return") {
      ParseReturn(location);
    } else {
      throw ParserException("Unexpected token: %s", token.c_str());
    }
    SkipSpaces();
  }
  Expect('}');
  AssertLocation(location);
  server.locations_.push_back(location);
}

void ConfigParser::SetLocationDefault(Location &location) {
  location.match_ = PREFIX;
  location.max_body_size_ = 1024 * 1024; // 1MB
  location.is_cgi_ = false;
  location.autoindex_ = false;
}

void ConfigParser::ParseMatch(Location &location) {
  SkipSpaces();
  std::string match_str = GetWord();
  SkipSpaces();
  Expect(';');
  AssertMatch(location.match_, match_str);

  // for debug
  std::cout << "match: " << location.match_ << std::endl;
}

void ConfigParser::ParseAllowMethod(Location &location) {
  if (!location.allow_method_.empty()) {
    throw ParserException("allow_method is already set");
  };
  while (!IsEof() && *it_ != ';') {
    SkipSpaces();
    std::string method_str = GetWord();
    SkipSpaces();
    AssertAllowMethod(location.allow_method_, method_str);
  }
  Expect(';');

  // for debug
  std::cout << "allow_method: ";
  const char *method_str[] = {"GET", "POST", "DELETE"};
  for (std::set<method_type>::iterator it = location.allow_method_.begin();
       it != location.allow_method_.end(); ++it) {
    std::cout << method_str[*it] << " ";
  }
  std::cout << std::endl;
}

void ConfigParser::ParseMaxBodySize(Location &location) {
  SkipSpaces();
  std::string size_str = GetWord();
  SkipSpaces();
  Expect(';');
  AssertMaxBodySize(location.max_body_size_, size_str);

  // for debug
  std::cout << "max_body_size: " << location.max_body_size_ << std::endl;
}

void ConfigParser::ParseRoot(Location &location) {
  SkipSpaces();
  location.root_ = GetWord();
  SkipSpaces();
  Expect(';');
  AssertRoot(location.root_);

  // for debug
  std::cout << "root: " << location.root_ << std::endl;
}

void ConfigParser::ParseIndex(Location &location) {}

void ConfigParser::ParseIsCgi(Location &location) {}

void ConfigParser::ParseCgiPath(Location &location) {}

void ConfigParser::ParseErrorPages(Location &location) {}

void ConfigParser::ParseAutoIndex(Location &location) {}

void ConfigParser::ParseReturn(Location &location) {}

// validator
void ConfigParser::AssertServer(const Server &server) {
  if (server.listen_ == -1) {
    throw ParserException("Listen port is not set");
  }
}

void ConfigParser::AssertPort(int &dest_port, const std::string &port_str) {
  if (!ws_strtoi<int>(&dest_port, port_str)) {
    throw ParserException("Invalid port number: %s", port_str.c_str());
  }
  if (dest_port < 0 || dest_port > kMaxPortNumber) {
    throw ParserException("Invalid port number: %d", dest_port);
  }
}

void ConfigParser::AssertServerName(const std::string &server_name) {
  if (server_name.empty()) {
    throw ParserException("Empty server name");
  }
  if (server_name.size() > kMaxDomainLength) {
    throw ParserException("Invalid server name: %s", server_name.c_str());
  }
  // 各ラベルの長さが1~63文字であり、ハイフンと英数字のみを含む(先頭、末尾はハイフン以外)ことを確認
  for (std::string::const_iterator it = server_name.begin();
       it < server_name.end();) {
    if (!IsValidLabel(server_name, it)) {
      throw ParserException("Invalid server name: %s", server_name.c_str());
    }
  }
}

bool ConfigParser::IsValidLabel(const std::string &server_name,
                                std::string::const_iterator &it) {
  std::string::const_iterator start = it;
  while (it < server_name.end() && *it != '.') {
    if (!(isdigit(*it) || isalpha(*it) || *it == '-')) {
      return false;
    }
    if (*it == '-' && (it == start || it + 1 == server_name.end())) {
      return false;
    }
    it++;
  }
  if (it - start == 0 || it - start > kMaxDomainLabelLength) {
    return false;
  }
  if (it < server_name.end() && *it == '.') {
    it++;
    if (it == server_name.end()) {
      return false;
    }
  }
  return true;
}

void ConfigParser::AssertLocation(const Location &location) {}

void ConfigParser::AssertMatch(match_type &dest_match,
                               const std::string &match_str) {
  if (match_str == "prefix") {
    dest_match = PREFIX;
  } else if (match_str == "back") {
    dest_match = BACK;
  } else {
    throw ParserException("Invalid match type: %s", match_str.c_str());
  }
}

void ConfigParser::AssertAllowMethod(std::set<method_type> &dest_method,
                                     const std::string &method_str) {
  method_type src_method;

  if (method_str == "GET") {
    src_method = GET;
  } else if (method_str == "POST") {
    src_method = POST;
  } else if (method_str == "DELETE") {
    src_method = DELETE;
  } else {
    throw ParserException("Invalid method: %s", method_str.c_str());
  }
  if (dest_method.find(src_method) != dest_method.end()) {
    throw ParserException("Duplicated method: %s", method_str.c_str());
  }
  dest_method.insert(src_method);
}

void ConfigParser::AssertMaxBodySize(uint64_t &dest_size,
                                     const std::string &size_str) {
  char unit = 'B';
  std::size_t len = size_str.length();

  if (len == 0) {
    throw ParserException("Empty size str");
  }
  if (size_str[len - 1] == 'B' || size_str[len - 1] == 'K' ||
      size_str[len - 1] == 'M' || size_str[len - 1] == 'G') {
    unit = size_str[len - 1];
    len--;
  }
  if (len == 0) {
    throw ParserException("Invalid size: %s", size_str.c_str());
  }
  if (!ws_strtoi<uint64_t>(&dest_size, size_str.substr(0, len))) {
    throw ParserException("Invalid size: %s", size_str.c_str());
  }
  switch (unit) {
  case 'B':
    break;
  case 'K':
    dest_size = mul_assert_overflow<uint64_t>(dest_size, 1024);
    break;
  case 'M':
    dest_size = mul_assert_overflow<uint64_t>(dest_size, 1024 * 1024);
    break;
  case 'G':
    dest_size = mul_assert_overflow<uint64_t>(dest_size, 1024 * 1024 * 1024);
    break;
  default:
    throw ParserException("Invalid size: %s", size_str.c_str());
  }
}

void ConfigParser::AssertRoot(const std::string &root) {}

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
