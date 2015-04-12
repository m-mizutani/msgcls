#ifndef __SRC_MSGCLS_HPP__
#define __SRC_MSGCLS_HPP__

#include <string>
#include <vector>
#include "./ptw.h"

namespace msgpack {
  class unpacker;
  struct object;
}

namespace msgcls {
  class Cluster;
  
  class RatioWorker : public ptw::Queue {
  private:
    const Cluster &c_;
    std::string data_;
    double r_;

  public:
    RatioWorker(const Cluster &c, const std::string &data) :
    c_(c), data_(data) {}
    void exec();
    double ratio() const { return this->r_; };
    const Cluster& cluster() const { return this->c_; }
  };

  class Cluster {
  private:
    std::string base_;
    std::string hash_hex_;
    
  public:
    Cluster(const std::string& base);
    ~Cluster() {};
    double ratio(const std::string& data);
    const std::string &base() const { return this->base_; }
    const std::string &hv() const { return this->hash_hex_; }
  };
  
  class Classifier {
  private:
    std::vector<Cluster*> cluster_;
    ptw::Ptw ptw_;

  public:
    Classifier() {};
    ~Classifier() {};
    const Cluster& classify(const std::string &data);
    size_t cluster_size() const { return this->cluster_.size(); }
  };

  
  class Emitter {
  public:
    Emitter() {};
    virtual ~Emitter() {};
    virtual void emit(const msgpack::object &obj, const Cluster& cluster) = 0;
  };
  
  class FileEmitter : public Emitter {
  private:
    std::string out_dir_;
    
  public:
    FileEmitter(const std::string &out_dir);
    virtual ~FileEmitter();
    void emit(const msgpack::object &obj, const Cluster& cluster);
  };
  
  
  class MsgCls {
  private:
    static const size_t BUFSIZE;
    msgpack::unpacker *unpkr_;
    std::string key_;
    Classifier *classifier_;
    Emitter *emitter_;
    
  public:
    MsgCls(const std::string &key);
    ~MsgCls();
    void run(int fd);
    void set_emitter(Emitter *emitter) { this->emitter_ = emitter; }
  };

  double ratio(const char *buf1, const size_t buf1_size,
               const char *buf2, const size_t buf2_size);
  double ratio(const std::string &a, const std::string &b);
}

#endif    // __SRC_MSGCLS_HPP__
