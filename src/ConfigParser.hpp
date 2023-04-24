#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "Config.hpp"

class ConfigParser {
public:
  ConfigParser(const char *filepath);
  ~ConfigParser();

  void Parse(Config &config);

  class ParserException : public std::exception {
  private:
    static const int MAX_ERROR_LEN = 1024;

  public:
    ParserException(const char *errfmt = "Parser error.", ...);
    const char *what() const throw();

  private:
    char errmsg_[MAX_ERROR_LEN];
  };

private:
  const std::string file_content_;
  std::string::const_iterator it_;

  // ピリオドを含むドメイン全体の長さ
  static const int kMaxDomainLength = 253;
  // ドメインの各ラベル(ピリオド区切りの文字列)の最大長
  static const int kMaxDomainLabelLength = 63;
  // ポート番号の最大値
  static const unsigned long kMaxPortNumber = 65535;

  std::string LoadFile(const char *filepath);

  // parser
  void ParseServer(Config &config);

  void ParseListen(Server &server);
  void ParseServerName(Server &server);
  void ParseLocation(Server &server);
  void SetLocationDefault(Location &location);

  void ParseMatch(Location &location);
  void ParseAllowMethod(Location &location);
  void ParseMaxBodySize(Location &location);
  void ParseRoot(Location &location);
  void ParseIndex(Location &location);
  void ParseIsCgi(Location &location);
  void ParseCgiPath(Location &location);
  void ParseErrorPages(Location &location);
  void ParseAutoIndex(Location &location);
  void ParseReturn(Location &location);

  // validator
  void AssertServer(const Server &server);
  void AssertPort(int &dest_port, const std::string &port_str);
  void AssertServerName(const std::string &server_name);
  bool IsValidLabel(const std::string &server_name,
                    std::string::const_iterator &it);
  void AssertLocation(const Location &location);
  void AssertMatch(match_type &dest_match, const std::string &match_str);
  void AssertAllowMethod(std::set<method_type> &dest_method,
                         const std::string &method_str);
  void AssertMaxBodySize(uint64_t &dest_size, const std::string &size_str);
  void AssertRoot(const std::string &root);
  void AssertIndex(std::vector<std::string> &dest_index,
                   const std::string &index_str);

  // utils
  char GetC();
  void Expect(char c);
  void SkipSpaces();
  std::string GetWord();

  bool IsEof();
  bool IsDelim();

  // 不使用だが、コンパイラが自動生成し、予期せず使用するのを防ぐために記述
  ConfigParser();
  ConfigParser(const ConfigParser &other);
  ConfigParser &operator=(const ConfigParser &other);
};

#endif