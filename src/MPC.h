#ifndef MPC_H
#define MPC_H

#include <vector>
#include "Eigen-3.3/Eigen/Core"

using namespace std;

struct Solution {
    vector<double> x_points;
    vector<double> y_points;
    double delta_actuation; // solution for delta actuation.
    double a_actuation; // solution for accelaration actuation.
};

struct Transform {
    vector<double> trans_X;
    vector<double> trans_Y;
};

class MPC {
 public:
  MPC();

  virtual ~MPC();

  // Solve the model given an initial state and polynomial coefficients.
  // Return the first actuatotions.
  Solution Solve(Eigen::VectorXd state, Eigen::VectorXd coeffs);
};

#endif /* MPC_H */
