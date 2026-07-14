/* -*- c++ -*- */
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Declares the oscilloscope GNU Radio block and the backend interface.
 *   Defines the contract between the block and instrument-specific backends.
 */

#pragma once

#include <gnuradio/oscilloscope/oscilloscope.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <locale.h>
#include <memory>

// Oscilloscope type identifiers
#include "scope_types.h"

// VXI-11 support
#include "./vxi11/library/vxi11_user.h"

namespace gr {
namespace oscilloscope {

/*
 * Backend interface.
 * All instrument-specific logic lives here.
 */
class scope_backend
{
public:
  virtual ~scope_backend() = default;

  virtual bool init() = 0;
  virtual void shutdown() = 0;

  virtual bool apply_range(float)     { return true; }
  virtual bool apply_rate(float)      { return true; }
  virtual bool apply_duration(float)  { return true; }
  virtual bool acquire() = 0;
};

/*
 * GNU Radio block implementation.
 * Owns buffers and delegates acquisition to the backend.
 */
class oscilloscope_impl : public oscilloscope
{
public:
  oscilloscope_impl(char* ip,
                    float range,
                    float rate,
                    float duration,
                    int channels,
                    int type);

  ~oscilloscope_impl() override;

  int work(int noutput_items,
           gr_vector_const_void_star& input_items,
           gr_vector_void_star& output_items) override;

  // Block parameters
  void set_type(int t);
  void set_ip(char* ip);
  void set_range(float r);
  void set_rate(float r);
  void set_duration(float d);

  int  type() const     { return _type; }
  bool is_vxi11() const { return _vxi11 == 1; }

  /*
   * Backend-shared resources.
   * Owned by the block, used by backends.
   */

  // VXI-11 instruments
  VXI11_CLINK* dev = nullptr;

  // TCP/IP instruments
  int sockfd = -1;

  char _device_ip[16] = {0};

  float _range     = 1.0f;
  float _rate      = 1.0f;
  float _duration  = 0.01f;
  int   _channels  = 1;

  int   _sample_size = 8192;
  char* _data_buffer = nullptr;

  float* _tab1 = nullptr;
  float* _tab2 = nullptr;
  float* _tab3 = nullptr;
  float* _tab4 = nullptr;

  int _num_values = 0;
  int _position   = 0;

  char _vxi11 = 0;
  int  _type  = SCOPE_TEKTRONIX;

  // Rigol-specific state
  int   _rigol_inited = 0;
  float _rigol_xincr  = 0.0f;

  void ensure_buffers();

private:
  std::unique_ptr<scope_backend> _backend;
};

} // namespace oscilloscope
} // namespace gr
