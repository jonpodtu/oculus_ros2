// Copyright 2023 Forssea Robotics
// All rights reserved.
//
// Unauthorized copying of this code base via any medium is strictly prohibited.
// Proprietary and confidential.

#ifndef OCULUS_ROS2__SONAR_VIEWER_HPP_
#define OCULUS_ROS2__SONAR_VIEWER_HPP_

#include <cv_bridge/cv_bridge.h>
#include <oculus_driver/AsyncService.h>
#include <oculus_driver/SonarDriver.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include <oculus_interfaces/msg/ping.hpp>
#include <oculus_ros2/conversions.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <std_msgs/msg/header.hpp>

class SonarViewer {
public:
  // SonarViewer(bool *arg);
  explicit SonarViewer(rclcpp::Node* node);
  ~SonarViewer();
  void publish_fan(const oculus::PingMessage::ConstPtr& ping, const std::string& frame_id = "", const int& data_depth = 0) const;
  void publish_fan(
      const oculus_interfaces::msg::Ping& ros_ping_msg, const std::string& frame_id = "", const int& data_depth = 0) const;
  template <typename dataType>
  void publish_fan(const int& width,
      const int& height,
      const int& offset,
      const std::vector<dataType>& ping_data,
      const int& master_mode,
      const double& ping_rage,
      const std_msgs::msg::Header& header) const;
  void stream_and_filter(const oculus::PingMessage::ConstPtr& ping, cv::Mat& data);
  // void stream_and_filter(const oculus::PingMessage::ConstPtr &ping);
  // sensor_msgs::msg::Image publish_fan(const oculus::PingMessage::ConstPtr &ping);

private:
  //      //   sensor_msgs::ImagePtr msg;
  const rclcpp::Node* node_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_publisher_;
};

template <typename T>
std::vector<double> linspace(T start_in, T end_in, int num_in) {
  std::vector<double> linspaced;

  double start = static_cast<double>(start_in);
  double end = static_cast<double>(end_in);
  double num = static_cast<double>(num_in);

  if (num == 0) {
    return linspaced;
  }
  if (num == 1) {
    linspaced.push_back(start);
    return linspaced;
  }

  double delta = (end - start) / (num - 1);

  for (int i = 0; i < num - 1; ++i) {
    linspaced.push_back(start + delta * i);
  }
  linspaced.push_back(end);  // I want to ensure that start and end
                             // are exactly the same as the input
  return linspaced;
}

template <typename dataType>
void SonarViewer::publish_fan(const int& width,
    const int& height,
    const int& offset,
    const std::vector<dataType>& ping_data,
    const int& master_mode,
    const double& ping_range,
    const std_msgs::msg::Header& header) const {
  // Create rawDataMat from ping_data
  cv::Mat rawDataMat(height, (width + 4), CV_8U);
  std::memcpy(rawDataMat.data, ping_data.data() + offset, height * (width + 4));

  int bearing = (master_mode == 1) ? 65 : 40;
  std::vector<double> ranges = linspace(0., ping_range, height);
  int image_width = 2 * std::sin(bearing * M_PI / 180) * ranges.size();
  cv::Mat rgb_img = cv::Mat::zeros(cv::Size(image_width, ranges.size()), CV_8UC3);

  for (int i = 0; i < image_width; i++) {
    for (int j = 0; j < ranges.size(); j++) {
      rgb_img.at<cv::Vec3b>(j, i) = cv::Vec3b(255, 255, 255);
    }
  }

  const float ThetaShift = 1.5 * 180;
  const cv::Point origin(image_width / 2, ranges.size());

  for (int r = 0; r < ranges.size(); r++) {
    std::vector<cv::Point> pts;
    cv::ellipse2Poly(origin, cv::Size(r, r), ThetaShift, -bearing, bearing, 1, pts);

    std::vector<cv::Point> arc_points;
    arc_points.push_back(pts[0]);

    for (size_t k = 0; k < (pts.size() - 1); k++) {  // TODO
      cv::LineIterator it(rgb_img, pts[k], pts[k + 1], 4);
      for (int i = 1; i < it.count; i++, ++it) arc_points.push_back(it.pos());
    }

    cv::Mat data_rows_resized;
    cv::resize(rawDataMat.row(r), data_rows_resized, cv::Size(arc_points.size(), arc_points.size()));

    for (size_t k = 0; k < arc_points.size(); k++)
      rgb_img.at<cv::Vec3b>(arc_points[k]) = cv::Vec3b(0, data_rows_resized.at<dataType>(1, k), 0);
  }

  // cv::Mat yuv_img;
  // cv::cvtColor(rgb_img, yuv_img, cv::COLOR_BGR2YUV_I420);

  // Publish sonar conic image
  sensor_msgs::msg::Image msg;
  const char* encoding =
      std::is_same<dataType, uint8_t>::value ? sensor_msgs::image_encodings::BGR8 : sensor_msgs::image_encodings::BGR16;
  cv_bridge::CvImage(header, encoding, rgb_img).toImageMsg(msg);
  //   cv_bridge::CvImage(std_msgs::msg::Header(), sensor_msgs::image_encodings::NV21, yuv_img).toImageMsg(msg);
  image_publisher_->publish(msg);
}

#endif  // OCULUS_ROS2__SONAR_VIEWER_HPP_
