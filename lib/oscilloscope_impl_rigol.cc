// TODO: one Remote Cmd Error at startup needs to be identified and corrected

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

//#define mydebug 1

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
 if (vxi11_open_device(&_o->dev, _o->_device_ip, device_name) != 0)
   {printf("[rigol] error opening vxi11\n");
    fflush(stdout); return false;
   }
 if (!_o || !_o->dev) return false;
 envoi(_o->dev, "*IDN?");
 relit(_o->dev, b, sizeof(b));
 printf("[rigol] IDN: %s\n", b); fflush(stdout);
 envoi(_o->dev, "*CLS");
 envoi(_o->dev, ":WAV:FORM BYTE\n");usleep(5000);
 envoi(_o->dev, ":WAV:MODE RAW\n");usleep(5000);
 envoi(_o->dev, ":WAV:POIN RAW\n");usleep(5000);
 return true;
}

void scope_backend_rigol::shutdown()
{if (_o->dev) envoi(_o->dev, ":RUN");
}

bool scope_backend_rigol::apply_range(float range)
{if (!_o || !_o->dev) return false;
 char cmd[256];
#ifdef mydebug
 char buf[256];
 bzero(buf,256);
#endif
 for (int c=1;c<=_o->_channels;c++)
   {sprintf(cmd, ":CHANNEL%d:DISP ON\n", c);
    envoi(_o->dev, cmd);
    sprintf(cmd, ":CHANNEL%d:SCAL %e;OFFSET 0.0\n", c, range/10);
    envoi(_o->dev, cmd);
#ifdef mydebug
    sprintf(cmd, ":CHANNEL%d:SCAL?\n", c);
    vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
    printf("[rigol] range %e -> %s", range, buf);
#endif
   }
 return true;
}

bool scope_backend_rigol::apply_rate(float rate)
{char cmd[256];
#ifdef mydebug
 char buf[256];
 bzero(buf,256);
#endif
 if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(_o->_duration * rate);
 _o->_rate=rate;
 _o->ensure_buffers();
 sprintf(cmd, ":ACQ:TYPE NORMAL\n"); // no average
 envoi(_o->dev, cmd);
 sprintf(cmd, ":ACQ:SRATE %f\n", rate);
 envoi(_o->dev, cmd);
#ifdef mydebug
 sprintf(cmd, ":ACQ:SRATE?\n");
 vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
 printf("[rigol] rate %e -> %s", rate, buf);
#endif
 sprintf(cmd, ":ACQ:MDEP %d\n", _o->_sample_size);
 envoi(_o->dev, cmd);
#ifdef mydebug
 sprintf(cmd, ":ACQ:MDEP?\n");
 vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
 printf("[rigol] memory depth %d -> %s",_o->_sample_size, buf);fflush(stdout);
#endif
 return true;
}

bool scope_backend_rigol::apply_duration(float dur)
{char cmd[256];
#ifdef mydebug
 char buf[256];
 bzero(buf,256);
#endif
 if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(dur * _o->_rate);
 _o->_duration=dur;
 _o->ensure_buffers();
 sprintf(cmd, ":TIM:HREF LB\n");
 envoi(_o->dev, cmd);
 sprintf(cmd, ":TIM:HREF:POS 0\n");
 envoi(_o->dev, cmd);
 sprintf(cmd, ":TIMEBASE:SCAL %e\n", dur/10.); // 10 divisions
 envoi(_o->dev, cmd);
#ifdef mydebug
 sprintf(cmd, ":TIMEBASE:SCAL?");
 vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
 printf("[rigol] duration %e -> %s",dur,buf);
 bzero(buf,256);
 sprintf(cmd, ":ACQ:MDEP?\n");
 vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
 printf("[rigol] size %d -> %s",_o->_sample_size, buf);fflush(stdout);
#endif
 return true;
}

bool scope_backend_rigol::acquire()
{if (!_o->dev) return false;

 char buffer[256];
 char prebuf[512];
 int pts=0;
 do {
   envoi(_o->dev, "*CLS");usleep(5000);
   envoi(_o->dev, ":SING");usleep(5000);
   envoi(_o->dev, "*OPC?");
   relit(_o->dev, buffer, sizeof(buffer));
   for (int chan = 1; chan <= _o->_channels; chan++)
     {sprintf(buffer, ":WAV:SOUR CHAN%d", chan);usleep(5000);
      envoi(_o->dev, buffer);
      memset(prebuf, 0, sizeof(prebuf));
      vxi11_send_and_receive(_o->dev, (const char*)":WAV:PRE?", prebuf, sizeof(prebuf), 100 * VXI11_READ_TIMEOUT);
  #ifdef mydebug
      printf("%s -> ",prebuf);
  #endif
      char* p = prebuf;
      while (*p && ((*p < '0') || (*p > '9')) && (*p != '-')) p++;
      int idx = 0;
      char* tok = strtok(p, ",");
      double yincr = 1.0;
      double yorg  = 0.0;
      double yref  = 0.0;
      while (tok)
        {if (idx == 2)
           {pts = atoi(tok);
            if (pts!=_o->_sample_size)
              {printf("[rigol] error pts!=sample_size\n");fflush(stdout);}
           }
         if (idx == 7) yincr = atof(tok);
         if (idx == 8) yorg  = atof(tok);
         if (idx == 9) yref  = atof(tok);
         idx++;
         tok = strtok(nullptr, ",");
        }
      if (yref!=128)
        {printf("[rigol] error YREF!=128\n");fflush(stdout);}
  #ifdef mydebug
        printf("yinc=%f yorg=%f yref=%f\n",yincr,yorg,yref);
  #endif
      envoi(_o->dev, ":WAV:STAR 1");usleep(5000);
      snprintf(buffer, sizeof(buffer), ":WAV:STOP %d", _o->_sample_size);;usleep(5000);
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
    if ((_o->_tab1[0]==0.) && (_o->_tab1[_o->_sample_size-1]==0.))
       printf("[rigol] wrong measurement: repeat\n");
  } while ((_o->_tab1[0]==0.) && (_o->_tab1[_o->_sample_size-1]==0.));
  return true;
}
} // namespace oscilloscope
} // namespace gr
