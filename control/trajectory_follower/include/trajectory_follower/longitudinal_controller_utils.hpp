// Copyright 2018-2021 Tier IV, Inc. All rights reserved.
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

#ifndef TRAJECTORY_FOLLOWER__LONGITUDINAL_CONTROLLER_UTILS_HPP_
#define TRAJECTORY_FOLLOWER__LONGITUDINAL_CONTROLLER_UTILS_HPP_

#include "common/types.hpp"
#include "eigen3/Eigen/Core"
#include "eigen3/Eigen/Geometry"
#include "geometry/common_2d.hpp"
#include "interpolation/linear_interpolation.hpp"
#include "motion_utils/trajectory/trajectory.hpp"
#include "tf2/utils.h"
#include "trajectory_follower/visibility_control.hpp"

#include <experimental/optional>  // NOLINT

#include "autoware_auto_planning_msgs/msg/trajectory.hpp"
#include "geometry_msgs/msg/pose.hpp"

#include <cmath>
#include <limits>

namespace autoware
{
namespace motion
{
namespace control
{
namespace trajectory_follower
{
namespace longitudinal_utils
{
using autoware::common::types::bool8_t;
using autoware::common::types::float64_t;
using autoware_auto_planning_msgs::msg::Trajectory;
using autoware_auto_planning_msgs::msg::TrajectoryPoint;
using geometry_msgs::msg::Point;
using geometry_msgs::msg::Pose;
using geometry_msgs::msg::Quaternion;

/**
 * @brief check if trajectory is invalid or not
 */
TRAJECTORY_FOLLOWER_PUBLIC bool8_t isValidTrajectory(const Trajectory & traj);

/**
 * @brief calculate distance to stopline from current vehicle position where velocity is 0
 */
TRAJECTORY_FOLLOWER_PUBLIC float64_t calcStopDistance(
  const Pose & current_pose, const Trajectory & traj, const float64_t max_dist,
  const float64_t max_yaw);

/**
 * @brief calculate pitch angle from estimated current pose
 */
TRAJECTORY_FOLLOWER_PUBLIC float64_t getPitchByPose(const Quaternion & quaternion);

/**
 * @brief calculate pitch angle from trajectory on map
 * NOTE: there is currently no z information so this always returns 0.0
 * @param [in] trajectory input trajectory
 * @param [in] closest_idx nearest index to current vehicle position
 * @param [in] wheel_base length of wheel base
 */
TRAJECTORY_FOLLOWER_PUBLIC float64_t
getPitchByTraj(const Trajectory & trajectory, const size_t closest_idx, const float64_t wheel_base);

/**
 * @brief calculate elevation angle
 */
TRAJECTORY_FOLLOWER_PUBLIC float64_t
calcElevationAngle(const TrajectoryPoint & p_from, const TrajectoryPoint & p_to);

/**
 * @brief calculate vehicle pose after time delay by moving the vehicle at current velocity for
 * delayed time
 */
TRAJECTORY_FOLLOWER_PUBLIC Pose calcPoseAfterTimeDelay(
  const Pose & current_pose, const float64_t delay_time, const float64_t current_vel);

/**
 * @brief apply linear interpolation to orientation
 * @param [in] o_from first orientation
 * @param [in] o_to second orientation
 * @param [in] ratio ratio between o_from and o_to for interpolation
 */
TRAJECTORY_FOLLOWER_PUBLIC Quaternion
lerpOrientation(const Quaternion & o_from, const Quaternion & o_to, const float64_t ratio);

/**
 * @brief apply linear interpolation to trajectory point that is nearest to a certain point
 * @param [in] points trajectory points
 * @param [in] point Interpolated point is nearest to this point.
 */
template <class T>
TRAJECTORY_FOLLOWER_PUBLIC TrajectoryPoint lerpTrajectoryPoint(
  const T & points, const Pose & pose, const float64_t max_dist, const float64_t max_yaw)
{
  TrajectoryPoint interpolated_point;
  const size_t seg_idx =
    motion_utils::findFirstNearestSegmentIndexWithSoftConstraints(points, pose, max_dist, max_yaw);

  const float64_t len_to_interpolated =
    motion_utils::calcLongitudinalOffsetToSegment(points, seg_idx, pose.position);
  const float64_t len_segment = motion_utils::calcSignedArcLength(points, seg_idx, seg_idx + 1);
  const float64_t interpolate_ratio = std::clamp(len_to_interpolated / len_segment, 0.0, 1.0);

  {
    const size_t i = seg_idx;

    interpolated_point.pose.position.x = interpolation::lerp(
      points.at(i).pose.position.x, points.at(i + 1).pose.position.x, interpolate_ratio);
    interpolated_point.pose.position.y = interpolation::lerp(
      points.at(i).pose.position.y, points.at(i + 1).pose.position.y, interpolate_ratio);
    interpolated_point.pose.orientation = lerpOrientation(
      points.at(i).pose.orientation, points.at(i + 1).pose.orientation, interpolate_ratio);
    interpolated_point.longitudinal_velocity_mps = interpolation::lerp(
      points.at(i).longitudinal_velocity_mps, points.at(i + 1).longitudinal_velocity_mps,
      interpolate_ratio);
    interpolated_point.lateral_velocity_mps = interpolation::lerp(
      points.at(i).lateral_velocity_mps, points.at(i + 1).lateral_velocity_mps, interpolate_ratio);
    interpolated_point.acceleration_mps2 = interpolation::lerp(
      points.at(i).acceleration_mps2, points.at(i + 1).acceleration_mps2, interpolate_ratio);
    interpolated_point.heading_rate_rps = interpolation::lerp(
      points.at(i).heading_rate_rps, points.at(i + 1).heading_rate_rps, interpolate_ratio);
  }

  return interpolated_point;
}

/**
 * @brief limit variable whose differential is within a certain value
 * @param [in] input_val current value
 * @param [in] prev_val previous value
 * @param [in] dt time between current and previous one
 * @param [in] lim_val limitation value for differential
 */
TRAJECTORY_FOLLOWER_PUBLIC float64_t applyDiffLimitFilter(
  const float64_t input_val, const float64_t prev_val, const float64_t dt, const float64_t lim_val);

/**
 * @brief limit variable whose differential is within a certain value
 * @param [in] input_val current value
 * @param [in] prev_val previous value
 * @param [in] dt time between current and previous one
 * @param [in] max_val maximum value for differential
 * @param [in] min_val minimum value for differential
 */
TRAJECTORY_FOLLOWER_PUBLIC float64_t applyDiffLimitFilter(
  const float64_t input_val, const float64_t prev_val, const float64_t dt, const float64_t max_val,
  const float64_t min_val);
}  // namespace longitudinal_utils
}  // namespace trajectory_follower
}  // namespace control
}  // namespace motion
}  // namespace autoware

#endif  // TRAJECTORY_FOLLOWER__LONGITUDINAL_CONTROLLER_UTILS_HPP_
