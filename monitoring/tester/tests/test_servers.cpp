#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include "solve_quadratics.h"

float Accuracy = 0.0000001;

BOOST_AUTO_TEST_CASE(solve_quadratics_Two_Root) {
  std::vector<double> solution;
  const unsigned int nSolution = 2;
  const double good_solution[nSolution] = { -0.5, -3 };
  BOOST_CHECK_NO_THROW(solution = solve_quadratics(2, 7, 3));
  BOOST_REQUIRE(solution.size() == nSolution);
  BOOST_REQUIRE_CLOSE(solution[0], good_solution[0], Accuracy);
  BOOST_REQUIRE_CLOSE(solution[1], good_solution[1], Accuracy);
}
BOOST_AUTO_TEST_CASE(solve_quadratics_Single_root) {
  std::vector<double> solution;
  const unsigned int nSolution = 1;
  const double good_solution = -1.;
  BOOST_CHECK_NO_THROW(solution = solve_quadratics(4, 8, 4));
  BOOST_REQUIRE(solution.size() == nSolution);
  BOOST_REQUIRE_CLOSE(solution[0], good_solution, Accuracy);
}
BOOST_AUTO_TEST_CASE(solve_quadratics_No_root) {
  const unsigned int nSolution = 0;
  std::vector<double> solution;
  BOOST_CHECK_NO_THROW(solution = solve_quadratics(1, 2, 3));
  BOOST_REQUIRE(solution.size() == nSolution);
}
