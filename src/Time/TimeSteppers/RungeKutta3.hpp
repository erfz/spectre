// Distributed under the MIT License.
// See LICENSE.txt for details.

/// \file
/// Defines class RungeKutta3.

#pragma once

#include <deque>
#include <string>
#include <tuple>

#include "ErrorHandling/Error.hpp"
#include "Time/Time.hpp"
#include "Time/TimeSteppers/TimeStepper.hpp"

namespace TimeSteppers {

/// \ingroup TimeSteppersGroup
///
/// A "strong stability-preserving" 3rd-order Runge-Kutta time-stepper.
/// Major reference:  J. Hesthaven & T. Warburton, Nodal Discontinuous
/// Galerkin Methods. section 5.7
class RungeKutta3 : public TimeStepper::Inherit {
 public:
  //using OptionsList = tmpl::list<>;
  //static std::string class_id() { return "RungeKutta3"; }
  //static constexpr OptionString_t help = {
  //    "A third-order strong stability-preserving Runge-Kutta time-stepper."};

  RungeKutta3() noexcept = default;
  RungeKutta3(const RungeKutta3&) noexcept = default;
  RungeKutta3& operator=(const RungeKutta3&) noexcept = default;
  RungeKutta3(RungeKutta3&&) noexcept = default;
  RungeKutta3& operator=(RungeKutta3&&) noexcept = default;
  ~RungeKutta3() noexcept override = default;

  template <typename Vars, typename DerivVars>
  TimeDelta update_u(
      Vars& u,
      const std::deque<std::tuple<Time, Vars, DerivVars>>& history,
      const TimeDelta& time_step) const noexcept;

  size_t number_of_substeps() const noexcept override;

  size_t number_of_past_steps() const noexcept override;

  bool is_self_starting() const noexcept override;

  double stable_step() const noexcept override;
};

template <typename Vars, typename DerivVars>
TimeDelta RungeKutta3::update_u(
    Vars& u,
    const std::deque<std::tuple<Time, Vars, DerivVars>>& history,
    const TimeDelta& time_step) const noexcept {
  const size_t substep = history.size() - 1;
  const auto& vars = std::get<1>(history.back());
  const auto& dt_vars = std::get<2>(history.back());
  const auto& U0 = std::get<1>(history[0]);

  switch (substep) {
    case 0: {
      // from (5.32) of Hesthaven
      // v^(1) = u^n + dt*RHS(u^n,t^n)
      // On entry V = u^n, U0 = u^n, rhs0 = RHS(u^n,t^n),
      // time = t^n
      u += time_step.value() * dt_vars;
      return time_step;
      // On exit v = v^(1), time = t^n + dt
    }
    case 1: {
      // from (5.32) of Hesthaven
      // v^(2) = (1/4)*( 3*u^n + v^(1) + dt*RHS(v^(1),t^n + dt) )
      // On entry V = v^(1), U0 = u^n, rhs0 = RHS(v^(1),t^n + dt),
      // time = t^n + dt
      u += 0.25 * (3.0 * (U0 - vars) + time_step.value() * dt_vars);
      return -time_step / 2;
      // On exit v = v^(2), time = t^n + (1/2)*dt
    }
    case 2: {
      // from (5.32) of Hesthaven
      // u^(n+1) = (1/3)*( u^n + 2*v^(2) + 2*dt*RHS(v^(2),t^n + (1/2)*dt) )
      // On entry V = v^(2), U0 = u^n, rhs0 = RHS(v^(2),t^n + (1/2)*dt),
      // time = t^n + (1/2)*dt
      u += (1.0 / 3.0) * (U0 - vars + 2.0 * time_step.value() * dt_vars);
      return time_step / 2;
      // On exit v = u^(n+1), time = t^n + dt
    }
    default:
      ERROR("Bad substep value in RK3: " << substep);
  }
}

}  // namespace TimeSteppers
