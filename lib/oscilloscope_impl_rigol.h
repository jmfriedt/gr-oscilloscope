// lib/oscilloscope_impl_rigol.h
#pragma once
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Rigol backend declaration (VXI-11 / SCPI).
 *   Implements acquisition, leaves scope scaling untouched.
 */

#include "oscilloscope_impl.h"

namespace gr {
namespace oscilloscope {

class scope_backend_rigol : public scope_backend
{
public:
  explicit scope_backend_rigol(oscilloscope_impl* owner) : _o(owner) {}
  ~scope_backend_rigol() override = default;

  bool init() override;
  void shutdown() override;

  // Do not modify scope settings
  bool apply_range(float) override     { return true; }
  bool apply_rate(float) override      { return true; }
  bool apply_duration(float) override  { return true; }
  bool acquire() override;

private:
  oscilloscope_impl* _o;

  static int  relit(VXI11_CLINK* clink, char* buffer, int buffer_length);
  static void envoi(VXI11_CLINK* clink, const char* buffer);
};

} // namespace oscilloscope
} // namespace gr
