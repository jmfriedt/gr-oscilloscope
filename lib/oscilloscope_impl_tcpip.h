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

class scope_backend_tcpip : public scope_backend
{
public:
  explicit scope_backend_tcpip(oscilloscope_impl* owner) : _o(owner) {}
  ~scope_backend_tcpip() override = default;

  bool init() override;
  void shutdown() override;
  bool apply_range(float range) override;
  bool apply_rate(float rate) override;
  bool apply_duration(float dur) override;
  bool acquire() override;

private:
  oscilloscope_impl* _o;
  static int   relit(int fd, char* buf, int total);
};

} // namespace oscilloscope
} // namespace gr
