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

#include "oscilloscope_impl_tcpip.h"
#include <algorithm>
#include <cstring>
#include <unistd.h>

namespace gr {
namespace oscilloscope {

int scope_backend_tcpip::relit(int fd, char* buf, int total)
{int got = 0;
 while (got < total)
   {int r = (int)read(fd, buf + got, (size_t)(total - got));
    if (r <= 0) return r;
    got += r;
   }
  return got;
}

bool scope_backend_tcpip::init()
{sockaddr_in a{};
 long longueur;
 _o->sockfd = socket(AF_INET, SOCK_STREAM, 0);
 if (_o->sockfd < 0) return false;
 a.sin_family = AF_INET;
 a.sin_addr.s_addr = inet_addr(_o->_device_ip);
 a.sin_port = htons(9999);
 bzero(&(a.sin_zero),8);
 if (connect(_o->sockfd, (struct sockaddr*)&a, sizeof(a)) != 0)
   {close(_o->sockfd);
    _o->sockfd = -1;
    return false;
   }
 printf("[TCPIP] OK\n"); fflush(stdout);
 longueur=htonl(_o->_channels);
 write(_o->sockfd,&longueur,sizeof(int)); // number of channels
 return true;
}

void scope_backend_tcpip::shutdown()
{int val=htonl(-1);
 write(_o->sockfd,&val,sizeof(int));
 close(_o->sockfd);
}

bool scope_backend_tcpip::apply_range(float range)
{return true;
}

bool scope_backend_tcpip::apply_rate(float rate)
{_o->_sample_size = 8192;
 _o->ensure_buffers();
 return true;
}

bool scope_backend_tcpip::apply_duration(float dur)
{_o->_sample_size = 8192;
 _o->ensure_buffers();
 return true;
}

bool scope_backend_tcpip::acquire()
{long val=htonl(_o->_sample_size);  // TCP server knows how many channels are requested
 if (_o->sockfd < 0) return false;
#ifdef mydebug
 printf("%d items requested\n",_o->_sample_size);
#endif
 write(_o->sockfd,&val,sizeof(long int));
 read(_o->sockfd, _o->_tab1, sizeof(float)*_o->_sample_size);
 if (_o->_channels>=2) read(_o->sockfd, _o->_tab2, sizeof(float)*_o->_sample_size);
 if (_o->_channels>=3) read(_o->sockfd, _o->_tab3, sizeof(float)*_o->_sample_size);
 if (_o->_channels>=4) read(_o->sockfd, _o->_tab4, sizeof(float)*_o->_sample_size);
 return true;
}
} // namespace oscilloscope
} // namespace gr
