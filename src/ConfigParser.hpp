#ifndef CONF_HPP
#define CONF_HPP

class ConfigParser {
private:
  std::vector<Server> sever_vec_; // 必須 複数可 複数の場合、一番上がデフォ

public:
  ConfigParser(const std::string &filepath);
  ~ConfigParser();

  Config Parse();

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

  std::string LoadFile(const std::string &filepath);

  void ParseServer(Config &conf);

  void ParseListen(Server &server);
  void ParseServerName(Server &server);
  void ParseLocation(Server &server);

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

  // utils
  char GetC();
  void Expect(char c);
  void SkipSpaces();
  std::string GetWord();

  // 不使用だが、コンパイラが自動生成し、予期せず使用するのを防ぐために記述
  CofigParser();
  ConfigParser(const ConfigParser &other);
  ConfigParser &operator=(const ConfigParser &other);
};

#endif