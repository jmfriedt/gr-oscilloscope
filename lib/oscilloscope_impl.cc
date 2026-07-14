/* -*- c++ -*- */
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Original oscilloscope GNU Radio block by:
 *   Jean-Michel Friedt <jmfriedt@femto-st.fr>/Thomas Lavarenne
 *
 * This is a refactoring / reorganization:
 *  - backend abstraction
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>

#include "scope_types.h"
#include "oscilloscope_impl.h"
#include "oscilloscope_factory.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <locale.h>
#include <unistd.h>

namespace gr {
namespace oscilloscope {

oscilloscope::sptr
oscilloscope::make(char* ip, float range, float rate, float duration, int channels, int type)
{return gnuradio::make_block_sptr<oscilloscope_impl>( ip, range, rate, duration, channels, type);
}

static int count_dots(const char* s)
{int c = 0;
 for (size_t i = 0; s && s[i]; i++) if (s[i] == '.') c++;
 return c;
}

oscilloscope_impl::oscilloscope_impl(char* ip, float range, float rate, float duration, int channels, int type)
  : gr::sync_block("oscilloscope",
                   gr::io_signature::make(0, 0, 0),
                   gr::io_signature::make(1, 4, sizeof(float)))
{ set_type(type);
  set_ip(ip);
  _range    = range;
  _rate     = rate;
  _duration = duration;
  _channels = channels;
  _sample_size = (int)(_duration * _rate);
  if (_sample_size < 256) _sample_size = 256;
  ensure_buffers();
  _backend = make_backend(_type, this);
  if (_backend) {
    _backend->init();
    _backend->apply_channels(_channels);
    _backend->apply_range(_range);
    _backend->apply_rate(_rate);
    _backend->apply_duration(_duration);
  }
}

oscilloscope_impl::~oscilloscope_impl()
{ if (_backend) _backend->shutdown();
  if (_vxi11 == 0 && sockfd >= 0) close(sockfd);
  std::free(_data_buffer);
  std::free(_tab1);
  std::free(_tab2);
  std::free(_tab3);
  std::free(_tab4);
}

void oscilloscope_impl::ensure_buffers()
{ if (_sample_size <= 0) _sample_size = 8192;
  _data_buffer = (char*)std::realloc(_data_buffer, (size_t)(2 * _sample_size + 512));
  _tab1 = (float*)std::realloc(_tab1, (size_t)_sample_size * sizeof(float));
  _tab2 = (float*)std::realloc(_tab2, (size_t)_sample_size * sizeof(float));
  _tab3 = (float*)std::realloc(_tab3, (size_t)_sample_size * sizeof(float));
  _tab4 = (float*)std::realloc(_tab4, (size_t)_sample_size * sizeof(float));
}

void oscilloscope_impl::set_type(int t)
{_type  = t;
 _vxi11 = (t == SCOPE_RIGOL || t == SCOPE_AGILENT || t == SCOPE_ROHDE_SCHWARZ);
}

void oscilloscope_impl::set_ip(char* ip)
{if (!ip || count_dots(ip) != 3)
   ip = (char*)"127.0.0.1";
 std::snprintf(device_ip, sizeof(device_ip), "%s", ip);
}

void oscilloscope_impl::set_channels(int c)
{_channels = c;
 if (_backend) _backend->apply_channels(_channels);
}

void oscilloscope_impl::set_range(float r)
{_range = r;
 if (_backend) _backend->apply_range(_range);
}

void oscilloscope_impl::set_rate(float r)
{_rate = r;
 _sample_size = (int)(_duration * _rate);
 if (_sample_size < 256) _sample_size = 256;
 ensure_buffers();
 if (_backend) _backend->apply_rate(_rate);
}

void oscilloscope_impl::set_duration(float d)
{_duration = d;
 _sample_size = (int)(_duration * _rate);
 if (_sample_size < 256) _sample_size = 256;
 ensure_buffers();
 if (_backend) _backend->apply_duration(_duration);
}

int oscilloscope_impl::work(int noutput_items,
                            gr_vector_const_void_star&,
                            gr_vector_void_star& output_items)
{
  float* out0 = (float*)output_items[0];
  float* out1 = output_items.size() > 1 ? (float*)output_items[1] : nullptr;
  float* out2 = output_items.size() > 2 ? (float*)output_items[2] : nullptr;
  float* out3 = output_items.size() > 3 ? (float*)output_items[3] : nullptr;

  if (_num_values == 0) {
    _num_values = _sample_size;
    _position   = 0;

    if (!_backend || !_backend->acquire()) {
      for (int i = 0; i < _sample_size; i++)
        _tab1[i] = _tab2[i] = _tab3[i] = _tab4[i] = 0.0f;
    }
  }

  int k = 0;
  for (; k < noutput_items && _position < _sample_size; k++) {
    out0[k] = _tab1[_position];
    if (_channels >= 2 && out1) out1[k] = _tab2[_position];
    if (_channels >= 3 && out2) out2[k] = _tab3[_position];
    if (_channels >= 4 && out3) out3[k] = _tab4[_position];
    _num_values--;
    _position++;
  }

  return k;
}

} // namespace oscilloscope
} // namespace gr
