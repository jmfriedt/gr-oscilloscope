// lib/oscilloscope_factory.cc
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Backend factory.
 *   Maps oscilloscope type identifiers to concrete backend implementations.
 *   Instrument-specific logic is selected here and nowhere else.
 */

#include "oscilloscope_factory.h"
#include "scope_types.h"
#include "oscilloscope_impl.h"
#include "oscilloscope_impl_tektronix.h"
#include "oscilloscope_impl_rigol.h"
#include "oscilloscope_impl_agilent.h"
#include "oscilloscope_impl_rohdeschwarz.h"

namespace gr {
namespace oscilloscope {

std::unique_ptr<scope_backend> make_backend(int type, oscilloscope_impl* owner)
{
  switch (type) {
    case SCOPE_TEKTRONIX:
      return std::make_unique<scope_backend_tektronix>(owner);
    case SCOPE_RIGOL:
      return std::make_unique<scope_backend_rigol>(owner);
    case SCOPE_AGILENT:
      return std::make_unique<scope_backend_agilent>(owner);
    case SCOPE_ROHDE_SCHWARZ:
      return std::make_unique<scope_backend_rohdeschwarz>(owner);
    default:
      return nullptr;
  }
}

} // namespace oscilloscope
} // namespace gr
