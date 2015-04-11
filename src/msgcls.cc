#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <time.h>

#include "./debug.h"

// #include "./md5.h"
#include "./msgcls.hpp"

namespace msgcls {
  const size_t MsgCls::BUFSIZE = 0xffff;
  
  MsgCls::MsgCls(const std::string &key) : key_(key) {
    this->unpkr_ = new msgpack::unpacker();
  }
  
  MsgCls::~MsgCls() {
    delete this->unpkr_;
  }

  
  void MsgCls::run(int fd) {
    
    while (true) {
      this->unpkr_->reserve_buffer(BUFSIZE);
      int rc = read(fd, this->unpkr_->buffer(),
                    this->unpkr_->buffer_capacity());
      if (rc <= 0) {
        break;
      }
      
      this->unpkr_->buffer_consumed(rc);

      while(this->unpkr_->execute()) {
        
        const msgpack::object &msg = this->unpkr_->data();
        const msgpack::object *obj = nullptr;
        try {
          if (msg.type == msgpack::type::ARRAY && msg.via.array.size == 3) {
            // Fluentd format message
            obj = &(msg.via.array.ptr[2]);
          } else if (msg.type == msgpack::type::MAP) {
            // Plain map message
            obj = &(msg);
          }
          
          if (obj) {
            for (auto &kv : obj->as<std::map<std::string, msgpack::object> >()) {
              if (kv.first == this->key_) {
                if (kv.second.type == msgpack::type::RAW) {
                  msgpack::object_raw &obj_raw = kv.second.via.raw;
                  std::string data(obj_raw.ptr, obj_raw.size);
                  // std::cout << obj_raw.size << " found!" << std::endl;
                  for (size_t i = 0; i < this->log_.size(); i++) {
                    ratio(data, this->log_[i]);
                  }
                  this->log_.push_back(data);
                  std::cout << this->log_.size() << " " << time(nullptr) << std::endl;
                }
              }
            }
          }

        } catch (msgpack::type_error &e) {
          std::cout << "Exception: " << e.what() << std::endl;
          std::cout << msg << std::endl;
        }
        delete this->unpkr_->release_zone();
        this->unpkr_->reset();
      }
      // std::auto_ptr<msgpack::zone> z(upk.release_zone());

      // this->unpkr_->release_zone();
    }
  }


  double ratio(const std::string &b1, const std::string &b2) {
    const size_t buf1_size = b1.length();
    const size_t buf2_size = b2.length();
    const char *buf1 = b1.c_str();
    const char *buf2 = b2.c_str();
    return ratio(buf1, buf1_size, buf2, buf2_size);
  }

  double ratio(const char *buf1, const size_t buf1_size,
               const char *buf2, const size_t buf2_size) {
    const size_t Lw = (buf2_size + 1);
    // int *L = new int[Lw * (buf2_size + 1)];
    size_t arr_size = sizeof(int) * (buf1_size + 1) * (buf2_size + 1);
    int *L = static_cast<int*>(malloc(arr_size));
    memset(L, 0, arr_size);

    
    int R1[buf1_size], R2[buf2_size];
    const size_t min_len = std::min (buf1_size, buf2_size);
    
    enum {NUL = 0, COM, ADD, DEL};
    memset (R1, 0, sizeof (R1));
    memset (R2, 0, sizeof (R2));
    
    for (size_t i = 0; i < buf1_size + 1; i++) {
      L[Lw * i + 0] = i;
    }
    for (size_t j = 0; j < buf2_size + 1; j++) {
      L[Lw * 0 + j] = j;
    }
    

    
    for (size_t i = 1; i < buf1_size + 1; i++) {
      for (size_t j = 1; j < buf2_size + 1; j++) {
        int d = std::min (L[Lw * (i - 1) + j] + 1, L[Lw * i + (j - 1)] + 1);
        L[(Lw * i) + j] = buf1[i - 1] == buf2[j - 1] ?
          std::min (d, L[Lw * (i - 1) + (j - 1)]) : d;
      }
    }
    
    int n = buf1_size, m = buf2_size;
    while((n > 0) && (m > 0)) {
      int a = L[Lw *  n      + (m - 1)];
      int d = L[Lw * (n - 1) +  m     ];
      int c = L[Lw * (n - 1) + (m - 1)];
      
      if (d < a) {
        if (buf1[n - 1] == buf2[m - 1] && (c < d)) {
          R1[n - 1] = R2[m - 1] = COM;
          n--; m--;
        }
        else {
          R1[n - 1] = DEL;
          n--;
        }
      }
      else {
        if (buf1[n - 1] == buf2[m - 1] && (c < a)) {
          R1[n - 1] = R2[m - 1] = COM;
          n--; m--;
        }
        else {
          R2[m - 1] = ADD;
          m--;
        }
      }
    }

    
    for(; n > 0; n--) {
      R1[n - 1] = DEL;
    }
    for(; m > 0; m--) {
      R2[m - 1] = ADD;
    }
    
    double match = 0;
    for (size_t i = 0; i < buf1_size; i++) {
      match += (R1[i] == COM) ? 1 : 0;
    }
    for (size_t j = 0; j < buf2_size; j++) {
      match += (R2[j] == COM) ? 1 : 0;
    }
    double r = match / static_cast <double> (buf1_size + buf2_size);
    
    const bool DBG = false;
    if (DBG) {
      std::string b1, b2;
      debug (DBG, "--");
      debug (DBG, "rate: %f", r);
      
      for (size_t i = 0; i < min_len; i++) {
        debug (DBG, "[%zd] = %d, %d", i, R1[i], R2[i]);
      }
    }

    free(L);
    
    return r;
  }
  
}
