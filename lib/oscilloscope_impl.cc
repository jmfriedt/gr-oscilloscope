/* -*- c++ -*- */
/* 
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#define VXI11
#define SIZE 80000
//#define mydebug

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "oscilloscope_impl.h"
#include "./vxi11/library/vxi11_user.h"

#ifdef VXI11
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
#endif

namespace gr {
  namespace oscilloscope {

    oscilloscope::sptr
    oscilloscope::make(char *ip,float range,int rate,float duration)
    {
      return gnuradio::get_initial_sptr
        (new oscilloscope_impl(ip,range,rate,duration));
    }

    /*
     * The private constructor
     */
    oscilloscope_impl::oscilloscope_impl(char *ip,float range,int rate,float duration)
      : gr::sync_block("oscilloscope",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(2, 2, sizeof(float))),
	      _range(range),_rate(rate),_duration(duration)
    {
#ifdef VXI11
     char *device_name=NULL;
     set_ip(ip); // "169.254.202.240"); // Agilent 54855DSO
     if (vxi11_open_device(&dev,device_ip,device_name)!=0) printf("error opening\n");
        else printf("connect OK\n");
     char buffer[256];
     int buffer_length=256;
     sprintf(buffer,"*IDN?");
     envoi(dev,buffer);
     relit(dev,buffer,buffer_length);
     printf("%s\n",buffer);

     sprintf(buffer,"*CLS"); envoi(dev,buffer);
     sprintf(buffer,"*RST"); envoi(dev,buffer);
     sprintf(buffer,":SYSTEM:HEADER OFF"); envoi(dev,buffer);
//   sprintf(buffer,":AUTOSCALE");envoi(dev,buffer); // + (sampleDuration));
     sprintf(buffer,":TRIGGER:EDGE:SOUCE CHANNEL1;SLOPE POSITIVE");envoi(dev,buffer); 
     sprintf(buffer,":TRIGGER:EDGE:LEVEL CHANNEL1,0.0");envoi(dev,buffer);
     set_range(range);
     sprintf(buffer,":TRIGGER:SWEEP SINGLE"); envoi(dev,buffer);
     set_range(rate);
     set_duration(duration);
// Right Click on sine wave on top of display, Setup Acquisition and see SamplingRate/MemDepth
#else
     int longueur;
     struct sockaddr_in adresse;
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     adresse.sin_family=AF_INET;
     adresse.sin_addr.s_addr = inet_addr("127.0.0.1");
     adresse.sin_port =htons(9999);
     bzero(&(adresse.sin_zero),8);
     longueur = sizeof(adresse);
     connect(sockfd, (struct sockaddr *)&adresse, longueur);
#endif
    }

    /*
     * Our virtual destructor.
     */
    oscilloscope_impl::~oscilloscope_impl()
    {
#ifdef VXI11
     char buffer[256];
     printf("Bye\n");
     sprintf(buffer,":CHANNEL1:DISPLAY ON"); envoi(dev,buffer);
     sprintf(buffer,":CHANNEL2:DISPLAY ON"); envoi(dev,buffer);
     sprintf(buffer,":TRIGGER:SWEEP AUTO"); envoi(dev,buffer);
#else
     int val;
     val=htonl(-1);
     write(sockfd,&val,sizeof(int));
     close(sockfd);
#endif
    }

    int
    oscilloscope_impl::work(int noutput_items,
      gr_vector_const_void_star &input_items,
      gr_vector_void_star &output_items)
    {float *out0 = (float *) output_items[0]; // 2 outputs
     float *out1 = (float *) output_items[1];
     float tab1[2*noutput_items];
     float tab2[2*noutput_items];
     long int k,val,offset;
#ifdef VXI11
     char buffer[SIZE];
     int buffer_length=SIZE;
     int res;
     sprintf(buffer,":DIGITIZE CHANNEL1,CHANNEL2\n");envoi(dev,buffer);
     sprintf(buffer,":WAVEFORM:SOURCE CHANNEL1");envoi(dev,buffer);
     //sprintf(buffer,":WAVEFORM:VIEW MAIN");envoi(dev,buffer);
     //sprintf(buffer,":ACQUIRE:COMPLETE 100");envoi(dev,buffer);
     sprintf(buffer,":WAVEFORM:FORMAT WORD;BYTEORDER LSBFIRST\n");envoi(dev,buffer);
     //sprintf(buffer,":WAVEFORM:FORMAT ASCII");envoi(dev,buffer);
     sprintf(buffer,":WAVEFORM:DATA?");envoi(dev,buffer);
     res=relit(dev,buffer,buffer_length);
     if (buffer[0]!='#') printf("error in trace header\n"); //  printf("%c",buffer[0]);  // #
     else
       {offset=buffer[1]-'0'; 
#ifdef mydebug
        printf("%d -> ",offset);
#endif
        for (k=0;((k<((res-offset-1)/2)) && (k<noutput_items));k++)  // rm # and header
           tab1[k]=(float)(*(short*)(&buffer[2*k+offset+2]))/65536.; // valid only on Intel/LE
//#ifdef mydebug
        printf("CH1: res/2=%d noutput_items=%d\n",res/2,noutput_items);
//#endif

        sprintf(buffer,":WAVEFORM:SOURCE CHANNEL2");envoi(dev,buffer);
        sprintf(buffer,":WAVEFORM:VIEW MAIN");envoi(dev,buffer);
        sprintf(buffer,":WAVEFORM:FORMAT WORD;BYTEORDER LSBFIRST\n");envoi(dev,buffer);
//      sprintf(buffer,":WAVEFORM:FORMAT ASCII");envoi(dev,buffer);
        sprintf(buffer,":WAVEFORM:DATA?");envoi(dev,buffer);
        res=relit(dev,buffer,buffer_length);
        if (buffer[0]!='#') printf("error in trace header\n"); //  printf("%c",buffer[0]);  // #
        else 
          {offset=buffer[1]-'0';
#ifdef mydebug
           printf("%d -> ",offset);
#endif
           for (k=0;((k<((res-offset-1)/2)) && (k<noutput_items));k++)  // rm # and header
              tab2[k]=(float)(*(short*)(&buffer[2*k+offset+2]))/65536.; // valid only on Intel/LE
#ifdef mydebug
           printf("CH2: res/2=%d noutput_items=%d\n",res/2,noutput_items);
#endif // debug
          }
     }
#else // VXI11
     val=htonl(noutput_items*2);
     write(sockfd,&val,sizeof(long int));
     read(sockfd, tab1, sizeof(float)*noutput_items*2);
     for (k=0;k<noutput_items;k++) {tab2[k]=tab1[2*k];tab1[k]=tab1[2*k+1];}
#endif
     for (k=0;k<noutput_items;k++)
          {out0[k]=tab1[k];
           out1[k]=tab2[k]; 
          }
      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

void oscilloscope_impl::set_ip(char *ip)
{int k,cnt=0;;
 for (k=0;k<strlen(ip);k++) {if (ip[k]=='.') cnt++;}
 if (cnt==3) sprintf(device_ip,"%s",ip); 
#ifdef VXI11
    else {printf("invalid IP @\n");sprintf(device_ip,"169.254.202.240");}
#else
    else {printf("invalid IP @\n");sprintf(device_ip,"127.0.0.1");} // TCP server on lo
#endif
 printf("IP address: %s -- check that the computer is on the same subnet\n",device_ip);
}

void oscilloscope_impl::set_duration(float duration)
{char buffer[256];
 printf("new duration: %f\n",duration);fflush(stdout);
 sprintf(buffer,":TIMEBASE:REFERENCE LEFT;POSITION 0;RANGE %e",duration);envoi(dev,buffer);
 _duration=duration;
}

void oscilloscope_impl::set_range(float range)
{char buffer[256];
 printf("new range: %f\n",range);fflush(stdout);
 sprintf(buffer,":CHANNEL1:RANGE %e;OFFSET 0.0",range);envoi(dev,buffer);
 sprintf(buffer,":CHANNEL2:RANGE %e;OFFSET 0.0",range);envoi(dev,buffer);
 _range=range;
}

void oscilloscope_impl::set_rate(int rate)
{char buffer[256];
 int samplesize = (int)(_duration * (float)rate);
 printf("new rate: %f\n",rate);fflush(stdout);
 sprintf(buffer,":ACQUIRE:MODE RTIME;AVERAGE OFF;SRATE %e;POINTS %d",rate,samplesize);
 envoi(dev,buffer);
 _rate=rate;
}
  } /* namespace oscilloscope */
} /* namespace gr */
