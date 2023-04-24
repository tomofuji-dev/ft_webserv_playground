#ifndef CONF_HPP
#define CONF_HPP

#include <map>
#include <netinet/in.h>
#include <string>
#include <vector>

// 複数指定不可の単一のみの設定項目で、複数指定された場合は最後の一つだけ保持する
enum match_type {
  prefix, // 前方一致
  back,   // 後方一致
};

struct Location {
  std::string path_;
  match_type match_; // 後方一致は、CGIの場合のみ使用可能
  std::vector<std::string> allow_method_; // GET POST DELETE から１個以上指定
  size_t max_body_size_; // 任意 単一 デフォルト１MB, 0は無制限 制限超え 413
                         // Request Entity Too Large
                         // 制限されるのはボディ部分でヘッダーは含まない
  std::string root_; // 必須 単一 相対パスの起点
  std::string index_; // 任意 単一 ディレクトリへのアクセス時に返すファイル
  bool is_cgi_; // デフォルトはfalse 一致する場合は、CGIとして処理する
  std::string cgi_path_; // execve(cgi_path, X, X) is_cgi_がtrueの場合必須
  std::map<int, std::string> error_pages_;
  // 任意 複数可 mapのkeyはエラーコード, valueはエラーページのパス
  // エラーコードに重複があった場合は、最後の一つだけ保持する（上書きされる）
  bool autoindex_;     // 任意 単一 デフォルトはfalse
  std::string retrun_; // 任意 単一
                       // HTTPステータスコード301と共に<path>へリダイレクトする｡
};

struct Server { // 各バーチャルサーバーの設定を格納する
  struct sockaddr_in listen_; // 必須 単一

  std::vector<std::string> sv_name_;
  // 任意 単一 ディレクティブは一つで、複数指定された場合は最後の一つだけ保持
  // 一つのディレクティブ内に、サーバーネームは並べて複数可能

  std::vector<Location> locations_; // 任意 複数可
};

class Config {
public:
  Config();
  ~Config();
  std::vector<Server> GetServerVec() const;

private:
  std::vector<Server> server_vec_; // 必須 複数可 複数の場合、一番上がデフォ
  // 不使用だが、コンパイラが自動生成し、予期せず使用するのを防ぐために記述
  Config(const Config &other);
  Config &operator=(const Config &other);
};

void ParseConfig(Config &dest, const char *src_file);
std::ostream &operator<<(std::ostream &os, const Config &conf);

#endif