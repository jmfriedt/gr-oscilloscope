/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Siglent SDS2000X Plus oscilloscope backend.
 * Implements waveform acquisition over VXI-11 using SCPI commands.
 * Communication principle:
 * - Explicit single acquisition (:TRIG:MODE SING), completion polled with :TRIG:STAT?
 * - Vertical scaling query (:CHAN<n>:SCAL? and :CHAN<n>:OFFS?)
 *   (the Siglent :WAV:PRE? answer is a 346-byte *binary* WAVEDESC block, not
 *   a comma-separated ASCII string as on Rigol, so it is not parsed here)
 * - Raw waveform transfer (:WAV:DATA?) by chunks of at most SIGLENT_CHUNK points
 * see https://siglentna.com/wp-content/uploads/dlm_uploads/2017/10/ProgrammingGuide_forSDS-1-1.pdf
 */

#include "oscilloscope_impl_siglent.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <strings.h>
#include <unistd.h>

// #define mydebug 1

// maximum number of points requested in one :WAV:DATA? call
#define SIGLENT_CHUNK 1000000
// 8-bit waveform: 25 codes per vertical division (volts = code*vdiv/25 - offset)
#define SIGLENT_CODE_PER_DIV 25.0

namespace gr {
namespace oscilloscope {

static double parse_num(const char* s)
{while (*s && ((*s<'0') || (*s>'9')) && (*s!='-') && (*s!='+') && (*s!='.')) s++;
 return atof(s);
}

int scope_backend_siglent::relit(VXI11_CLINK* clink, char* buffer, int buffer_length)
{int ret = vxi11_receive(clink, buffer, buffer_length);
 if (ret > 0) buffer[ret - 1] = 0;
 return ret;
}

void scope_backend_siglent::envoi(VXI11_CLINK* clink, const char* buffer)
{vxi11_send(clink, (char*)buffer, (int)strlen(buffer));
}

bool scope_backend_siglent::init()
{char* device_name = nullptr;
 char b[256];
 _o->dev = nullptr;
 if (vxi11_open_device(&_o->dev, _o->_device_ip, device_name) != 0)
    {printf("[siglent] error opening vxi11\n");
     fflush(stdout); return false;
    }
 if (!_o || !_o->dev) return false;
 envoi(_o->dev, "*IDN?");
 relit(_o->dev, b, sizeof(b));
 printf("[siglent] IDN: %s\n", b); fflush(stdout);
 envoi(_o->dev, "*CLS");
 envoi(_o->dev, ":WAV:WIDT BYTE\n");usleep(5000); // 8-bit samples
 envoi(_o->dev, ":WAV:INT 1\n");usleep(5000);     // no decimation, every point
 return true;
}

void scope_backend_siglent::shutdown()
{if (_o->dev)
    {envoi(_o->dev, ":TRIG:MODE AUTO\n");usleep(5000);
     envoi(_o->dev, ":TRIG:RUN\n");
    }
}

bool scope_backend_siglent::apply_range(float range)
{if (!_o || !_o->dev) return false;
 char cmd[256];
#ifdef mydebug
 char buf[256];
 bzero(buf,256);
#endif
 for (int c=1;c<=_o->_channels;c++)
    {sprintf(cmd, ":CHAN%d:SWIT ON\n", c);          envoi(_o->dev, cmd);
     sprintf(cmd, ":CHAN%d:SCAL %e\n", c, range/8); envoi(_o->dev, cmd); // 8 vertical divisions
     sprintf(cmd, ":CHAN%d:OFFS 0.0\n", c);
     envoi(_o->dev, cmd);
#ifdef mydebug
     sprintf(cmd, ":CHAN%d:SCAL?\n", c);
     vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
     printf("[siglent] range %e -> %s", range, buf);
#endif
    }
 return true;
}

bool scope_backend_siglent::apply_rate(float rate)
{char cmd[256];
#ifdef mydebug
 char buf[256];
 bzero(buf,256);
#endif
 if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(_o->_duration * rate);
 _o->_rate=rate;
 _o->ensure_buffers();
 sprintf(cmd, ":ACQ:TYPE NORM\n"); // no average
 envoi(_o->dev, cmd);
// sample rate cannot be set directly: it results from timebase and memory depth (:ACQ:SRAT? query only)
#ifdef mydebug
 sprintf(cmd, ":ACQ:SRAT?\n");
 vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
 printf("[siglent] rate %e -> %s/div", rate, buf);
#endif
 char mbuf[256];
 sprintf(cmd, ":ACQ:MDEP %d\n", _o->_sample_size);
 envoi(_o->dev, cmd);usleep(20000);
 memset(mbuf, 0, sizeof(mbuf));
 vxi11_send_and_receive(_o->dev, (const char*)":ACQ:MDEP?", mbuf, sizeof(mbuf), 100 * VXI11_READ_TIMEOUT);
 printf("[siglent] memory depth for %d points -> %s\n", _o->_sample_size, mbuf);
 envoi(_o->dev, "*CLS");usleep(5000); // clear the errors of the rejected candidates
 fflush(stdout);
 return true;
}

bool scope_backend_siglent::apply_duration(float dur)
{char cmd[256];
#ifdef mydebug
 char buf[256];
 bzero(buf,256);
#endif
 if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(dur * _o->_rate);
 _o->_duration=dur;
 _o->ensure_buffers();
 // no HREF on Siglent: TIM:DEL is the time of the screen centre relative to the trigger.
 // 5 divisions (dur/2) -> left border of the screen is at the trigger instant (t=0)
 sprintf(cmd, ":TIM:DEL %e\n", dur/2.);
 envoi(_o->dev, cmd);
 sprintf(cmd, ":TIM:SCAL %e\n", dur/14.); // SDS2000X Plus: 14 horizontal divisions
 envoi(_o->dev, cmd);
#ifdef mydebug
 sprintf(cmd, ":TIM:SCAL?");
 vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
 printf("[siglent] duration %e -> %s",dur,buf);
 bzero(buf,256);
 sprintf(cmd, ":ACQ:MDEP?\n");
 vxi11_send_and_receive(_o->dev, cmd, buf, 256, 100 * VXI11_READ_TIMEOUT);
 printf("[siglent] size %d -> %s",_o->_sample_size, buf);fflush(stdout);
#endif
 return true;
}

bool scope_backend_siglent::acquire()
{if (!_o->dev) return false;
 char buffer[256];
 char cmd[256];
 char reply[256];
 do {
      envoi(_o->dev, "*CLS");usleep(5000);
      envoi(_o->dev, ":TRIG:MODE SING");usleep(50000);  // arm single acquisition
      // wait for the end of the acquisition: status goes back to "Stop"
      int tries = 0;
      bool finished = false;
      while (!finished && (tries < 3000))
         {memset(reply, 0, sizeof(reply));
          vxi11_send_and_receive(_o->dev, (const char*)":TRIG:STAT?", reply, sizeof(reply), 100 * VXI11_READ_TIMEOUT);
          if (strncasecmp(reply, "Stop", 4) == 0) finished = true;
          else usleep(10000);
          tries++;
         }
      if (!finished) {printf("[siglent] error: acquisition timeout\n");fflush(stdout);return false;}

      for (int chan = 1; chan <= _o->_channels; chan++)
         {sprintf(cmd, ":WAV:SOUR C%d", chan);
          envoi(_o->dev, cmd);usleep(5000);

          // vertical scale and offset, replace the Rigol preamble (yincr/yorg/yref)
          memset(reply, 0, sizeof(reply));
          sprintf(cmd, ":CHAN%d:SCAL?", chan);
          vxi11_send_and_receive(_o->dev, (const char*)cmd, reply, sizeof(reply), 100 * VXI11_READ_TIMEOUT);
          double vdiv = parse_num(reply);
          memset(reply, 0, sizeof(reply));
          sprintf(cmd, ":CHAN%d:OFFS?", chan);
          vxi11_send_and_receive(_o->dev, (const char*)cmd, reply, sizeof(reply), 100 * VXI11_READ_TIMEOUT);
          double yorg = parse_num(reply);
          double yincr = vdiv / SIGLENT_CODE_PER_DIV;  // volts per code
#ifdef mydebug
          printf("yinc=%f yorg=%f\n",yincr,yorg);
#endif
          // number of points actually available in memory
          memset(reply, 0, sizeof(reply));
          vxi11_send_and_receive(_o->dev, (const char*)":ACQ:POIN?", reply, sizeof(reply), 100 * VXI11_READ_TIMEOUT);
          int pts = (int)parse_num(reply);
          int want = _o->_sample_size;  // number of points to read
          if ((pts > 0) && (pts < want))
             {printf("[siglent] warning: %d points returned, %d wanted\n", pts, want);fflush(stdout);
              want = pts;
             }

          float* tab = nullptr;
          if (chan == 1) tab = _o->_tab1;
          if (chan == 2) tab = _o->_tab2;
          if (chan == 3) tab = _o->_tab3;
          if (chan == 4) tab = _o->_tab4;

          // read the waveform by chunks
          int done = 0;
//          while (done < want)
             {int n = want - done;
              if (n > SIGLENT_CHUNK) n = SIGLENT_CHUNK;
              sprintf(buffer, ":WAV:STAR %d", done);  // 0-based index on Siglent
              envoi(_o->dev, buffer);usleep(5000);
              sprintf(buffer, ":WAV:POIN %d", n);
              envoi(_o->dev, buffer);usleep(5000);
              int rcv = (int)vxi11_send_and_receive(_o->dev, (const char*)":WAV:DATA?", _o->_data_buffer, (n + 256), 300 * VXI11_READ_TIMEOUT);
              if (_o->_data_buffer[0] != '#') {
                 printf("[siglent] error in trace header\n"); fflush(stdout);
                 return false;
              }
              int nd = _o->_data_buffer[1] - '0';
              int ln = 0;
              for (int k=0; k<nd; k++) ln=10*ln+(_o->_data_buffer[2+k]-'0');
              int off = 2 + nd;
              signed char* u = (signed char*)(&_o->_data_buffer[off]); // signed 8-bit codes
              if ((rcv > 0) && (ln > rcv - off)) ln = rcv - off; // truncated transfer: keep what was really received
              if (ln > n) ln = n;
              if (ln <= 0) break;
              if (ln < n) {printf("[siglent] chunk: %d points received, %d requested\n", ln, n);fflush(stdout);}
              if (tab)
                 for (int k = 0; k < ln; k++)
                    tab[done + k] = (float)((double)u[k] * yincr - yorg);
              done += ln;
             }
          if (done != _o->_sample_size)
             {printf("[siglent] chan %d: %d points actually read, %d expected (rest set to 0)\n", chan, done, _o->_sample_size);fflush(stdout);}
          if (tab)
             for (int k = done; k < _o->_sample_size; k++) tab[k] = 0.0f;
         }
      if ((_o->_tab1[0]==0.) && (_o->_tab1[_o->_sample_size-1]==0.))
         printf("[siglent] wrong measurement: repeat\n");
    } while ((_o->_tab1[0]==0.) && (_o->_tab1[_o->_sample_size-1]==0.));
 return true;
}

} // namespace oscilloscope
} // namespace gr
