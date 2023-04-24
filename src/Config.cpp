#include "Config.hpp"
#include "ConfigParser.hpp"
#include <iostream>

Config::Config(const char *conf_path) {
  std::ifstream ifs(conf_path);
  if (!ifs.is_open()) {
    throw ParserException("Failed to open config file");
  }

  std::string line;
  while (std::getline(ifs, line)) {
    std::istringstream iss(line);
    std::string token;
    iss >> token;

    if (token == "server") {
      Server server;
      ParseServer(ifs, server);
      sever_vec_.push_back(server);
    }
  }
}

Config::~Config() {}

std::vector<Server> Config::GetServerVec() const { return sever_vec_; }

void Config::ParseServer(std::ifstream &ifs, Server &server) {
  std::string line;
  while (std::getline(ifs, line)) {
    std::istringstream iss(line);
    std::string token;
    iss >> token;

    if (token == "}") {
      return;
    } else if (token == "listen") {
      int port;
      iss >> port;
      server.listen_.sin_port = htons(port);
    } else if (token == "server_name") {
      std::string name;
      while (iss >> name) {
        server.sv_name_.push_back(name);
      }
    } else if (token == "location") {
      Location location;
      iss >> location.name_;
      ParseLocation(ifs, location);
      server.locations_.push_back(location);
    }
  }
}

void Config::ParseLocation(std::ifstream &ifs, Location &location) {
  std::string line;
  while (std::getline(ifs, line)) {
    std::istringstream iss(line);
    std::string token;
    iss >> token;

    if (token == "}") {
      return;
    } else if (token == "allow_method") {
      std::string method;
      while (iss >> method) {
        location.allow_method_.push_back(method);
      }
    } else if (token == "client_max_body_dize") {
      size_t max_body_size;
      iss >> max_body_size;
      location.max_body_size_ = max_body_size;
    } else if (token == "root") {
      iss >> location.root_;
    } else if (token == "index") {
      iss >> location.index_;
    } else if (token == "is_cgi") {
      iss >> location.is_cgi_;
    } else if (token == "cgi_executor") {
      iss >> location.cgi_path_;
    } else if (token == "autoindex") {
      iss >> location.autoindex_;
    } else if (token == "return") {
      iss >> location.retrun_;
    } else if (token == "error_page") {
      int code;
      std::string path;
      iss >> code >> path;
      location.error_pages_[code] = path;
    }
  }
}

std::ostream &operator<<(std::ostream &os, const Config &conf) {
  std::vector<Server> server_vec = conf.GetServerVec();

  for (std::vector<Server>::const_iterator server_iter = server_vec.begin();
       server_iter != server_vec.end(); ++server_iter) {
    os << "server {" << std::endl;
    os << "  listen " << ntohs(server_iter->listen_.sin_port) << ";"
       << std::endl;
    os << "  server_name";
    for (std::vector<std::string>::const_iterator name_iter =
             server_iter->sv_name_.begin();
         name_iter != server_iter->sv_name_.end(); ++name_iter) {
      os << " " << *name_iter;
    }
    os << ";" << std::endl;

    for (std::vector<Location>::const_iterator location_iter =
             server_iter->locations_.begin();
         location_iter != server_iter->locations_.end(); ++location_iter) {
      os << "  location " << location_iter->path_ << " {" << std::endl;
      os << "    allow_method";
      for (std::vector<std::string>::const_iterator method_iter =
               location_iter->allow_method_.begin();
           method_iter != location_iter->allow_method_.end(); ++method_iter) {
        os << " " << *method_iter;
      }
      os << ";" << std::endl;
      os << "    root " << location_iter->root_ << ";" << std::endl;
      os << "    index " << location_iter->index_ << ";" << std::endl;

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

Config ParseConfig(const std::string &filepath) {
  ConfigParser parser(filepath);
  return parser.Parse();
}

int main() {
  try {
    Config conf = ParseConfig("config.txt");
    std::cout << conf;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
