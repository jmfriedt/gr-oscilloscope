#pragma once
/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Declares the backend factory interface.
 *   Creates instrument-specific backends from a type identifier.
 */

#include <memory>

namespace gr {
namespace oscilloscope {

class scope_backend;
class oscilloscope_impl;

// Backend factory
std::unique_ptr<scope_backend> make_backend(int type, oscilloscope_impl* owner);

} // namespace oscilloscope
} // namespace gr
