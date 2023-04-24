#include "Config.hpp"
#include "ConfigParser.hpp"
#include <iostream>

Config::Config() {}

Config::~Config() {}

void Config::AddServer(const Server &server) { server_vec_.push_back(server); }

std::vector<Server> Config::GetServerVec() const { return server_vec_; }

void ParseConfig(Config &dest, const char *src_file) {
  ConfigParser parser(src_file); // Load file
  parser.Parse(dest);
}

std::ostream &operator<<(std::ostream &os, const Config &conf) {
  std::vector<Server> server_vec = conf.GetServerVec();
  if (server_vec.empty()) {
    os << "No server config" << std::endl;
    return os;
  }

  for (std::vector<Server>::const_iterator server_iter = server_vec.begin();
       server_iter != server_vec.end(); ++server_iter) {
    os << "server {" << std::endl;
    os << "  listen " << server_iter->listen_ << ";" << std::endl;
    os << "  server_name";
    for (std::vector<std::string>::const_iterator name_iter =
             server_iter->server_names_.begin();
         name_iter != server_iter->server_names_.end(); ++name_iter) {
      os << " " << *name_iter;
    }
    os << ";" << std::endl;

    for (std::vector<Location>::const_iterator location_iter =
             server_iter->locations_.begin();
         location_iter != server_iter->locations_.end(); ++location_iter) {
      os << "  location " << location_iter->path_ << " {" << std::endl;
      os << "    allow_method";
      for (std::set<method_type>::const_iterator method_iter =
               location_iter->allow_method_.begin();
           method_iter != location_iter->allow_method_.end(); ++method_iter) {
        os << " " << *method_iter;
      }
      os << ";" << std::endl;
      os << "    root " << location_iter->root_ << ";" << std::endl;
      for (std::vector<std::string>::const_iterator index_iter =
               location_iter->index_.begin();
           index_iter != location_iter->index_.end(); ++index_iter) {
        os << "    index " << *index_iter << ";" << std::endl;
      }

      for (std::map<int, std::string>::const_iterator error_page_iter =
               location_iter->error_pages_.begin();
           error_page_iter != location_iter->error_pages_.end();
           ++error_page_iter) {
        os << "    error_page " << error_page_iter->first << " "
           << error_page_iter->second << ";" << std::endl;
      }

      os << "  }" << std::endl;
    }
    os << "}" << std::endl;
  }
  return os;
}

int main() {
  try {
    Config config;
    ParseConfig(config, "config.txt");
    std::cout << config;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
