#pragma once
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Defines oscilloscope backend type identifiers.
 *   Used for backend selection and factory dispatch.
 */

namespace gr {
namespace oscilloscope {

// Oscilloscope backend identifiers
enum scope_type_t : int
{
  SCOPE_TCPIP         = 0,
  SCOPE_ROHDE_SCHWARZ = 1,
  SCOPE_AGILENT       = 2,
  SCOPE_TEKTRONIX     = 3,
  SCOPE_RIGOL         = 4
};

} // namespace oscilloscope
} // namespace gr
