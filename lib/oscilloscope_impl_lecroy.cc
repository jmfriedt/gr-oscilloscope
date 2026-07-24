/* -*- c++ -*- */
/*
 *   LeCroy oscilloscope backend.
 */

#include "oscilloscope_impl_lecroy.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <locale.h>

#define mydebug

#define	MAX_ST	(1512000*2)	/* maximum message string. no picked from thin air */

namespace gr {
namespace oscilloscope {

int scope_backend_lecroy::relit(VXI11_CLINK* clink, char* buffer, int buffer_length)
{int ret = vxi11_receive(clink, buffer, buffer_length);
 if (ret > 0) buffer[ret - 1] = 0;
 return ret;
}

void scope_backend_lecroy::envoi(VXI11_CLINK* clink, const char* s)
{vxi11_send(clink, (char*)s, (int)std::strlen(s));
}

bool scope_backend_lecroy::init()
{char* device_name = nullptr;
 char buf[256];
 _o->dev = nullptr;
 if (vxi11_open_device(&_o->dev, _o->_device_ip, device_name) != 0)
   {printf("[LeCroy] error opening vxi11\n"); fflush(stdout);
    return false;
   }
 if (!_o || !_o->dev) return false;
 envoi(_o->dev, "*IDN?");
 relit(_o->dev, buf, sizeof(buf));
 printf("[LeCroy] IDN: %s\n", buf); fflush(stdout);
 envoi(_o->dev, "*CLS");
 envoi(_o->dev, "AUTO_CALIBRATE OFF\n"); // no auto cal
 envoi(_o->dev, "CHDR OFF\n");           // no header
 envoi(_o->dev, "CFMT OFF,SHORT,BIN\n"); // binary short
 envoi(_o->dev, "TRMD SINGLE\n");
 for (int c=1;c<=_o->_channels;c++)
    {sprintf(buf,"C%d:TRA ON\n",c);envoi(_o->dev,buf);
     sprintf(buf,"C%d:TRACE?\n",c);envoi(_o->dev,buf);
     relit(_o->dev,buf,256);
     printf("[LeCroy] C%d: %s\n",c,buf);
    }
 return true;
}

void scope_backend_lecroy::shutdown()
{
}

bool scope_backend_lecroy::apply_range(float range)
{char buf[128];
 if (!_o || !_o->dev) return false;
 for (int ch = 1; ch <= _o->_channels; ch++)
   {sprintf(buf, "C%d:VDIV %f\n", ch, range/10.);
    envoi(_o->dev, buf);
  }
  return true;
}

bool scope_backend_lecroy::apply_rate(float rate)
{char buf[128];
 if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(_o->_duration * rate);
 _o->_rate=rate;
 _o->ensure_buffers();
 sprintf(buf, "MSIZ %d\n", _o->_sample_size*2);
 envoi(_o->dev, buf);
#ifdef mydebug
 sprintf(buf,"MSIZ?\n");envoi(_o->dev,buf);
 relit(_o->dev,buf,256);
 printf("[LeCroy] MSIZ: %s\n",buf);
#endif
/*
 sprintf(buf, "VBS 'app.Acquisition.Horizontal.SamplingRate = %f'\n", rate);
 envoi(_o->dev, buf);
#ifdef mydebug
 envoi(_o->dev, "VBS? 'return = app.Acquisition.Horizontal.SamplingRate\n'");
 relit(_o->dev,buf,256);
 printf("[LeCroy] FREQ: %s vs %f\n",buf,rate);
#endif
*/
 return true;
}

bool scope_backend_lecroy::apply_duration(float dur)
{char buf[128];
 if (!_o || !_o->dev) return false;
 _o->_sample_size = (int)(dur * _o->_rate);
 _o->_duration=dur;
 _o->ensure_buffers();
 sprintf(buf, "MSIZ %d\n", _o->_sample_size*2);
 envoi(_o->dev, buf);
#ifdef mydebug
 sprintf(buf,"MSIZ?\n");envoi(_o->dev,buf);
 relit(_o->dev,buf,256);
 printf("[LeCroy] MSIZ: %s\n",buf);
#endif
/*
 sprintf(buf, "VBS 'app.Acquisition.Horizontal.HorScale = %f'\n", dur/10.);
 envoi(_o->dev, buf);
#ifdef mydebug
 envoi(_o->dev, "VBS? 'return = app.Acquisition.Horizontal.AcquisitionDuration\n'");
 relit(_o->dev,buf,256);
 printf("[LeCroy] TDIV: %s vs %f\n",buf,dur);
#endif
*/
 return true;
}

bool scope_backend_lecroy::acquire()
{char tmp[256] = {0};
 char cmd[128];
 if (!_o || !_o->dev) return false;
 envoi(_o->dev, "CHDR OFF\n");           // no header
 envoi(_o->dev, "*CLS");
 // envoi(_o->dev, "*TRG\n");
 envoi(_o->dev, "TRMD SINGLE\n");
 envoi(_o->dev, "ARM\n");
// envoi(_o->dev, "WAIT\n");
 envoi(_o->dev, "*OPC?\n");
 relit(_o->dev, tmp, sizeof(tmp));
 for (int chan = 1; chan <= _o->_channels; chan++)
   {sprintf(cmd,"C%d:WF? DATA_ARRAY_1\n",chan);
    vxi11_send_and_receive(_o->dev, (char*)cmd, _o->_data_buffer, 2*_o->_sample_size+512, 100 * VXI11_READ_TIMEOUT);
    if ((_o->_data_buffer[0]=='D') && (_o->_data_buffer[13]=='#')) // start with DATA_ARRAY_1:#
      {int L=_o->_data_buffer[14]-'0'; // number of decimal digits to encode byte count
       int pos=14+L+348;               // 348 = length of WAVEDESC header description
#ifdef mydebug
       char tmp=_o->_data_buffer[14+L+1];
       _o->_data_buffer[14+L+1+20]=0;
       printf("%s -- %d\n",&_o->_data_buffer[13],_o->_sample_size);
       _o->_data_buffer[14+L+1]=tmp;
#endif
       for (int k = 0; k < _o->_sample_size; k++) 
         {float v = (float)(*(short*)(&_o->_data_buffer[pos+2*k])) / 65536.0f;
          if (chan == 1) _o->_tab1[k] = v; // ^^^ valid only on Intel/LE
          if (chan == 2) _o->_tab2[k] = v;
          if (chan == 3) _o->_tab3[k] = v;
          if (chan == 4) _o->_tab4[k] = v;
         }
      }
     else
       for (int k = 0; k < _o->_sample_size; k++) 
         {if (chan == 1) _o->_tab1[k] = 0; 
          if (chan == 2) _o->_tab2[k] = 0;
          if (chan == 3) _o->_tab3[k] = 0;
          if (chan == 4) _o->_tab4[k] = 0;
         }
   }
  return true;
}
} // namespace oscilloscope
} // namespace gr
