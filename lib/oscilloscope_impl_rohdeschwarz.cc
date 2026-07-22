/* -*- c++ -*- */
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Rohde & Schwarz oscilloscope backend.
 *   Implements waveform acquisition over VXI-11 using SCPI.
 *   Communication principle:
 *   - Configure binary transfer (FORM:DATA INT,16)
 *   - Single acquisition (RUNSINGLE + *OPC?)
 *   - Per-channel waveform fetch (CHANx:WAV:DATA?)
 */

#include "oscilloscope_impl_rohdeschwarz.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <locale.h>

namespace gr {
namespace oscilloscope {

int scope_backend_rohdeschwarz::relit(VXI11_CLINK* clink, char* buffer, int buffer_length)
{int ret = vxi11_receive(clink, buffer, buffer_length);
 if (ret > 0) buffer[ret - 1] = 0;
 return ret;
}

void scope_backend_rohdeschwarz::envoi(VXI11_CLINK* clink, const char* s)
{vxi11_send(clink, (char*)s, (int)std::strlen(s));
}

bool scope_backend_rohdeschwarz::init()
{char* device_name = nullptr;
 char buf[256];
 _o->dev = nullptr;
 if (vxi11_open_device(&_o->dev, _o->_device_ip, device_name) != 0)
   {printf("[R&S] error opening vxi11\n"); fflush(stdout);
    return false;
   }
 if (!_o || !_o->dev) return false;
 envoi(_o->dev, "*IDN?");
 relit(_o->dev, buf, sizeof(buf));
 printf("[R&S] IDN: %s\n", buf); fflush(stdout);
 envoi(_o->dev, "*CLS");
 envoi(_o->dev, "FORM:DATA INT,16\n");
 for (int c=1;c<=_o->_channels;c++)
    {sprintf(buf,"CHAN%d:STAT ON\n",c);envoi(_o->dev,buf);
     sprintf(buf,"CHAN%d:STAT?\n",c);envoi(_o->dev,buf);
     relit(_o->dev,buf,256);
     printf("[R&S] CHAN%d STAT: %s\n",c,buf);
    }
 return true;
}

void scope_backend_rohdeschwarz::shutdown()
{
}

bool scope_backend_rohdeschwarz::apply_range(float range)
{char buf[128];
 if (!_o || !_o->dev) return false;
 for (int ch = 1; ch <= _o->_channels; ch++)
   {sprintf(buf, "CHAN%d:SCAL %f\n", ch, range);
    envoi(_o->dev, buf);
  }
  return true;
}

bool scope_backend_rohdeschwarz::apply_rate(float rate)
{if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(_o->_duration * rate);
 _o->_rate=rate;
 _o->ensure_buffers();
 char buf[128];
 sprintf(buf, "ACQ:SRATE %f\n", rate);
 envoi(_o->dev, buf);
 return true;
}

bool scope_backend_rohdeschwarz::apply_duration(float dur)
{char buf[128];
 if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(dur * _o->_rate);
 _o->_duration=dur;
 _o->ensure_buffers();
 sprintf(buf, "TIM:RANGE %f\n", dur);
 envoi(_o->dev, buf);
 return true;
}

bool scope_backend_rohdeschwarz::acquire()
{char tmp[256] = {0};
 char cmd[128];
 if (!_o || !_o->dev) return false;
 envoi(_o->dev, "*CLS");
 envoi(_o->dev, "FORM:DATA INT,16\n");
 envoi(_o->dev, "RUNSINGLE\n");
 envoi(_o->dev, "*OPC?\n");
 relit(_o->dev, tmp, sizeof(tmp));
 const int want = 2 * _o->_sample_size + 100; // Expected size: binary + header
 for (int chan = 1; chan <= _o->_channels; chan++)
   {sprintf(cmd, "CHAN%d:WAV:DATA?\n", chan);
    vxi11_send_and_receive(_o->dev, (char*)cmd, _o->_data_buffer, want, 100 * VXI11_READ_TIMEOUT);
    if (_o->_data_buffer[0] != '#') {
      std::printf("[R&S] error in trace header (chan %d)\n", chan);
      return false;
    }
    long offset = (long)(_o->_data_buffer[1] - '0');
    for (int k = 0; k < _o->_sample_size; k++) {
      float v = (float)(*(short*)(&_o->_data_buffer[2 * k + offset + 2])) / 65536.0f;
      if (chan == 1) _o->_tab1[k] = v; // ^^^ valid only on Intel/LE
      if (chan == 2) _o->_tab2[k] = v;
      if (chan == 3) _o->_tab3[k] = v;
      if (chan == 4) _o->_tab4[k] = v;
    }
  }
  return true;
}
} // namespace oscilloscope
} // namespace gr
