#ifndef __SRC_MSGCLS_HPP__
#define __SRC_MSGCLS_HPP__

#include <string>
#include <vector>
#include <msgpack.hpp>

namespace msgcls {
  enum Format {
    PLAIN = 0,
    FLUENTD,
  };

  class MsgCls {
  private:
    static const size_t BUFSIZE;
    msgpack::unpacker *unpkr_;
    std::string key_;
    Format fmt_;
    std::vector<std::string> log_;
    
  public:
    MsgCls(const std::string &key);
    ~MsgCls();
    void set_format(msgcls::Format fmt) { this->fmt_ = fmt; }
    void run(int fd);
  };

  double ratio(const char *buf1, const size_t buf1_size,
               const char *buf2, const size_t buf2_size);
  double ratio(const std::string &a, const std::string &b);
}

#endif    // __SRC_MSGCLS_HPP__
