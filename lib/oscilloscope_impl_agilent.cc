// lib/oscilloscope_impl_agilent.cc
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Agilent oscilloscope backend.
 *   Implements waveform acquisition over VXI-11 using SCPI.
 */

#include "oscilloscope_impl_agilent.h"
#include "./vxi11/library/vxi11_user.h"

#define mydebug

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

namespace gr {
namespace oscilloscope {

int scope_backend_agilent::relit(VXI11_CLINK* clink, char* buffer, int buffer_length)
{int ret = vxi11_receive(clink, buffer, buffer_length);
 if (ret > 0) buffer[ret - 1] = 0;
 return ret;
}

void scope_backend_agilent::envoi(VXI11_CLINK* clink, const char* s)
{vxi11_send(clink, (char*)s, (int)std::strlen(s));
 usleep(5000);
}

bool scope_backend_agilent::init()
{char* device_name = nullptr;
 char b[256];       // Right Click on sine wave on top of display, Setup Acquisition
                    //             and see SamplingRate/MemDepth
 _o->dev = nullptr; // "169.254.202.240" for Agilent 54855DSO
 if (vxi11_open_device(&_o->dev, _o->_device_ip, device_name) != 0)
   {printf("[Agilent] error opening vxi11\n"); fflush(stdout);
    return false;
   }
 if (!_o || !_o->dev) return false;
 envoi(_o->dev, "*IDN?\n");
 relit(_o->dev, b, sizeof(b));
 printf("[Agilent] IDN: %s\n", b); fflush(stdout);
 envoi(_o->dev, "*CLS\n");
 envoi(_o->dev, ":SYSTEM:HEADER OFF\n");
 envoi(_o->dev, ":TRIGGER:EDGE:SOURCE CHANNEL1;SLOPE POSITIVE\n");
 envoi(_o->dev, ":TRIGGER:EDGE:LEVEL CHANNEL1,0.0\n");
 envoi(_o->dev, ":TRIGGER:SWEEP SINGLE\n");
#ifdef mydebug
 printf("[Agilent] init OK\n");
#endif
 return true;
}

void scope_backend_agilent::shutdown()
{char cmd[256];
 if (!_o || !_o->dev) return;
 for (int c=1;c<=_o->_channels;c++)
   {sprintf(cmd,":CHANNEL%d:DISPLAY ON\n",c);
    envoi(_o->dev, cmd);
   }
 envoi(_o->dev, ":TRIGGER:SWEEP AUTO\n");
}

bool scope_backend_agilent::apply_range(float range)
{if (!_o || !_o->dev) return false;
 char cmd[256];
#ifdef mydebug
 char buf[256];
 bzero(buf,256);
#endif
 for (int c=1;c<=_o->_channels;c++)
   {sprintf(cmd, ":CHANNEL%d:RANGE %e;OFFSET 0.0\n", c, range);
    envoi(_o->dev, cmd);
#ifdef mydebug
    sprintf(cmd, ":CHANNEL%d:RANGE?\n", c);
    vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
    printf("[Agilent] range %e -> %s", range, buf);
#endif
   }
 return true;
}

bool scope_backend_agilent::apply_rate(float rate)
{char cmd[256];
#ifdef mydebug
 char buf[256];
 bzero(buf,256);
#endif
 if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(_o->_duration * rate);
 _o->_rate=rate;
 _o->ensure_buffers();
 sprintf(cmd, ":ACQUIRE:MODE RTIME\n");
 envoi(_o->dev, cmd);
 sprintf(cmd, ":ACQUIRE:AVERAGE OFF\n");
 envoi(_o->dev, cmd);
 sprintf(cmd, ":ACQUIRE:SRATE:ANAL %e\n", rate);
 envoi(_o->dev, cmd);
#ifdef mydebug
 sprintf(cmd, ":ACQUIRE:SRATE:ANAL?\n");
 vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
 printf("[Agilent] rate %e -> %s",rate,buf);fflush(stdout);
#endif
 sprintf(cmd, ":ACQ:POIN:ANAL %d\n", _o->_sample_size);
 envoi(_o->dev, cmd);
#ifdef mydebug
 bzero(buf,256);
 sprintf(cmd, ":ACQ:POIN:ANAL?\n");
 vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
 printf("[Agilent] size %d -> %s",_o->_sample_size, buf);fflush(stdout);
#endif
 return true;
}

bool scope_backend_agilent::apply_duration(float dur)
{char cmd[256];
#ifdef mydebug
 char buf[256];
 bzero(buf,256);
#endif
 if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(dur * _o->_rate);
 _o->_duration=dur;
 _o->ensure_buffers();
 sprintf(cmd, ":TIMEBASE:REFERENCE LEFT\n");
 envoi(_o->dev, cmd);
 sprintf(cmd, ":TIMEBASE:POSITION 0\n");
 envoi(_o->dev, cmd);
 sprintf(cmd, ":TIMEBASE:RANGE %e\n", dur);
 envoi(_o->dev, cmd);
#ifdef mydebug
 sprintf(cmd, ":TIMEBASE:RANGE?");
 vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
 printf("[Agilent] duration %e -> %s",dur,buf);
 bzero(buf,256);
 sprintf(cmd, ":ACQ:POIN:ANAL?\n");
 vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
 printf("[Agilent] size %d -> %s",_o->_sample_size, buf);fflush(stdout);
#endif
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
   {sprintf(cmd,":WAV:SOUR CHAN%d\n",c);
    envoi(_o->dev, cmd);
    // envoi(_o->dev, ":WAVEFORM:FORMAT WORD;BYTEORDER LSBFIRST\n");
    envoi(_o->dev, ":WAV:FORM WORD\n");
    envoi(_o->dev, ":WAV:BYT LSBF\n");
    vxi11_send_and_receive(_o->dev, (char*)":WAV:DATA?\n", _o->_data_buffer, want, 100 * VXI11_READ_TIMEOUT);
#ifdef mydebug
//    printf("[Agilent] measurement request\n");fflush(stdout);
#endif
    if (_o->_data_buffer[0] != '#')
       {printf("[Agilent] error in trace header (CH%d)\n",c); fflush(stdout);
        return false;
       }
    int offset = _o->_data_buffer[1] - '0';
#ifdef mydebug
//    printf("[Agilent] size=%d\n",_o->_sample_size);fflush(stdout);
#endif
    for (int k=0; k<_o->_sample_size; k++)
     {int16_t s = *(int16_t*)(&_o->_data_buffer[2+offset+2*k]);
#ifdef mydebug
//      if (k<10) {printf(" %f ",(float)s/32768.);fflush(stdout);}
#endif
      if (c==1) _o->_tab1[k] = (float)s / 32768.0f;
      if (c==2) _o->_tab2[k] = (float)s / 32768.0f;
      if (c==3) _o->_tab3[k] = (float)s / 32768.0f;
      if (c==4) _o->_tab4[k] = (float)s / 32768.0f;
     }
   }
  return true;
}
} // namespace oscilloscope
} // namespace gr
