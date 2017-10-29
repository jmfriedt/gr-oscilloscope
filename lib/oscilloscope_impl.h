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

#ifndef INCLUDED_OSCILLOSCOPE_OSCILLOSCOPE_IMPL_H
#define INCLUDED_OSCILLOSCOPE_OSCILLOSCOPE_IMPL_H

#include <oscilloscope/oscilloscope.h>

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
     private:
      // Nothing to declare in this block.
#ifdef VXI11
       VXI11_CLINK   *dev;
#else
//  char ch;short y;
       int sockfd;
       struct sockaddr_in adresse;
       int longueur,result;
#endif
       float _range,_duration,_rate;
       int _sample_size;
       char device_ip[16]; // IP @
       char *_data_buffer;

     public:
      void set_range(float);
      void set_rate(float);
      void set_duration(float);
      void set_ip(char*);
      oscilloscope_impl(char*,float,float,float);
      ~oscilloscope_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace oscilloscope
} // namespace gr

#endif /* INCLUDED_OSCILLOSCOPE_OSCILLOSCOPE_IMPL_H */

