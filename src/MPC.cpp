#include "MPC.h"
#include <cppad/cppad.hpp>
#include <cppad/ipopt/solve.hpp>
#include "Eigen-3.3/Eigen/Core"

using CppAD::AD;

const size_t STEER_TUNER = 500;
const size_t STATE_VECTOR_SIZE = 6; // x, y, psi, v, cte, epsi
const size_t ACTUATOR_VECTOR_SIZE = 2; // delta, a

// TODO: Set the timestep length and duration
size_t N = 10;
double dt = 0.1; // in secs = 100 ms

// intialize reference variables. Like speed limit etc.

double ref_v = 30;
double ref_cte = 0; // reference errors should be always be 0.
double ref_epsi = 0;

// intialize the pointers inside the vars Vector to specify where different values are for the different paramaters.
size_t x_start = 0;
size_t y_start = x_start + N;
size_t psi_start = y_start + N;
size_t v_start = psi_start + N;
size_t cte_start = v_start + N;
size_t epsi_start = cte_start + N;
size_t delta_start = epsi_start + N;
size_t a_start = delta_start + N - 1; // see lecture notes. when N = 5 => [a1...a4]


// This value assumes the model presented in the classroom is used.
//
// It was obtained by measuring the radius formed by running the vehicle in the
// simulator around in a circle with a constant steering angle and velocity on a
// flat terrain.
//
// Lf was tuned until the the radius formed by the simulating the model
// presented in the classroom matched the previous radius.
//
// This is the length from front to CoG that has a similar radius.
const double Lf = 2.67;

class FG_eval {
public:
    // Fitted polynomial coefficients
    Eigen::VectorXd coeffs;

    FG_eval(Eigen::VectorXd coeffs) { this->coeffs = coeffs; }

    typedef CPPAD_TESTVECTOR(AD<double>) ADvector;

    void operator()(ADvector &fg, const ADvector &vars) {
        // TODO: implement MPC
        // `fg` a vector of the cost constraints, `vars` is a vector of variable values (state & actuators)
        // NOTE: You'll probably go back and forth between this function and
        // the Solver function below.


        // fg is the vector where the cost function and the model/constraints are defined.
        // vars is the vector which contains all the variables required for defining the cost function and the model/constraints.
        // The cost function is stored is the first element of `fg`.
        // Any additions to the cost should be added to `fg[0]`

        fg[0] = 0;

        // The part of the cost based on the reference state.
        for (int t = 0; t < N; t++) {
            fg[0] += CppAD::pow(vars[cte_start + t] - ref_cte, 2);
            fg[0] += CppAD::pow(vars[epsi_start + t] - ref_epsi, 2);
            fg[0] += CppAD::pow(vars[v_start + t] - ref_v, 2);
        }

        // Minimize the use of actuators.
        for (int t = 0; t < N - 1; t++) {
            fg[0] += CppAD::pow(vars[delta_start + t], 2);
            fg[0] += CppAD::pow(vars[a_start + t], 2);
        }

        // Minimize the value gap between sequential actuations.
        for (int t = 0; t < N - 2; t++) {
            fg[0] += STEER_TUNER * CppAD::pow(vars[delta_start + t + 1] - vars[delta_start + t], 2);
            fg[0] += CppAD::pow(vars[a_start + t + 1] - vars[a_start + t], 2);
        }


        // compute model/constraints

        // setup initial constraints

        // We initialize the model to the initial state.
        // Recall fg[0] is reserved for the cost value, so the other indices are bumped up by 1.

        fg[1 + x_start] = vars[x_start];
        fg[1 + y_start] = vars[y_start];
        fg[1 + psi_start] = vars[psi_start];
        fg[1 + v_start] = vars[v_start];
        fg[1 + cte_start] = vars[cte_start];
        fg[1 + epsi_start] = vars[epsi_start];


        // compute the other remaining constraints based on vehicle model.

        for (int t = 1; t < N; t++) {
            // The state at time t+1 .
            AD<double> x1 = vars[x_start + t];
            AD<double> y1 = vars[y_start + t];
            AD<double> psi1 = vars[psi_start + t];
            AD<double> v1 = vars[v_start + t];
            AD<double> cte1 = vars[cte_start + t];
            AD<double> epsi1 = vars[epsi_start + t];

            // The state at time t.
            AD<double> x0 = vars[x_start + t - 1];
            AD<double> y0 = vars[y_start + t - 1];
            AD<double> psi0 = vars[psi_start + t - 1];
            AD<double> v0 = vars[v_start + t - 1];
            AD<double> cte0 = vars[cte_start + t - 1];
            AD<double> epsi0 = vars[epsi_start + t - 1];

            // Only consider the actuation at time t.
            AD<double> delta0 = vars[delta_start + t - 1];
            AD<double> a0 = vars[a_start + t - 1];

            // make f0 a 3rd degree polynomial
            AD<double> f0 = coeffs[0] + coeffs[1] * x0 + coeffs[2] * x0 * x0 + coeffs[3] * x0 * x0 * x0;
            AD<double> psides0 = CppAD::atan(coeffs[1] + 2 * coeffs[2] * x0 + 3 * coeffs[3] * x0 * x0);

            // Here's `x` to get you started.
            // The idea here is to constraint this value to be 0.
            //
            // Recall the equations for the model:
            // x_[t] = x[t-1] + v[t-1] * cos(psi[t-1]) * dt
            // y_[t] = y[t-1] + v[t-1] * sin(psi[t-1]) * dt
            // psi_[t] = psi[t-1] + v[t-1] / Lf * delta[t-1] * dt
            // v_[t] = v[t-1] + a[t-1] * dt
            // cte[t] = f(x[t-1]) - y[t-1] + v[t-1] * sin(epsi[t-1]) * dt
            // epsi[t] = psi[t] - psides[t-1] + v[t-1] * delta[t-1] / Lf * dt
            fg[1 + x_start + t] = x1 - (x0 + v0 * CppAD::cos(psi0) * dt);
            fg[1 + y_start + t] = y1 - (y0 + v0 * CppAD::sin(psi0) * dt);
            fg[1 + psi_start + t] = psi1 - (psi0 + v0 * delta0 / Lf * dt);
            fg[1 + v_start + t] = v1 - (v0 + a0 * dt);
            fg[1 + cte_start + t] =
                    cte1 - ((f0 - y0) + (v0 * CppAD::sin(epsi0) * dt));
            fg[1 + epsi_start + t] =
                    epsi1 - ((psi0 - psides0) + v0 * delta0 / Lf * dt);
        }
    }
};

//
// MPC class definition implementation.
//
MPC::MPC() {}

MPC::~MPC() {}

Solution MPC::Solve(Eigen::VectorXd state, Eigen::VectorXd coeffs) {
    bool ok = true;
    typedef CPPAD_TESTVECTOR(double) Dvector;

    // TODO: Set the number of model variables (includes both states and inputs).
    // For example: If the state is a 4 element vector, the actuators is a 2
    // element vector and there are 10 timesteps. The number of variables is:
    //
    // 4 * 10 + 2 * 9
    size_t n_vars = STATE_VECTOR_SIZE * N + ACTUATOR_VECTOR_SIZE * (N - 1);

    // TODO: Set the number of constraints
    size_t n_constraints = STATE_VECTOR_SIZE * N;

    // Initial value of the independent variables.
    // SHOULD BE 0 besides initial state.
    Dvector vars(n_vars);
    for (int i = 0; i < n_vars; i++) {
        vars[i] = 0;
    }

    // Set the initial variable values
    vars[x_start] = state[0];
    vars[y_start] = state[1];
    vars[psi_start] = state[2];
    vars[v_start] = state[3];
    vars[cte_start] = state[4];
    vars[epsi_start] = state[5];

    Dvector vars_lowerbound(n_vars);
    Dvector vars_upperbound(n_vars);
    // TODO: Set lower and upper limits for variables.

    // Set limits for all non actuators vars.
    for (int i = 0; i < delta_start; ++i) {
        vars_lowerbound[i] = -1.0e19;
        vars_upperbound[i] = 1.0e19;
    }

    // delta = [-25, 25] degrees. values are in radians.
    for (int i = delta_start; i < a_start; i++) {
        vars_lowerbound[i] = -0.436332;
        vars_upperbound[i] = 0.436332;
    }

    // acceleration = [-1, 1].
    for (int i = a_start; i < n_vars; i++) {
        vars_lowerbound[i] = -1.0;
        vars_upperbound[i] =  1.0;
    }

    // Lower and upper limits for the constraints
    // Should be 0 besides initial state.
    Dvector constraints_lowerbound(n_constraints);
    Dvector constraints_upperbound(n_constraints);
    for (int i = 0; i < n_constraints; i++) {
        constraints_lowerbound[i] = 0;
        constraints_upperbound[i] = 0;
    }

    constraints_lowerbound[x_start] = vars[x_start];
    constraints_lowerbound[y_start] = vars[y_start];
    constraints_lowerbound[psi_start] = vars[psi_start];
    constraints_lowerbound[v_start] = vars[v_start];
    constraints_lowerbound[cte_start] = vars[cte_start];
    constraints_lowerbound[epsi_start] = vars[epsi_start];

    constraints_upperbound[x_start] = vars[x_start];
    constraints_upperbound[y_start] = vars[y_start];
    constraints_upperbound[psi_start] = vars[psi_start];
    constraints_upperbound[v_start] = vars[v_start];
    constraints_upperbound[cte_start] = vars[cte_start];
    constraints_upperbound[epsi_start] = vars[epsi_start];

    // object that computes objective and constraints
    FG_eval fg_eval(coeffs);

    //
    // NOTE: You don't have to worry about these options
    //
    // options for IPOPT solver
    std::string options;
    // Uncomment this if you'd like more print information
    options += "Integer print_level  0\n";
    // NOTE: Setting sparse to true allows the solver to take advantage
    // of sparse routines, this makes the computation MUCH FASTER. If you
    // can uncomment 1 of these and see if it makes a difference or not but
    // if you uncomment both the computation time should go up in orders of
    // magnitude.
    options += "Sparse  true        forward\n";
    options += "Sparse  true        reverse\n";
    // NOTE: Currently the solver has a maximum time limit of 0.5 seconds.
    // Change this as you see fit.
    options += "Numeric max_cpu_time          0.5\n";

    // place to return solution
    CppAD::ipopt::solve_result<Dvector> solution;

    // solve the problem
    CppAD::ipopt::solve<Dvector, FG_eval>(
            options, vars, vars_lowerbound, vars_upperbound, constraints_lowerbound,
            constraints_upperbound, fg_eval, solution);

    // Check some of the solution values
    ok &= solution.status == CppAD::ipopt::solve_result<Dvector>::success;

    // Cost
    auto cost = solution.obj_value;
    std::cout << "Cost " << cost << std::endl;

    // TODO: Return the first actuator values. The variables can be accessed with
    // `solution.x[i]`.
    //
    // {...} is shorthand for creating a vector, so auto x1 = {1.0,2.0}
    // creates a 2 element double vector.


    Solution soln;
    // Skip the first actuator commands and push the second actuator command into the solution vector.
    // This is how we take into consideration of latency of 100ms. Since dt = 100 ms, we can skip the first actuation
    // to take into consideration a delay of 100ms.

    soln.delta_actuation = solution.x[delta_start + 1];
    soln.a_actuation = solution.x[a_start + 1];

    for (int i = 0; i < N - 1; ++i) {
        soln.x_points.push_back(solution.x[x_start + i]);
        soln.y_points.push_back(solution.x[y_start + i]);
    }

    return soln;
}
