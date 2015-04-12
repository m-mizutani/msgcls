/*-
 * Copyright (c) 2013 Masayoshi Mizutani <mizutani@sfc.wide.ad.jp>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __LIB_PTW_H__
#define __LIB_PTW_H__

#include <vector>
#include <queue>
#include <pthread.h>

namespace ptw {
  class Ptw;
  class Queue;

  class QueueList {
  private:
    Queue * head_;
    Queue * tail_;
    int count_;

  public:
    QueueList ();
    ~QueueList ();
    void push (Queue * q);
    void push_bulk (QueueList *q_list);
    Queue * pop ();
    Queue * pop_bulk ();
    int count () const;
  };


  class Queue {
    friend class QueueList;
  private:
    Queue * next_;

  public:    
    Queue ();
    virtual ~Queue ();
    virtual void exec () = 0; 
    Queue * detach (); // return detached queues
  };

  class Worker {
  private:
    QueueList queue_;
    QueueList out_queue_;
    Ptw * ptw_;
    pthread_t th_;
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
    
    size_t done_;
    size_t wait_;

    static void * loop (void * obj);

  public:
    Worker (Ptw * ptw);
    virtual ~Worker ();
    int input_queue (Queue * q);
    void run ();
    void ret_queue (QueueList * q);
    pthread_t pthread () const;
  };

  class Ptw {
    friend void Worker::ret_queue (QueueList * q_list);

  private:
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;    
    std::vector <Worker *> worker_;
    QueueList queue_;
    QueueList buf_queue_;

    int last_ptr_;
    int in_count_;
    int out_count_;

    size_t q_total_;
    size_t q_count_;

    void ret_queue (QueueList * q_list);

  public:
    Ptw (size_t worker_num=0);
    ~Ptw ();
    void push_queue (Queue *q); 
    Queue * pop_queue (bool wait=true);

    size_t worker_num () const;
    static size_t cpu_core_num () ;
  };
}

#endif
