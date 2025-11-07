/* -*- c++ -*- */
/*
 * Copyright 2025 JM Friedt (jmfriedt@femto-st.fr).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

//#define VXI11
//#define mydebug
//#define rohdeschwarz

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "oscilloscope_impl.h"
#include "./vxi11/library/vxi11_user.h"

//#ifdef VXI11
int relit(VXI11_CLINK *clink,char *buffer,int buffer_length)
{int ret;
 ret=vxi11_receive(clink,buffer,buffer_length);
 buffer[ret-1]=0;
 return(ret);
}

void envoi(VXI11_CLINK *clink,char *buffer)
{
#ifdef mydebug
 printf("-> %s\n",buffer);
#endif
 vxi11_send(clink, buffer,strlen(buffer));}
//#endif

// Tektronix TCP communication functions
static int recv_all(int fd, char *buf, int total)
{int r,got = 0;
 while (got<total)
   {r=read(fd,buf+got,total-got);
    if (r<=0) return r;
    got+=r;
  }
  return got;
}

static int tek_query_int(int fd, const char *cmd)
{char b[256];
 int r;
 write(fd, cmd, strlen(cmd));
 usleep(50000);
 memset(b,0,sizeof(b));
 r=read(fd,b,sizeof(b)-1);
 return (r>0)? atoi(b) : -1;
}

static float tek_query_float(int fd, const char *cmd)
{ char b[256];
  int r;
  write(fd, cmd, strlen(cmd));
  usleep(50000);
  memset(b,0,sizeof(b));
  r=read(fd,b,sizeof(b)-1);
  return (r>0)? (float)atof(b) : 0.0f;
}

namespace gr {
  namespace oscilloscope {

    oscilloscope::sptr
    oscilloscope::make(char *ip,float range,float rate,float duration,int channels,int type)
    {
      return gnuradio::make_block_sptr<oscilloscope_impl>(ip,range,rate,duration,channels,type);
    }

    /*
     * The private constructor
     */
    oscilloscope_impl::oscilloscope_impl(char *ip,float range,float rate,float duration,int channels,int type)
      : gr::sync_block("oscilloscope",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 4, sizeof(float))), // min max channels
	      _range(range),_rate(rate),_duration(duration),_channels(channels),_type(type)
    {
//#ifdef VXI11
     char *device_name=NULL;
     char buffer[256];
     int buffer_length=256;
     int longueur;               // TCP/IP
     struct sockaddr_in adresse; // TCP/IP
     printf("%s\n",ip);fflush(stdout);
     set_type(type);  // sets _vxi11 and _type
     set_ip(ip);      // "169.254.202.240" for Agilent 54855DSO
     if (_vxi11==1)
       {
        if (vxi11_open_device(&dev,device_ip,device_name)!=0) printf("error opening\n");
           else printf("connect OK\n");
        sprintf(buffer,"*IDN?");
        envoi(dev,buffer);
        relit(dev,buffer,buffer_length);
        printf("%s\n",buffer);

// #ifdef agilent
if (_type==agilent)
{       sprintf(buffer,"*CLS"); envoi(dev,buffer);
        sprintf(buffer,"*RST"); envoi(dev,buffer);
        sprintf(buffer,":SYSTEM:HEADER OFF"); envoi(dev,buffer);
//        sprintf(buffer,":AUTOSCALE");envoi(dev,buffer); // + (sampleDuration));
        sprintf(buffer,":TRIGGER:EDGE:SOURCE CHANNEL1;SLOPE POSITIVE");envoi(dev,buffer);
        sprintf(buffer,":TRIGGER:EDGE:LEVEL CHANNEL1,0.0");envoi(dev,buffer);
        sprintf(buffer,":TRIGGER:SWEEP SINGLE"); envoi(dev,buffer);
// Right Click on sine wave on top of display, Setup Acquisition and see SamplingRate/MemDepth
}
//#endif
        set_range(range);
        set_rate(rate);
        set_duration(duration);
// #else
       }
     else // TCP/IP
      {if (_type==0) set_duration(1);
       sockfd = socket(AF_INET, SOCK_STREAM, 0);
       adresse.sin_family=AF_INET;
       adresse.sin_addr.s_addr = inet_addr(ip); // 127.0.0.1");
       if (_type==0)
          adresse.sin_port =htons(9999);
       else
          adresse.sin_port =htons(4000);  // Tektronix oscilloscope
       bzero(&(adresse.sin_zero),8);
       longueur = sizeof(adresse);
       connect(sockfd, (struct sockaddr *)&adresse, longueur);
       longueur=htonl(_channels);
       if (_type==0)
          write(sockfd,&longueur,sizeof(int)); // number of channels
       else
          {set_range(range);
           set_rate(rate);
           set_duration(duration);
           sprintf(buffer,"*IDN?\n");write(sockfd,buffer,strlen(buffer));
           read(sockfd, buffer, sizeof(char)*256);
           printf("IDN: %s\n",buffer);
          }
      }   // TCP/IP
      _num_values=0;  // amount of data sent from buffers
// #endif
    }

    /*
     * Our virtual destructor.
     */
    oscilloscope_impl::~oscilloscope_impl()
    {
//#ifdef VXI11
     char buffer[256];
     int val;
     printf("Bye\n");
     if (_vxi11==1)
       {
//#ifdef agilent
if (_type==agilent)
{       sprintf(buffer,":CHANNEL1:DISPLAY ON"); envoi(dev,buffer);
        sprintf(buffer,":CHANNEL2:DISPLAY ON"); envoi(dev,buffer);
        sprintf(buffer,":TRIGGER:SWEEP AUTO"); envoi(dev,buffer);
}
//#endif
       }
//#else
     else
       {val=htonl(-1);
        write(sockfd,&val,sizeof(int));
        close(sockfd);
       }
// #endif
    }

    int
    oscilloscope_impl::work(int noutput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
    {float *out0 = (float *) output_items[0]; // 2 outputs
     float *out1 = (float *) output_items[1];
     float *out2 = (float *) output_items[2];
     float *out3 = (float *) output_items[3];
     long int k,val,offset;
     int chan_count;
     char mystring[256];
     char buffer[256];

#ifdef mydebug
     printf("channels: %ld\n",output_items.size()); // contient le nombre de canaux
#endif
//#ifdef VXI11
     if (_num_values==0) // not enough data left in buffer -- reload
       {
#ifdef mydebug
        printf("collecting new data: %d\n",_sample_size);
#endif
        _num_values=_sample_size;
        _position=0;
        if (_vxi11==1)
          {
//#ifdef agilent
if (_type==agilent)
{          sprintf(buffer,":DIGITIZE CHANNEL1,CHANNEL2\n");envoi(dev,buffer);
           sprintf(buffer,":WAVEFORM:SOURCE CHANNEL1");envoi(dev,buffer);
     //      sprintf(buffer,":WAVEFORM:VIEW MAIN");envoi(dev,buffer);
     //      sprintf(buffer,":ACQUIRE:COMPLETE 100");envoi(dev,buffer);
           sprintf(buffer,":WAVEFORM:FORMAT WORD;BYTEORDER LSBFIRST\n");envoi(dev,buffer);
     //      sprintf(buffer,":WAVEFORM:FORMAT ASCII");envoi(dev,buffer);
           vxi11_send_and_receive(dev, "WAVEFORM:DATA?", _data_buffer, (2*_sample_size+30), 100*VXI11_READ_TIMEOUT); // extend timeout
           if (_data_buffer[0]!='#') printf("error in trace header\n"); // printf("%c",buffer[0]); //#
           else
             {offset=_data_buffer[1]-'0';
#ifdef mydebug
              printf("%ld -> ",offset);
#endif
              for (k=0;k<_sample_size;k++)  // rm # and header
                 _tab1[k]=(float)(*(short*)(&_data_buffer[2*k+offset+2]))/65536.; // valid only on Intel/LE

              sprintf(buffer,":WAVEFORM:SOURCE CHANNEL2");envoi(dev,buffer);
              sprintf(buffer,":WAVEFORM:VIEW MAIN");envoi(dev,buffer);
              sprintf(buffer,":WAVEFORM:FORMAT WORD;BYTEORDER LSBFIRST\n");envoi(dev,buffer);
//              sprintf(buffer,":WAVEFORM:FORMAT ASCII");envoi(dev,buffer);
              vxi11_send_and_receive(dev, "WAVEFORM:DATA?", _data_buffer, (2*_sample_size+30), 100*VXI11_READ_TIMEOUT); // extend timeout
              if (_data_buffer[0]!='#')
                 printf("error in trace header\n"); //  printf("%c",buffer[0]);  //#
              else
                 {offset=_data_buffer[1]-'0';
#ifdef mydebug
                  printf("%ld -> ",offset);
#endif
                  for (k=0;k<_sample_size;k++)  // rm # and header
                     _tab2[k]=(float)(*(short*)(&_data_buffer[2*k+offset+2]))/65536.; // valid only on Intel/LE
                 }
             } // tab1 and tab2 now how the two-channel data
//#endif
}
//#ifdef rohdeschwarz
if (_type==rohdeschwarz)
{          sprintf(buffer,"FORM:DATA INT,16\n");envoi(dev,buffer); // LSB first by default
           sprintf(buffer,"RUNSINGLE\n");envoi(dev,buffer);
           sprintf(buffer,"*OPC?"); envoi(dev,buffer); relit(dev,buffer,256);

           for (chan_count=1;chan_count<=_channels;chan_count++)
             {sprintf(mystring,"CHAN%d:WAV:DATA?",chan_count);
              vxi11_send_and_receive(dev, mystring, _data_buffer, (2*_sample_size+100), 100*VXI11_READ_TIMEOUT); // extend timeout
              if (_data_buffer[0]!='#')
                 printf("error in trace header\n"); //  printf("%c",buffer[0]);  // #
              else
                {offset=_data_buffer[1]-'0';       // ASCII -> dec
#ifdef mydebug
                 printf("#ok => skipping %ld chars ; ",offset);
#endif
                }
              for (k=0;k<_sample_size;k++)  // rm # and header
                {if (chan_count==1) _tab1[k]=(float)(*(short*)(&_data_buffer[2*k+offset+2]))/65536.; // valid only on Intel/LE
                 if (chan_count==2) _tab2[k]=(float)(*(short*)(&_data_buffer[2*k+offset+2]))/65536.; // valid only on Intel/LE
                 if (chan_count==3) _tab3[k]=(float)(*(short*)(&_data_buffer[2*k+offset+2]))/65536.; // valid only on Intel/LE
                 if (chan_count==4) _tab4[k]=(float)(*(short*)(&_data_buffer[2*k+offset+2]))/65536.; // valid only on Intel/LE
                }
             } // end of chan_count
} // #endif
// #else // VXI11 -> mode tcp server
          }
     else
       {if (_type==0)
         {val=htonl(_sample_size);  // TCP server knows how many channels are requested
#ifdef mydebug
          printf("%d items requested\n",_sample_size);
#endif
          write(sockfd,&val,sizeof(long int));
          read(sockfd, _tab1, sizeof(float)*_sample_size);
          if (_channels>=2) read(sockfd, _tab2, sizeof(float)*_sample_size);
          if (_channels>=3) read(sockfd, _tab3, sizeof(float)*_sample_size);
          if (_channels>=4) read(sockfd, _tab4, sizeof(float)*_sample_size);
// #endif
         }
       else  // tektro
         {sprintf(buffer,"HEADER OFF\n");write(sockfd,buffer,strlen(buffer));usleep(50000);
          sprintf(buffer,"ACQUIRE:STOPAFTER SEQUENCE\n");write(sockfd,buffer,strlen(buffer));usleep(50000);
          sprintf(buffer,"ACQUIRE:STATE RUN\n");write(sockfd,buffer,strlen(buffer));usleep(50000);
          sprintf(buffer,"*OPC?\n");write(sockfd,buffer,strlen(buffer));usleep(50000);
          memset(buffer,0,sizeof(buffer));read(sockfd, buffer, 256);
          int nr_pt = tek_query_int(sockfd, "HORIZONTAL:RECORDLENGTH?\n");
          if (nr_pt <= 0) nr_pt = _sample_size;
#ifdef mydebug
          printf("Tek RL = %d\n", nr_pt);
#endif
          if (nr_pt > _sample_size) {
            _sample_size = nr_pt;
            _data_buffer=(char*)realloc(_data_buffer,2*_sample_size+100);
            _tab1=(float*)realloc(_tab1,_sample_size*sizeof(float));
            _tab2=(float*)realloc(_tab2,_sample_size*sizeof(float));
            _tab3=(float*)realloc(_tab3,_sample_size*sizeof(float));
            _tab4=(float*)realloc(_tab4,_sample_size*sizeof(float));
          }
          for (chan_count=1;chan_count<=_channels;chan_count++)
            {sprintf(buffer,"DATA:SOURCE CH%d\n",chan_count);write(sockfd,buffer,strlen(buffer));usleep(50000);
             sprintf(buffer,"DATA:ENCDG RIBinary\n");write(sockfd,buffer,strlen(buffer));usleep(50000);
             sprintf(buffer,"DATA:WIDTH 2\n");write(sockfd,buffer,strlen(buffer));usleep(50000);
             float ymult = tek_query_float(sockfd, "WFMOUTPRE:YMULT?\n");
             float yoff  = tek_query_float(sockfd, "WFMOUTPRE:YOFF?\n");
             float yzero = tek_query_float(sockfd, "WFMOUTPRE:YZERO?\n");
             ymult = ymult / 1000000.0f;
#ifdef mydebug
             printf("CH%d: ymult=%e yoff=%e yzero=%e\n", chan_count, ymult, yoff, yzero);
#endif
             sprintf(buffer,"DATA:START 1\n");write(sockfd,buffer,strlen(buffer));usleep(50000);
             sprintf(buffer,"DATA:STOP %d\n", nr_pt);write(sockfd,buffer,strlen(buffer));usleep(50000);
             sprintf(buffer,"CURVE?\n");write(sockfd,buffer,strlen(buffer));usleep(50000);
             char h[16];
             if (recv_all(sockfd, h, 2) != 2 || h[0] != '#') break;
             int nd = h[1]-'0';
             if (nd<=0 || nd>9) break;
             if (recv_all(sockfd, h, nd) != nd) break;
             h[nd]=0;
             int data_bytes = atoi(h);
             if (data_bytes > (2*_sample_size))
               _data_buffer=(char*)realloc(_data_buffer, data_bytes+100);
             if (recv_all(sockfd, _data_buffer, data_bytes) != data_bytes) break;
             char nl; read(sockfd,&nl,1);
             const unsigned char* u8 = reinterpret_cast<const unsigned char*>(_data_buffer);
             long kmax = (nr_pt < _sample_size) ? nr_pt : _sample_size;
             for (k=0; k<kmax; k++) {
               uint16_t u16 = ((uint16_t)u8[2*k] << 8) | (uint16_t)u8[2*k+1];
               int16_t s16 = (int16_t)u16;
               float volts = (((float)s16 - yoff) * ymult) + yzero;
               if (chan_count==1) _tab1[k]=volts;
               if (chan_count==2) _tab2[k]=volts;
               if (chan_count==3) _tab3[k]=volts;
               if (chan_count==4) _tab4[k]=volts;
             }
            }
         }
     }
     } // end of _num_values==0

     for (k=0;((k<noutput_items) && (_position<_sample_size));k++)
       {out0[k]=_tab1[_position];
        if (_channels>=2) out1[k]=_tab2[_position];
        if (_channels>=3) out2[k]=_tab3[_position];
        if (_channels>=4) out3[k]=_tab4[_position];
        _num_values--;
        _position++;
       }
     if (_num_values==0)
        {
#ifdef mydebug
         printf("_num_values==0\n");fflush(stdout);
#endif
         return(k); // only return what was left in the buffer
        }
      // Tell runtime system how many output items we produced.
#ifdef mydebug
      printf("noutput_items=%d _num_values=%d _position=%d\n",noutput_items,_num_values,_position);
      fflush(stdout);
#endif
     return noutput_items;
    }

void oscilloscope_impl::set_type(int type)
{if ((type==rohdeschwarz)||(type==agilent))
    _vxi11=1;
 else
    _vxi11=0; // TCP/IP server or Tektronix
 _type=type;
}

void oscilloscope_impl::set_ip(char *ip)
{int k,cnt=0;;
 for (k=0;k<(int)strlen(ip);k++) {if (ip[k]=='.') cnt++;}
 if (cnt==3) sprintf(device_ip,"%s",ip);
    else {printf("invalid IP @\n");sprintf(device_ip,"127.0.0.1");} // TCP server on lo
 printf("IP address: %s -- check that the computer is on the same subnet\n",device_ip);
}

void oscilloscope_impl::set_duration(float duration)
{char buffer[256];
 if (_vxi11==1)
    {_sample_size = (int)(duration * _rate);
//#ifdef agilent
if (_type==agilent)
    {sprintf(buffer,":TIMEBASE:REFERENCE LEFT;POSITION 0;RANGE %e",duration);envoi(dev,buffer);
    }
//#endif
//#ifdef rohdeschwarz
if (_type==rohdeschwarz)
// Defines the time of one acquisition, that is the time across the 10 divisions
    {sprintf(buffer,"TIM:RANGE %f\n",duration);envoi(dev,buffer);     // of the diagram:
    }
//#endif
    }
 else
   {if (_type==tektronix)
      {_sample_size = (int)(duration * _rate);
       sprintf(buffer,"HORIZONTAL:RECORDLENGTH %d\n",_sample_size);write(sockfd,buffer,strlen(buffer));usleep(50000);
       int rl = tek_query_int(sockfd, "HORIZONTAL:RECORDLENGTH?\n");
       if (rl>0) _sample_size = rl;
      }
    else
    _sample_size = 8192;
   }
 printf("_sample_size=%d\n",_sample_size);
 _data_buffer=(char*)realloc(_data_buffer,2*_sample_size+100);
 _tab1=(float*)realloc(_tab1,_sample_size*sizeof(float));
 _tab2=(float*)realloc(_tab2,_sample_size*sizeof(float));
 _tab3=(float*)realloc(_tab3,_sample_size*sizeof(float));
 _tab4=(float*)realloc(_tab4,_sample_size*sizeof(float));
/*
 if (_data_buffer!=NULL)
   {free(_data_buffer);
    free(_tab1);
    free(_tab2);
    free(_tab3);
    free(_tab4);
   }
 _data_buffer=(char*)malloc(2*_sample_size+100);
 _tab1=(float*)malloc(_sample_size*sizeof(float));
 _tab2=(float*)malloc(_sample_size*sizeof(float));
 _tab3=(float*)malloc(_sample_size*sizeof(float));
 _tab4=(float*)malloc(_sample_size*sizeof(float));
*/
 printf("new duration: %f\n",duration);fflush(stdout);
 _duration=duration;
}

void oscilloscope_impl::set_range(float range)
{char buffer[256];
 printf("new range: %e\n",range);fflush(stdout);
 if (_vxi11==1)
   {
//#ifdef agilent
if (_type==agilent)
{   sprintf(buffer,":CHANNEL1:RANGE %e;OFFSET 0.0",range);envoi(dev,buffer);
    sprintf(buffer,":CHANNEL2:RANGE %e;OFFSET 0.0",range);envoi(dev,buffer);
}
//#endif
//#ifdef rohdeschwarz
if (_type==rohdeschwarz)
{   sprintf(buffer,"CHAN1:SCAL %f\n",range);envoi(dev,buffer); // RANG is not working ?!
    if (_channels>=2) {sprintf(buffer,"CHAN2:SCAL %f\n",range);envoi(dev,buffer);}
    if (_channels>=3) {sprintf(buffer,"CHAN3:SCAL %f\n",range);envoi(dev,buffer);}
    if (_channels>=4) {sprintf(buffer,"CHAN4:SCAL %f\n",range);envoi(dev,buffer);}
}
//#endif
   }
 else
   if (_type==tektronix)
     {float scale = range / 10.0f;
      for (int i=1;i<=_channels;i++) {
        sprintf(buffer,"CH%d:SCALE %e\n",i,scale);
        write(sockfd,buffer,strlen(buffer));usleep(50000);
      }
     }
 _range=range;
}

void oscilloscope_impl::set_channels(int channels)
{char buffer[256];
 _channels=channels;
 printf("Channels set to %d\n",channels);
 if (_type==tektronix)
   {for (int i=1;i<=4;i++) {
      sprintf(buffer,"SELECT:CH%d %s\n", i, (i<=channels)?"ON":"OFF");
      write(sockfd,buffer,strlen(buffer));usleep(50000);
    }
   }
}

void oscilloscope_impl::set_rate(float rate)
{char buffer[256];
 if (_vxi11==1)
    {_sample_size = (int)(_duration * rate);
//#ifdef agilent
if (_type==agilent)
{    sprintf(buffer,":ACQUIRE:MODE RTIME;AVERAGE OFF;SRATE %e;POINTS %d",rate,_sample_size);
     envoi(dev,buffer);
}
//#endif
//#ifdef rohdeschwarz
// Defines the time of one acquisition, that is the time across the 10 divisions
if (_type==rohdeschwarz)
{    sprintf(buffer,"ACQ:SRATE %f\n",rate);
     envoi(dev,buffer);
}
//#endif
    }
 else
  {if (_type==tektronix)
    {_sample_size = (int)(_duration * rate);
     sprintf(buffer,"HORIZONTAL:MODE:SAMPLERATE %e\n",rate);write(sockfd,buffer,strlen(buffer));usleep(50000);
     sprintf(buffer,"HORIZONTAL:RECORDLENGTH %d\n",_sample_size);write(sockfd,buffer,strlen(buffer));usleep(50000);
     int rl = tek_query_int(sockfd, "HORIZONTAL:RECORDLENGTH?\n");
     if (rl>0) _sample_size = rl;
    }
   else
    _sample_size = 8192;
  }
 printf("_sample_size=%d\n",_sample_size);
// If  ptr  is  NULL,  then  the call is equivalent to malloc(size), for all values of size
 _data_buffer=(char*)realloc(_data_buffer,2*_sample_size+100);
 _tab1=(float*)realloc(_tab1,_sample_size*sizeof(float));
 _tab2=(float*)realloc(_tab2,_sample_size*sizeof(float));
 _tab3=(float*)realloc(_tab3,_sample_size*sizeof(float));
 _tab4=(float*)realloc(_tab4,_sample_size*sizeof(float));
/*
 if (_data_buffer!=NULL)
   {free(_data_buffer);
    free(_tab1);
    free(_tab2);
    free(_tab3);
    free(_tab4);
  }
 _data_buffer=(char*)malloc(2*_sample_size+100);
 _tab1=(float*)malloc(_sample_size*sizeof(float));
 _tab2=(float*)malloc(_sample_size*sizeof(float));
 _tab3=(float*)malloc(_sample_size*sizeof(float));
 _tab4=(float*)malloc(_sample_size*sizeof(float));
*/
 printf("new rate: %f\n",rate);fflush(stdout);
 _rate=rate;
}
  } /* namespace oscilloscope */
} /* namespace gr */
