/* -*- c++ -*- */
/*
 * Copyright 2025 JM Friedt (jmfriedt@femto-st.fr).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// #define VXI11

#ifndef INCLUDED_OSCILLOSCOPE_OSCILLOSCOPE_IMPL_H
#define INCLUDED_OSCILLOSCOPE_OSCILLOSCOPE_IMPL_H

#include <gnuradio/oscilloscope/oscilloscope.h>

#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdio.h>
#include "./vxi11/library/vxi11_user.h"

namespace gr {
namespace oscilloscope {

class oscilloscope_impl : public oscilloscope
{

#define tcpip 0
#define agilent 1
#define rohdeschwarz 2

private:
// Nothing to declare in this block.
// #ifdef VXI11
       VXI11_CLINK   *dev;
// #else
       int sockfd;
       struct sockaddr_in adresse;
       int longueur,result;
// #endif
       float _range,_rate,_duration;
       float *_tab1=NULL,*_tab2=NULL,*_tab3=NULL,*_tab4=NULL;
       int _sample_size;
       char device_ip[16]; // IP @
       char *_data_buffer=NULL;
       int _num_values,_position; // number of data left in buffer, and index in buffer
       int _noutput_position;
       int _channels;
       char _vxi11;    // select TCP/IP server if @==127.0.0.1, VXI11 otherwise
       int _type;

public:
      void set_type(int);
      void set_range(float);
      void set_rate(float);
      void set_duration(float);
      void set_ip(char*);
      void set_channels(int);
      oscilloscope_impl(char*,float,float,float,int,int);
      ~oscilloscope_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star& input_items,
         gr_vector_void_star& output_items);
};

} // namespace oscilloscope
} // namespace gr

#endif /* INCLUDED_OSCILLOSCOPE_OSCILLOSCOPE_IMPL_H */
