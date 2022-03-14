#pragma once

// ROS
#include "ros/ros.h"

// ROS messages
#include <mavros_msgs/AttitudeTarget.h>
#include <nav_msgs/Odometry.h>

#include "px4_control_msgs/DroneState.h"

// Eigen
#include <eigen3/Eigen/Dense>

// Sensors
#include "state_estimation/sensors/mavros_odometry_eskf.h"

// Common
#include "state_estimation/common.hpp"

/**
 * @brief State Observer Class. Implements an EKF to calculate an estimation of
 * the drone state and external disturbances
 * TODO: Add mutexes to make sure that the inputs/states won't change while they
 * are being used
 */
namespace px4_ctrl {
class StateObserver {
 public:
  StateObserver(ros::NodeHandle &nh);
  ~StateObserver();

 private:
  // ROS Subscribers
  ros::Subscriber odom_sub;
  ros::Subscriber ctrl_sub;

  // ROS Publishers
  ros::Publisher state_pub;

  // ROS Services
  ros::ServiceClient reset_filter;

  // Callbacks
  void odomCallback(const nav_msgs::Odometry &msg);
  void ctrlCallback(const mavros_msgs::AttitudeTarget &msg);

  /** @brief Loads the model parameters
   */
  void loadParameters();

  /**
   * @brief Runs the observer's filter prediction step. Updates state_pred,
   * F_mat and P_pred_mat
   * @param pred_time Time for prediction
   */
  void predict(ros::Time pred_time);

  /**
   * @brief Calculates the system's derivative at a specific state and input
   * @param state System state
   * @param input Input to the system
   * @returns System's derivative
   */
  Eigen::VectorXd getSystemDerivative(const eskf_state &state,
                                      const Eigen::Vector4d &cmd);

  /**
   * @brief Adds the update to the state
   * @param state_update System's state update
   * @returns Updated state
   */
  eskf_state addUpdate(const eskf_state &state,
                       const Eigen::VectorXd &state_update);

  /** @brief Updates the P_pred_mat for the prediction step
   * @param dt Time step for prediction
   * @param cmd The input for the prediction step
   */
  void updatePpred(const double &dt, const Eigen::Vector4d &cmd);

  /**
   * @brief Updates the system state using the estimated state
   */
  void updateState();

  /**
   * @brief Publishes the current state estimate
   * @param time Timestamp for the current state
   * TODO: Publish covariance as well
   */
  void publishState(ros::Time time);

  // Observer data
  ros::Time past_state_time;
  bool is_initialized;
  static const int state_size = 13;
  static const int error_state_size = 12;
  static const int odom_size = 10;

  // state = [x, y, z, xdot, ydot, zdot, qw, qx, qy, qz, fdx, fdy, fdz]T
  // error_state = [dpos, dvel, dtheta, dfd]T
  eskf_state state, state_pred;
  Eigen::Matrix<double, error_state_size, error_state_size> Q_mat;
  Eigen::Matrix<double, error_state_size, error_state_size> P_mat;
  Eigen::Matrix<double, error_state_size, error_state_size> P_pred_mat;
  Eigen::Matrix<double, error_state_size, 1> error_state;

  // input = [yaw_rate, pitch, roll, thrust]T
  Eigen::Vector4d past_cmd, latest_cmd;
  ros::Time latest_cmd_time;

  // Model parameters
  Eigen::Vector3d gravity_vector;
  Eigen::Matrix3d damping_matrix;
  double t_pitch, k_pitch, t_roll, k_roll;
  double damp_x, damp_y, damp_z;
  double k_thrust;
  double gravity;
  double disturbance_limit;

  // Sensors
  MavrosOdometry *odom_sensor;
};
}  // namespace px4_ctrl