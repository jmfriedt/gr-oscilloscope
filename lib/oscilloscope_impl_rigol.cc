/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Rigol oscilloscope backend.
 *   Implements waveform acquisition over VXI-11 using SCPI commands.
 *   Communication principle:
 *   - Explicit single acquisition (:SING)
 *   - Metadata query (:WAV:PRE?)
 *   - Raw waveform transfer (:WAV:DATA?)
 */

#include "oscilloscope_impl_rigol.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace gr {
namespace oscilloscope {

int scope_backend_rigol::relit(VXI11_CLINK* clink, char* buffer, int buffer_length)
{int ret = vxi11_receive(clink, buffer, buffer_length);
 if (ret > 0) buffer[ret - 1] = 0;
 return ret;
}

void scope_backend_rigol::envoi(VXI11_CLINK* clink, const char* buffer)
{vxi11_send(clink, (char*)buffer, (int)strlen(buffer));
}

bool scope_backend_rigol::init()
{char* device_name = nullptr;
 char b[256];
 _o->dev = nullptr;
 if (vxi11_open_device(&_o->dev, _o->device_ip, device_name) != 0)
   {printf("[rigol] error opening vxi11\n");
    fflush(stdout);
    return false;
   }
 envoi(_o->dev, "*IDN?");
 relit(_o->dev, b, sizeof(b));
 printf("[rigol] IDN: %s\n", b);
 fflush(stdout);
 envoi(_o->dev, "*CLS");
 envoi(_o->dev, ":WAV:FORM BYTE");
 envoi(_o->dev, ":WAV:MODE RAW");
 return true;
}

void scope_backend_rigol::shutdown()
{if (_o->dev) envoi(_o->dev, ":RUN");
}

bool scope_backend_rigol::acquire()
{if (!_o->dev) return false;

 char buffer[256];
 char prebuf[512];
 envoi(_o->dev, ":SING");
 envoi(_o->dev, "*OPC?");
 relit(_o->dev, buffer, sizeof(buffer));
 for (int chan = 1; chan <= _o->_channels; chan++)
   {sprintf(buffer, ":WAV:SOUR CHAN%d", chan);
    envoi(_o->dev, buffer);
    envoi(_o->dev, ":WAV:MODE RAW");
    envoi(_o->dev, ":WAV:FORM BYTE");
    envoi(_o->dev, ":WAV:POIN RAW");
    memset(prebuf, 0, sizeof(prebuf));
    vxi11_send_and_receive(_o->dev, (const char*)":WAV:PRE?", prebuf, sizeof(prebuf), 100 * VXI11_READ_TIMEOUT);
    char* p = prebuf;
    while (*p && ((*p < '0') || (*p > '9')) && (*p != '-')) p++;
    int idx = 0;
    char* tok = strtok(p, ",");
    double yincr = 1.0;
    double yorg  = 0.0;
    double yref  = 0.0;
    while (tok) {
      if (idx == 2) {
        int pts = atoi(tok);
        if (pts > 0 && pts != _o->_sample_size) {
          _o->_sample_size = pts;
          _o->ensure_buffers();
        }
      }
      if (idx == 7) yincr = atof(tok);
      if (idx == 8) yorg  = atof(tok);
      if (idx == 9) yref  = atof(tok);

      idx++;
      tok = strtok(nullptr, ",");
    }

    envoi(_o->dev, ":WAV:STAR 1");
    snprintf(buffer, sizeof(buffer), ":WAV:STOP %d", _o->_sample_size);
    envoi(_o->dev, buffer);
    vxi11_send_and_receive(_o->dev, (const char*)":WAV:DATA?", _o->_data_buffer, (_o->_sample_size + 256), 300 * VXI11_READ_TIMEOUT);
    if (_o->_data_buffer[0] != '#') {
      printf("[rigol] error in trace header\n"); fflush(stdout);
      return false;
    }
    int nd = _o->_data_buffer[1] - '0';
    int ln = 0;
    for (int k = 0; k < nd; k++)
      ln = 10 * ln + (_o->_data_buffer[2 + k] - '0');
    int off = 2 + nd;
    unsigned char* u = (unsigned char*)(&_o->_data_buffer[off]);
    if (ln > _o->_sample_size)
      ln = _o->_sample_size;
    for (int k = 0; k < ln; k++) {
      float v = (float)(((double)u[k]-yref)*yincr+yorg);
      if (chan == 1) _o->_tab1[k] = v;
      if (chan == 2) _o->_tab2[k] = v;
      if (chan == 3) _o->_tab3[k] = v;
      if (chan == 4) _o->_tab4[k] = v;
    }

    for (int k = ln; k < _o->_sample_size; k++) {
      if (chan == 1) _o->_tab1[k] = 0.0f;
      if (chan == 2) _o->_tab2[k] = 0.0f;
      if (chan == 3) _o->_tab3[k] = 0.0f;
      if (chan == 4) _o->_tab4[k] = 0.0f;
    }
  }
  envoi(_o->dev, ":RUN");
  return true;
}

/*
 * TODO:
 *  - Optional configuration of timebase and vertical scale
 *    (currently avoided to prevent user settings override)
 *
 *  - Support acquisition modes other than :SING
 *
 *  - Cache PRE metadata when unchanged to reduce SCPI traffic
 *
 *
 */

} // namespace oscilloscope
} // namespace gr
