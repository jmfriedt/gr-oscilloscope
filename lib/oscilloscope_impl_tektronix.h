// lib/oscilloscope_impl_tektronix.h
#pragma once
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Tektronix backend declaration (TCP/IP / SCPI).
 *   Implements acquisition and optional instrument configuration.
 */

#include "oscilloscope_impl.h"

namespace gr {
namespace oscilloscope {

class scope_backend_tektronix : public scope_backend
{
public:
  explicit scope_backend_tektronix(oscilloscope_impl* owner) : _o(owner) {}
  ~scope_backend_tektronix() override = default;

  bool init() override;
  void shutdown() override;

  bool apply_range(float range) override;
  bool apply_rate(float rate) override;
  bool apply_duration(float dur) override;
  bool apply_channels(int ch) override;

  bool acquire() override;

private:
  oscilloscope_impl* _o;

  static int   recv_all(int fd, char* buf, int total);
  static int   tek_query_int(int fd, const char* cmd);
  static float tek_query_float(int fd, const char* cmd);
};

} // namespace oscilloscope
} // namespace gr
