#include "solve_quadratics.h"

double get_discriminant(const double a, const double b, const double c) {
    return b*b - 4*a*c;
  }
  
  std::vector<double> solve_quadratics(const double a, const double b, const double c) {
    double discriminant = get_discriminant(a, b, c);
    if (discriminant < 0)
      return std::vector<double>();
    double root_1 = (-b + std::sqrt(discriminant)) / (2*a),
           root_2 = (-b - std::sqrt(discriminant)) / (2*a);
    if (compare_nsp::is_close(discriminant, 0))
      return std::vector<double>({root_1});
    return std::vector<double>({root_1, root_2});
  }
