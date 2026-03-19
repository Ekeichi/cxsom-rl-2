#pragma once

#include "discrete-rocket-problem.hpp"

// This configures the rocket discretization for all our examples.

using types = rocket::enumerable::types<
  51, -100., 100., // nb errors, min, max
  51, -100., 100., // nb speeds, min, max
  15.,    1.>;     // up thrust, simulation step duration (dt).

inline gdyn::problem::rocket::parameters make_params() {return {};}
