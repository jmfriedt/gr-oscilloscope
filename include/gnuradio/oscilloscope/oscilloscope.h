/* -*- c++ -*- */
/*
 * Copyright 2025 JM Friedt (jmfriedt@femto-st.fr).
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_OSCILLOSCOPE_OSCILLOSCOPE_H
#define INCLUDED_OSCILLOSCOPE_OSCILLOSCOPE_H

#include <gnuradio/oscilloscope/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace oscilloscope {

/*!
 * \brief <+description of block+>
 * \ingroup oscilloscope
 *
 */
class OSCILLOSCOPE_API oscilloscope : virtual public gr::sync_block
{
public:
   typedef std::shared_ptr<oscilloscope> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of oscilloscope::oscilloscope.
     *
     * To avoid accidental use of raw pointers, oscilloscope::oscilloscope's
     * constructor is in a private implementation
     * class. oscilloscope::oscilloscope::make is the public interface for
     * creating new instances.
     */
   static sptr make(char* ip, float range, float rate, float duration, int channels, int type);

   virtual void set_range(float range) = 0;     // setters exposed to GRC callbacks / Python
   virtual void set_rate(float rate) = 0;
   virtual void set_duration(float duration) = 0;
   virtual void set_ip(char* ip) = 0;
   virtual void set_channels(int channels) = 0;
   virtual void set_type(int type) = 0;
//   virtual void set_acq_mode(int acq_mode) = 0; // 0 = RAW (mem), 1 = NORM/LIVE (display/fast) — only meaningful for Rigol/Tek
};

} // namespace oscilloscope
} // namespace gr

#endif /* INCLUDED_OSCILLOSCOPE_OSCILLOSCOPE_H */
