#pragma once

#include <vector>
#include <cmath>

namespace compare_nsp {
  inline bool is_close(const float a, const float b, const float epsilon = 0.00001) {
    throw "notImplemented";
  }
}

double get_discriminant(const double a, const double b, const double c);
std::vector<double> solve_quadratics(const double a, const double b, const double c);
