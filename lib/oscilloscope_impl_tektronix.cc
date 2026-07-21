// lib/oscilloscope_impl_tektronix.cc
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Tektronix oscilloscope backend.
 *   Implements waveform acquisition over raw TCP/IP using SCPI.
 *   Communication principle:
 *   - Direct TCP socket (port 4000)
 *   - Explicit single acquisition (STOPAFTER SEQUENCE)
 *   - Binary waveform transfer (CURVE?)
 */

#include "oscilloscope_impl_tektronix.h"
#include <algorithm>
#include <cstring>
#include <unistd.h>

namespace gr {
namespace oscilloscope {

int scope_backend_tektronix::relit(int fd, char* buf, int total)
{int got = 0;
 while (got < total)
   {int r = (int)read(fd, buf + got, (size_t)(total - got));
    if (r <= 0) return r;
    got += r;
   }
  return got;
}

int scope_backend_tektronix::tek_query_int(int fd, const char* cmd)
{char buf[256];
 write(fd, cmd, strlen(cmd)); usleep(50000);
 memset(buf, 0, sizeof(buf));
 int r = (int)read(fd, buf, sizeof(buf) - 1);
 return (r > 0) ? atoi(buf) : -1;
}

float scope_backend_tektronix::tek_query_float(int fd, const char* cmd)
{char buf[256];
 write(fd, cmd, strlen(cmd));
 usleep(50000);
 memset(buf, 0, sizeof(buf));
 int r = (int)read(fd, buf, sizeof(buf) - 1);
 return (r > 0) ? (float)atof(buf) : 0.0f;
}

bool scope_backend_tektronix::init()
{sockaddr_in a{};
 char buf[256];
 _o->sockfd = socket(AF_INET, SOCK_STREAM, 0); // Tektronix SCPI server (raw TCP)
 if (_o->sockfd < 0) return false;
 a.sin_family = AF_INET;
 a.sin_addr.s_addr = inet_addr(_o->_device_ip);
 a.sin_port = htons(4000);
 bzero(&(a.sin_zero),8);
 if (connect(_o->sockfd, (struct sockaddr*)&a, sizeof(a)) != 0)
   {close(_o->sockfd);
    _o->sockfd = -1;
    return false;
   }
 for (int c = 1; c <= _o->_channels; c++)
   {sprintf(buf, "SELECT:CH%d ON\n",c);
    write(_o->sockfd, buf, strlen(buf)); usleep(20000);
   }
 for (int c =_o->_channels+1; c<=4; c++)
   {sprintf(buf, "SELECT:CH%d OFF\n",c);
    write(_o->sockfd, buf, strlen(buf)); usleep(20000);
   }
 const char* idn = "*IDN?\n";
 write(_o->sockfd, idn, strlen(idn)); usleep(50000);
 memset(buf, 0, sizeof(buf));
 read(_o->sockfd, buf, sizeof(buf) - 1);
 printf("[Tektro] IDN: %s\n", buf); fflush(stdout);
 return true;
}

void scope_backend_tektronix::shutdown()
{close(_o->sockfd);
}

bool scope_backend_tektronix::apply_range(float range)
{float scale = range / 10.0f; // Vertical scale = full range / 10 div
 char buf[64];
 for (int c = 1; c <= _o->_channels; c++)
   {snprintf(buf, sizeof(buf), "CH%d:SCALE %e\n", c, scale);
    write(_o->sockfd, buf, strlen(buf)); usleep(50000);
  }
  return true;
}

bool scope_backend_tektronix::apply_rate(float rate)
{int n = (int)(_o->_duration * rate); // length=samp_rate*duration
 char b[128];
 if (n < 256) n = 256;
 _o->_sample_size = n;
 _o->ensure_buffers();
 sprintf(b, "HORIZONTAL:MODE:SAMPLERATE %e\n", rate);
 write(_o->sockfd, b, strlen(b)); usleep(50000);
 sprintf(b, "HORIZONTAL:RECORDLENGTH %d\n", _o->_sample_size);
 write(_o->sockfd, b, strlen(b)); usleep(50000);
 int rl = tek_query_int(_o->sockfd, "HORIZONTAL:RECORDLENGTH?\n");
 if (rl > 0)
   {_o->_sample_size = rl;
    _o->ensure_buffers();
   }
 return true;
}

bool scope_backend_tektronix::apply_duration(float dur)
{int n = (int)(dur * _o->_rate);
 char b[128];
 if (n < 256) n = 256;
 _o->_sample_size = n;
 _o->ensure_buffers();
 sprintf(b, "HORIZONTAL:RECORDLENGTH %d\n", _o->_sample_size);
 write(_o->sockfd, b, strlen(b)); usleep(50000);
 int rl = tek_query_int(_o->sockfd, "HORIZONTAL:RECORDLENGTH?\n");
 if (rl > 0)
   {_o->_sample_size = rl;
    _o->ensure_buffers();
   }
 return true;
}

bool scope_backend_tektronix::acquire()
{char buffer[256];
 if (_o->sockfd < 0) return false;
 sprintf(buffer, "HEADER OFF\n");
 write(_o->sockfd, buffer, strlen(buffer)); usleep(50000);
 sprintf(buffer, "ACQUIRE:STOPAFTER SEQUENCE\n");
 write(_o->sockfd, buffer, strlen(buffer)); usleep(50000);
 sprintf(buffer, "ACQUIRE:STATE RUN\n");
 write(_o->sockfd, buffer, strlen(buffer)); usleep(50000);
 sprintf(buffer, "*OPC?\n");
 write(_o->sockfd, buffer, strlen(buffer)); usleep(50000);
 memset(buffer, 0, sizeof(buffer));
 read(_o->sockfd, buffer, sizeof(buffer));
 int nr_pt = tek_query_int(_o->sockfd, "HORIZONTAL:RECORDLENGTH?\n");
 if (nr_pt <= 0) {nr_pt = _o->_sample_size;printf("NR < 0 !");fflush(stdout);}
// if (nr_pt > _o->_sample_size) {
//    _o->_sample_size = nr_pt;
//    _o->ensure_buffers();
//  }
 for (int chan = 1; chan <= _o->_channels; chan++)
   {sprintf(buffer, "DATA:SOURCE CH%d\n", chan);
    write(_o->sockfd, buffer, strlen(buffer)); usleep(50000);
    sprintf(buffer, "DATA:ENCDG RIBinary\n"); // Big-endian signed binary
    write(_o->sockfd, buffer, strlen(buffer)); usleep(50000);
    sprintf(buffer, "DATA:WIDTH 2\n");
    write(_o->sockfd, buffer, strlen(buffer)); usleep(50000);
    float ymult = tek_query_float(_o->sockfd, "WFMOUTPRE:YMULT?\n");
    float yoff  = tek_query_float(_o->sockfd, "WFMOUTPRE:YOFF?\n");
    float yzero = tek_query_float(_o->sockfd, "WFMOUTPRE:YZERO?\n");
    ymult = ymult / 1000000.0f;
    sprintf(buffer, "DATA:START 1\n");
    write(_o->sockfd, buffer, strlen(buffer)); usleep(50000);
    sprintf(buffer, "DATA:STOP %d\n", nr_pt);
    write(_o->sockfd, buffer, strlen(buffer)); usleep(50000);
    sprintf(buffer, "CURVE?\n");
    write(_o->sockfd, buffer, strlen(buffer)); usleep(50000);
    char h[16]; // SCPI binary block header
    if (relit(_o->sockfd, h, 2) != 2 || h[0] != '#') return false;
    int nd = h[1] - '0';
    if (nd <= 0 || nd > 9) return false;
    if (relit(_o->sockfd, h, nd) != nd) return false;
    h[nd] = 0;
    int data_bytes = atoi(h);
    if (data_bytes <= 0) return false;
    if (data_bytes > (2 * _o->_sample_size))
       {_o->_data_buffer = (char*)realloc(_o->_data_buffer, (size_t)data_bytes + 512);
        if (!_o->_data_buffer) return false;
       }

    if (relit(_o->sockfd, _o->_data_buffer, data_bytes) != data_bytes) return false;
    char nl;
    read(_o->sockfd, &nl, 1); // trailing newline
    const unsigned char* u8 = (const unsigned char*)_o->_data_buffer;
    long kmax = std::min((long)nr_pt, (long)_o->_sample_size);
    for (long k = 0; k < kmax; k++)
      {uint16_t u16 = ((uint16_t)u8[2 * k] << 8) | (uint16_t)u8[2 * k + 1];
       int16_t s16 = (int16_t)u16;
       float volts = (((float)s16 - yoff) * ymult) + yzero;
       if (chan == 1) _o->_tab1[k] = volts;
       if (chan == 2) _o->_tab2[k] = volts;
       if (chan == 3) _o->_tab3[k] = volts;
       if (chan == 4) _o->_tab4[k] = volts;
      }
    for (long k = kmax; k < _o->_sample_size; k++) // pad remaining samples
      {if (chan == 1) _o->_tab1[k] = 0.0f;
       if (chan == 2) _o->_tab2[k] = 0.0f;
       if (chan == 3) _o->_tab3[k] = 0.0f;
       if (chan == 4) _o->_tab4[k] = 0.0f;
      }
   }
   return true;
}
} // namespace oscilloscope
} // namespace gr
