/* -*- c++ -*- */
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Rohde & Schwarz backend declaration (VXI-11 / SCPI).
 *   Implements acquisition and basic instrument configuration.
 */
#pragma once

#include "oscilloscope_impl.h"

namespace gr {
namespace oscilloscope {

class scope_backend_rohdeschwarz : public scope_backend
{
public:
  explicit scope_backend_rohdeschwarz(oscilloscope_impl* owner) : _o(owner) {}
  ~scope_backend_rohdeschwarz() override = default;

  bool init() override;
  void shutdown() override;

  bool apply_range(float range) override;
  bool apply_rate(float rate) override;
  bool apply_duration(float dur) override;
  bool acquire() override;

private:
  oscilloscope_impl* _o = nullptr;

  static int  relit(VXI11_CLINK* clink, char* buffer, int buffer_length);
  static void envoi(VXI11_CLINK* clink, const char* s);
};

} // namespace oscilloscope
} // namespace gr
