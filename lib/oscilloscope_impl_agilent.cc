// lib/oscilloscope_impl_agilent.cc
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Agilent oscilloscope backend.
 *   Implements waveform acquisition over VXI-11 using SCPI.
 */

#include "oscilloscope_impl_agilent.h"
#include "./vxi11/library/vxi11_user.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

namespace gr {
namespace oscilloscope {

void scope_backend_agilent::envoi(VXI11_CLINK* clink, const char* s)
{vxi11_send(clink, (char*)s, (int)std::strlen(s));
}

bool scope_backend_agilent::init()
{if (!_o || !_o->dev) return false;
 envoi(_o->dev, "*CLS");
 // envoi(_o->dev, "*RST");
 envoi(_o->dev, ":SYSTEM:HEADER OFF");
 envoi(_o->dev, ":TRIGGER:EDGE:SOURCE CHANNEL1;SLOPE POSITIVE");
 envoi(_o->dev, ":TRIGGER:EDGE:LEVEL CHANNEL1,0.0");
 envoi(_o->dev, ":TRIGGER:SWEEP SINGLE");
 return true;
}

void scope_backend_agilent::shutdown()
{char cmd[256];
 if (!_o || !_o->dev) return;
 for (int c=1;c<=_o->_channels;c++)
   {sprintf(cmd,"CHANNEL%d:DISPLAY ON",c); 
    envoi(_o->dev, cmd);
   }
 envoi(_o->dev, ":TRIGGER:SWEEP AUTO");
}

bool scope_backend_agilent::apply_range(float range)
{if (!_o || !_o->dev) return false;
 char cmd[256];
 for (int c=1;c<=_o->_channels;c++)
   {sprintf(cmd, ":CHANNEL%d:RANGE %e;OFFSET 0.0", c, range);
    envoi(_o->dev, cmd);
   }
 return true;
}

bool scope_backend_agilent::apply_rate(float rate)
{char cmd[256];
 if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(_o->_duration * rate);
 _o->ensure_buffers();
 sprintf(cmd, ":ACQUIRE:MODE RTIME;AVERAGE OFF;SRATE %e;POINTS %d", rate, _o->_sample_size);
 envoi(_o->dev, cmd);
 return true;
}

bool scope_backend_agilent::apply_duration(float dur)
{char cmd[256];
 if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(dur * _o->_rate);
 _o->ensure_buffers();
 sprintf(cmd, ":TIMEBASE:REFERENCE LEFT;POSITION 0;RANGE %e", dur);
 envoi(_o->dev, cmd);
 return true;
}

bool scope_backend_agilent::acquire()
{char cmd[256],cmdtmp[256];
 if (!_o || !_o->dev) return false;
 sprintf(cmd,":DIGITIZE ");
 for (int c=1;c<=_o->_channels;c++)
   {sprintf(cmdtmp,"CHANNEL%d,",c);
    strcat(cmd, cmdtmp);
   }  // create string as :DIGITIZE CHANNEL1,CHANNEL2\n
 cmd[strlen(cmd)-1]='\n'; // replace trailing , with CR
 envoi(_o->dev, cmd);
 const int want = (2 * _o->_sample_size + 64);
 for (int c=1;c<=_o->_channels;c++)
   {sprintf(cmd,":WAVEFORM:SOURCE CHANNEL%d\n",c);
    envoi(_o->dev, cmd);
    envoi(_o->dev, ":WAVEFORM:FORMAT WORD;BYTEORDER LSBFIRST\n");
    vxi11_send_and_receive(_o->dev, (char*)"WAVEFORM:DATA?\n", _o->_data_buffer, want, 100 * VXI11_READ_TIMEOUT);
    if (_o->_data_buffer[0] != '#') 
       {printf("Agilent: error in trace header (CH%d)\n",c); return false; 
       }
    int nd = _o->_data_buffer[1] - '0';
    int hdr = 2 + nd;
    for (int k = 0; k < _o->_sample_size; k++) 
     {const int idx = hdr + 2 * k;
      int16_t s = *(int16_t*)(&_o->_data_buffer[idx]);
      if (c==1) _o->_tab1[k] = (float)s / 65536.0f;
      if (c==2) _o->_tab1[k] = (float)s / 65536.0f;
      if (c==3) _o->_tab1[k] = (float)s / 65536.0f;
      if (c==4) _o->_tab1[k] = (float)s / 65536.0f;
     }
   }
  return true;
}
} // namespace oscilloscope
} // namespace gr
