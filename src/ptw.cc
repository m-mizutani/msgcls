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

#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "ptw.h"
#include "debug.h"

namespace ptw {
  const bool DBG = false;

  // ----------------------------------------------------------
  // Queue
  Queue::Queue () : next_(NULL) {
  }
  Queue::~Queue () {
  }
  Queue * Queue::detach () {
    if (this->next_) {
      Queue * next = this->next_;
      this->next_ = NULL;
      return next;
    }
    else {
      return NULL;
    }
  }

  QueueList::QueueList () : head_(NULL), tail_(NULL), count_(0) {}
  QueueList::~QueueList () {}
  void QueueList::push (Queue * q) {
    assert (!((this->head_ == NULL) ^ (this->tail_ == NULL)));
    q->next_ = NULL;
    if (this->tail_) {
      this->tail_->next_ = q;
      this->tail_ = q;
    }
    else {
      assert (this->head_ == NULL);
      this->head_ = this->tail_ = q;
    }
    this->count_++;
    assert (this->count_ > 0);
  }
  void QueueList::push_bulk (QueueList *q_list) {
    assert (!((this->head_ == NULL) ^ (this->tail_ == NULL)));

    if (q_list->count_ == 0) {
      assert (q_list->head_ == NULL && q_list->tail_ == NULL);
      return ;
    }

    if (this->tail_) {
      this->tail_->next_ = q_list->head_;
      this->tail_ = q_list->tail_;
    }
    else {
      assert (this->head_ == NULL);
      this->head_ = q_list->head_;
      this->tail_ = q_list->tail_;
    }

    this->count_ += q_list->count_;

    q_list->head_ = q_list->tail_ = NULL;
    q_list->count_ = 0;

    assert (this->count_ > 0);
  }
  Queue * QueueList::pop () {
    assert (!((this->head_ == NULL) ^ (this->tail_ == NULL)));
    // debug (1, "(%p) head = %p", this, this->head_);


    if (this->head_) {
      Queue * q = this->head_;
      this->head_ = q->next_;

      if (this->tail_ == q) {
        assert (q->next_ == NULL);
        this->tail_ = NULL;
      }

      this->count_--;
      assert (this->count_ >= 0);
      q->next_ = NULL;
      return q;
    } 
    else {
      return NULL;
    }
  }
  Queue * QueueList::pop_bulk () {
    assert (!((this->head_ == NULL) ^ (this->tail_ == NULL)));

    if (this->head_) {
      Queue * q = this->head_;      
      this->head_ = this->tail_ = NULL;
      this->count_ = 0;
      return q;
    } 
    else {
      return NULL;
    }    
  }
  int QueueList::count () const {
    return this->count_;
  }

  // ----------------------------------------------------------
  // Worker
  Worker::Worker (Ptw * ptw) : ptw_(ptw), done_(0), wait_(0) {
    ::pthread_mutex_init (&(this->mutex_), NULL);
    ::pthread_cond_init (&(this->cond_), NULL);
  }
  Worker::~Worker () {
    // debug (0, "done = %zd, wait = %zd", this->done_, this->wait_);
  }
  void Worker::ret_queue (QueueList * q) {
    this->ptw_->ret_queue (q);
  }
  int Worker::input_queue (Queue * q) {
    int qc;
    pthread_mutex_lock (&(this->mutex_));
    // debug (DBG, "input queue: %p", q);
    this->queue_.push (q);
    qc = this->queue_.count ();
    pthread_cond_signal (&(this->cond_));
    pthread_mutex_unlock (&(this->mutex_));    
    return qc;
  }
  void Worker::run () {
    ::pthread_create (&(this->th_), NULL, Worker::loop, this);
  }

  void * Worker::loop (void * obj) {
    Worker *w = static_cast<Worker *>(obj);
    Queue *q, *next;
    debug (DBG, "start loop %p", w);

    while (true) {
      pthread_mutex_lock (&(w->mutex_));
      if (NULL == (q = w->queue_.pop_bulk ())) {
        w->wait_++;
        pthread_cond_wait (&(w->cond_), &(w->mutex_));
      }

      if (NULL == q) {
        q = w->queue_.pop_bulk ();
      }
      pthread_mutex_unlock (&(w->mutex_));
      
      if (q) {
        int c = 0;
        do {
          next = q->detach ();
          c++;
          w->done_++;
          q->exec ();
          w->out_queue_.push (q);
          // w->ret_queue (q);
        } while (NULL != (q = next));

        w->ret_queue (&(w->out_queue_));
      }
    }    
  }

  pthread_t Worker::pthread () const {
    return this->th_;
  }

  // ----------------------------------------------------------
  // Ptw
  Ptw::Ptw (size_t worker_num) : 
    worker_(worker_num, NULL), 
    last_ptr_(0),
    in_count_(0),
    out_count_(0),
    q_total_(0), q_count_(0)
  { 
    ::pthread_mutex_init (&(this->mutex_), NULL);
    ::pthread_cond_init (&(this->cond_), NULL);

    if (worker_num == 0) {
      worker_num = Ptw::cpu_core_num ();
      this->worker_.resize (worker_num, NULL);
    }
    for (size_t i = 0; i < worker_num; i++) {
      this->worker_[i] = new Worker (this);
      this->worker_[i]->run ();
    }
  }

  Ptw::~Ptw () {
    // debug (1, "avg queuing: %f", (double)this->q_total_ / (double)this->q_count_);

    for (size_t i = 0; i < this->worker_.size (); i++) {
      pthread_cancel (this->worker_[i]->pthread ());
      pthread_join (this->worker_[i]->pthread (), NULL);
      delete this->worker_[i];
    }
  }

  void Ptw::ret_queue (QueueList * q_list) {
    pthread_mutex_lock (&(this->mutex_));
    this->queue_.push_bulk (q_list);
    pthread_cond_signal (&(this->cond_));
    pthread_mutex_unlock (&(this->mutex_));
  }

  void Ptw::push_queue (Queue *q) {
    this->last_ptr_++;

    // simple round robin algorithm
    if (this->last_ptr_ >= this->worker_.size ()) {
      this->last_ptr_ = 0;
    }

    this->in_count_++;
    int qc = this->worker_[this->last_ptr_]->input_queue (q);
    // this->q_total_ += qc;
    // this->q_count_ += 1;
  }

  Queue * Ptw::pop_queue (bool wait) {
    Queue * q;
    
    if (0 == this->buf_queue_.count ()) {

      pthread_mutex_lock (&(this->mutex_));
      if (0 == this->queue_.count () &&
          wait && this->in_count_ > this->out_count_) {
        pthread_cond_wait (&(this->cond_), &(this->mutex_));
      }

      this->buf_queue_.push_bulk (&(this->queue_));

      pthread_mutex_unlock (&(this->mutex_));

    }

    q = this->buf_queue_.pop ();
    if (q) {
      this->out_count_++;
    }
    return q;
  }

  size_t Ptw::worker_num () const {
    return this->worker_.size ();
  }

  size_t Ptw::cpu_core_num () {
    return ::sysconf(_SC_NPROCESSORS_ONLN);
  }

}



