/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Siglent backend declaration (VXI-11 / SCPI).
 */

#include "oscilloscope_impl.h"

namespace gr {
namespace oscilloscope {

class scope_backend_siglent : public scope_backend
{
public:
  explicit scope_backend_siglent(oscilloscope_impl* owner) : _o(owner) {}
  ~scope_backend_siglent() override = default;

  bool init() override;
  void shutdown() override;

  bool apply_range(float) override;
  bool apply_rate(float) override;
  bool apply_duration(float) override;
  bool acquire() override;

private:
  oscilloscope_impl* _o;

  static int  relit(VXI11_CLINK* clink, char* buffer, int buffer_length);
  static void envoi(VXI11_CLINK* clink, const char* buffer);
};

} // namespace oscilloscope
} // namespace gr
