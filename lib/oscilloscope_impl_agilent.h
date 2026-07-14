#pragma once
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Agilent backend declaration (VXI-11 / SCPI).
 *   Implements acquisition and basic instrument configuration.
 */

#include "oscilloscope_impl.h"

namespace gr {
namespace oscilloscope {

class scope_backend_agilent : public scope_backend
{
public:
  explicit scope_backend_agilent(oscilloscope_impl* owner) : _o(owner) {}
  ~scope_backend_agilent() override = default;

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
