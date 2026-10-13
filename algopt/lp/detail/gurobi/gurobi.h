// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifndef REBALANCER_OSS_BUILD
#include "algopt/lp/environment/fb/Constants.h"
#endif

#ifdef REBALANCER_USE_GUROBI

#include "algopt/lp/generic/SolverError.h"

#include <gurobi_c++.h>

namespace facebook::algopt::lp::detail {

// Every Gurobi entry point reachable through the generic interface rethrows
// GRBException through here, so callers see the same SolverError whatever the
// backend. The message is kept as Gurobi's own, which callers already match.
[[noreturn]] inline void rethrowGurobiError(const GRBException& e) {
  throw SolverError(e.getMessage());
}

} // namespace facebook::algopt::lp::detail

#endif
