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
    static sptr make(char*,float,float,float,int);
};

} // namespace oscilloscope
} // namespace gr

#endif /* INCLUDED_OSCILLOSCOPE_OSCILLOSCOPE_H */
