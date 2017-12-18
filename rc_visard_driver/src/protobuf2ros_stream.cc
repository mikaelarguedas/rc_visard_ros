/*
 * Copyright (c) 2017 Roboception GmbH
 * All rights reserved
 *
 * Author: Christian Emmerich
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "protobuf2ros_stream.h"

#include "publishers/protobuf2ros_publisher.h"
#include "publishers/protobuf2ros_conversions.h"

#include <ros/ros.h>
#include <tf/transform_broadcaster.h>
#include <visualization_msgs/Marker.h>
#include <nav_msgs/Odometry.h>

#include <rc_dynamics_api/unexpected_receive_timeout.h>


using namespace std;

namespace rc
{

namespace rcd = dynamics;


bool Protobuf2RosStream::startReceivingAndPublishingAsRos()
{
  unsigned int timeoutMillis = 500;
  string pbMsgType = _rcdyn->getPbMsgTypeOfStream(_stream);

  Protobuf2RosPublisher rosPublisher(_nh, _stream, pbMsgType, _tfPrefix);

  unsigned int cntNoListener = 0;
  bool failed = false;

  while (!_stop && !failed)
  {

    // start streaming only if someone is listening

    if (!rosPublisher.used())
    {
      // ros getNumSubscribers usually takes a while to register the subscribers
      if (++cntNoListener > 200)
      {
        _requested = false;
      }
      usleep(1000 * 10);
      continue;
    }
    cntNoListener = 0;
    _requested = true;
    _success = false;
    ROS_INFO_STREAM("rc_visard_driver: Enabled rc-dynamics stream: " << _stream);

    // initialize data stream to this host

    rcd::DataReceiver::Ptr receiver;
    try
    {
      receiver = _rcdyn->createReceiverForStream(_stream);
    }
    catch (rcd::UnexpectedReceiveTimeout& e) {
      stringstream msg;
      msg << "Could not initialize rc-dynamics stream: " << _stream << ":" << endl << e.what();
      ROS_WARN_STREAM_THROTTLE(5, msg.str());
      continue;
    }
    catch (exception &e)
    {
      ROS_ERROR_STREAM(std::string("Could not initialize rc-dynamics stream: ")
                       + _stream + ": " + e.what());
      failed = true;
      break;
    }
    receiver->setTimeout(timeoutMillis);
    ROS_INFO_STREAM("rc_visard_driver: rc-dynamics stream ready: " << _stream);

    // main loop for listening, receiving and republishing data to ROS

    shared_ptr<::google::protobuf::Message> pbMsg;
    while (!_stop && rosPublisher.used())
    {

      // try receive msg; blocking call (timeout)
      try
      {
        pbMsg = receiver->receive(pbMsgType);
        _success = true;
      }
      catch (std::exception &e)
      {
        ROS_ERROR_STREAM("Caught exception during receiving "
                                 << _stream << ": " << e.what());
        failed = true;
        break; // stop receiving loop
      }

      // timeout happened; if its only small let's put a warning and increasing recv timeout
      if (!pbMsg)
      {
        ROS_ERROR_STREAM("Did not receive any "
                                 << _stream << " message within the last "
                                 << timeoutMillis << " ms. "
                                 << "Either rc_visard stopped streaming or is turned off, "
                                 << "or you seem to have serious network/connection problems!");
        failed = true;
        break; // stop receiving loop
      }

      ROS_DEBUG_STREAM_THROTTLE(1, "Received protobuf message: "
              << pbMsg->DebugString());

      // convert to ROS message and publish
      rosPublisher.publish(pbMsg);
    }

    ROS_INFO_STREAM("rc_visard_driver: Disabled rc-dynamics stream: " << _stream);
  }

  // return info about stream being stopped externally or failed internally
  return !failed;
}


bool PoseStream::startReceivingAndPublishingAsRos()
{
  unsigned int timeoutMillis = 500;

  ros::Publisher pub = _nh.advertise<geometry_msgs::PoseStamped>(_stream, 1000);
  tf::TransformBroadcaster tf_pub;

  unsigned int cntNoListener = 0;
  bool failed = false;

  while (!_stop && !failed)
  {
    bool someoneListens = _tfEnabled || pub.getNumSubscribers() > 0;

    // start streaming only if someone is listening

    if (!someoneListens)
    {
      // ros getNumSubscribers usually takes a while to register the subscribers
      if (++cntNoListener > 200)
      {
        _requested = false;
      }
      usleep(1000 * 10);
      continue;
    }
    cntNoListener = 0;
    _requested = true;
    _success = false;
    ROS_INFO_STREAM("rc_visard_driver: Enabled rc-dynamics stream: " << _stream);

    // initialize data stream to this host

    rcd::DataReceiver::Ptr receiver;
    try
    {
      receiver = _rcdyn->createReceiverForStream(_stream);
    }
    catch (rcd::UnexpectedReceiveTimeout& e) {
      stringstream msg;
      msg << "Could not initialize rc-dynamics stream: " << _stream << ":" << endl << e.what();
      ROS_WARN_STREAM_THROTTLE(5, msg.str());
      continue;
    }
    catch (exception &e)
    {
      ROS_ERROR_STREAM(std::string("Could not initialize rc-dynamics stream: ")
                       + _stream + ": " + e.what());
      failed = true;
      break;
    }
    receiver->setTimeout(timeoutMillis);
    ROS_INFO_STREAM("rc_visard_driver: rc-dynamics stream ready: " << _stream);

    // main loop for listening, receiving and republishing data to ROS

    shared_ptr <roboception::msgs::Frame> protoFrame;
    while (!_stop && someoneListens)
    {

      // try receive msg; blocking call (timeout)
      try
      {
        protoFrame = receiver->receive<roboception::msgs::Frame>();
        _success = true;
      }
      catch (std::exception &e)
      {
        ROS_ERROR_STREAM("Caught exception during receiving "
                                 << _stream << ": " << e.what());
        failed = true;
        break; // stop receiving loop
      }

      // timeout happened; if its only small let's put a warning and increasing recv timeout
      if (!protoFrame)
      {
        ROS_ERROR_STREAM("Did not receive any "
                                 << _stream << " message within the last "
                                 << timeoutMillis << " ms. "
                                 << "Either rc_visard stopped streaming or is turned off, "
                                 << "or you seem to have serious network/connection problems!");
        failed = true;
        break; // stop receiving loop
      }

      ROS_DEBUG_STREAM_THROTTLE(1, "Received protoFrame: "
              << protoFrame->DebugString());


      // overwrite frame name/ids
      protoFrame->set_parent(_tfPrefix+protoFrame->parent());
      protoFrame->set_name(_tfPrefix+protoFrame->name());

      auto rosPose = toRosPoseStamped(*protoFrame);
      pub.publish(rosPose);

      // convert to tf and publish
      if (_tfEnabled)
      {
        tf::StampedTransform transform = toRosTfStampedTransform(*protoFrame);
        tf_pub.sendTransform(transform);
      }

      // check if still someone is listening
      someoneListens = _tfEnabled || pub.getNumSubscribers() > 0;
    }

    ROS_INFO_STREAM("rc_visard_driver: Disabled rc-dynamics stream: " << _stream);
  }

  // return info about stream being stopped externally or failed internally
  return !failed;
}



bool DynamicsStream::startReceivingAndPublishingAsRos()
{
  unsigned int timeoutMillis = 500;

  ros::Publisher pub_pose = _nh.advertise<geometry_msgs::PoseStamped>(_stream, 1000);
  ros::Publisher pub_markers = _nh.advertise<visualization_msgs::Marker>("visualization_marker", 1000);
  ros::Publisher pub_odom = _nh.advertise<nav_msgs::Odometry>("odometry", 1000);
  tf::TransformBroadcaster tf_pub;

  unsigned int cntNoListener = 0;
  bool failed = false;

  while (!_stop && !failed)
  {
    bool someoneListens = pub_pose.getNumSubscribers() > 0 ||
            pub_markers.getNumSubscribers() > 0 ||
            pub_odom.getNumSubscribers() > 0;

    // start streaming only if someone is listening

    if (!someoneListens)
    {
      // ros getNumSubscribers usually takes a while to register the subscribers
      if (++cntNoListener > 200)
      {
        _requested = false;
      }
      usleep(1000 * 10);
      continue;
    }
    cntNoListener = 0;
    _requested = true;
    _success = false;
    ROS_INFO_STREAM("rc_visard_driver: Enabled rc-dynamics stream: " << _stream);

    // initialize data stream to this host

    rcd::DataReceiver::Ptr receiver;
    try
    {
      receiver = _rcdyn->createReceiverForStream(_stream);
    }
    catch (rcd::UnexpectedReceiveTimeout& e) {
      stringstream msg;
      msg << "Could not initialize rc-dynamics stream: " << _stream << ":" << endl << e.what();
      ROS_WARN_STREAM_THROTTLE(5, msg.str());
      continue;
    }
    catch (exception &e)
    {
      ROS_ERROR_STREAM(std::string("Could not initialize rc-dynamics stream: ")
                       + _stream + ": " + e.what());
      failed = true;
      break;
    }
    receiver->setTimeout(timeoutMillis);
    ROS_INFO_STREAM("rc_visard_driver: rc-dynamics stream ready: " << _stream);

    // main loop for listening, receiving and republishing data to ROS

    shared_ptr <roboception::msgs::Dynamics> protoMsg;
    while (!_stop && someoneListens)
    {

      // try receive msg; blocking call (timeout)
      try
      {
        protoMsg = receiver->receive<roboception::msgs::Dynamics>();
        _success = true;
      }
      catch (std::exception &e)
      {
        ROS_ERROR_STREAM("Caught exception during receiving "
                                 << _stream << ": " << e.what());
        failed = true;
        break; // stop receiving loop
      }

      // timeout happened; if its only small let's put a warning and increasing recv timeout
      if (!protoMsg)
      {
        ROS_ERROR_STREAM("Did not receive any "
                                 << _stream << " message within the last "
                                 << timeoutMillis << " ms. "
                                 << "Either rc_visard stopped streaming or is turned off, "
                                 << "or you seem to have serious network/connection problems!");
        failed = true;
        break; // stop receiving loop
      }

      ROS_DEBUG_STREAM_THROTTLE(1, "Received protoMsg: "
              << protoMsg->DebugString());


      // prefix all frame ids before
      protoMsg->set_pose_frame(_tfPrefix+protoMsg->pose_frame());
      protoMsg->set_linear_velocity_frame(_tfPrefix+protoMsg->linear_velocity_frame());
      protoMsg->set_angular_velocity_frame(_tfPrefix+protoMsg->angular_velocity_frame());
      protoMsg->set_linear_acceleration_frame(_tfPrefix+protoMsg->linear_acceleration_frame());
      protoMsg->mutable_cam2imu_transform()->set_name(_tfPrefix+protoMsg->cam2imu_transform().name());
      protoMsg->mutable_cam2imu_transform()->set_parent(_tfPrefix+protoMsg->cam2imu_transform().parent());

      // time stamp of this message
      ros::Time msgStamp = toRosTime(protoMsg->timestamp());

      // create PoseStamped and send out
      pub_pose.publish(toRosPoseStamped(protoMsg->pose(), protoMsg->timestamp(), protoMsg->pose_frame()));

      // convert cam2imu_transform to tf and publish it
      auto cam2imu_tf = toRosTfStampedTransform(protoMsg->cam2imu_transform());
      // from dynamics-module we get camera-pose in imu-frame, but we want to have imu-pose in camera frame
      auto cam2imu_tf_inv = tf::StampedTransform(cam2imu_tf.inverse(), msgStamp, cam2imu_tf.child_frame_id_, cam2imu_tf.frame_id_);
      tf_pub.sendTransform(cam2imu_tf_inv);

      // convert velocities and accelerations to visualization Markers
      geometry_msgs::Point start, end;
      auto protoPosePosition = protoMsg->pose().position();
      start.x = protoPosePosition.x();
      start.y = protoPosePosition.y();
      start.z = protoPosePosition.z();

      visualization_msgs::Marker lin_vel_marker; // single marker for linear velocity
      lin_vel_marker.header.stamp = msgStamp;
      lin_vel_marker.header.frame_id = protoMsg->linear_velocity_frame();
      lin_vel_marker.ns = _tfPrefix;
      lin_vel_marker.id = 0;
      lin_vel_marker.type = visualization_msgs::Marker::ARROW;
      lin_vel_marker.action = visualization_msgs::Marker::MODIFY;
      lin_vel_marker.frame_locked = true;
      auto lin_vel = protoMsg->linear_velocity();
      end.x = start.x + lin_vel.x();
      end.y= start.y + lin_vel.y();
      end.z = start.z + lin_vel.z();
      lin_vel_marker.points.push_back(start);
      lin_vel_marker.points.push_back(end);
      lin_vel_marker.scale.x = 0.005;
      lin_vel_marker.scale.y = 0.01;
      lin_vel_marker.color.a = 1;
      lin_vel_marker.color.g = lin_vel_marker.color.b = 1.0; // cyan
      pub_markers.publish(lin_vel_marker);

      // convert pose to transform to use it for transformations
      auto pose_tf = toRosTfTransform(protoMsg->pose());
      pose_tf.setOrigin(tf::Vector3(0.0, 0.0, 0.0));

      visualization_msgs::Marker ang_vel_marker; // single marker for linear velocity
      ang_vel_marker.header.stamp = msgStamp;
      ang_vel_marker.header.frame_id = protoMsg->pose_frame();
      ang_vel_marker.ns = _tfPrefix;
      ang_vel_marker.id = 1;
      ang_vel_marker.type = visualization_msgs::Marker::ARROW;
      ang_vel_marker.action = visualization_msgs::Marker::MODIFY;
      ang_vel_marker.frame_locked = true;
      auto ang_vel = protoMsg->angular_velocity();
      auto ang_vel_transformed = pose_tf*tf::Vector3(ang_vel.x(), ang_vel.y(), ang_vel.z());
      end.x = start.x + ang_vel_transformed.x();
      end.y= start.y + ang_vel_transformed.y();
      end.z = start.z + ang_vel_transformed.z();
      ang_vel_marker.points.push_back(start);
      ang_vel_marker.points.push_back(end);
      ang_vel_marker.scale.x = 0.005;
      ang_vel_marker.scale.y = 0.01;
      ang_vel_marker.color.a = 1;
      ang_vel_marker.color.r = ang_vel_marker.color.b = 1.0; // mangenta
      pub_markers.publish(ang_vel_marker);

      visualization_msgs::Marker lin_accel_marker; // single marker for linear acceleration
      lin_accel_marker.header.stamp = msgStamp;
      lin_accel_marker.header.frame_id = protoMsg->pose_frame();
      lin_accel_marker.ns = _tfPrefix;
      lin_accel_marker.id = 2;
      lin_accel_marker.type = visualization_msgs::Marker::ARROW;
      lin_accel_marker.action = visualization_msgs::Marker::MODIFY;
      lin_accel_marker.frame_locked = true;
      auto lin_accel = protoMsg->linear_acceleration();
      auto lin_accel_transformed = pose_tf*tf::Vector3(lin_accel.x(), lin_accel.y(), lin_accel.z());
      end.x = start.x + lin_accel_transformed.x();
      end.y= start.y + lin_accel_transformed.y();
      end.z = start.z + lin_accel_transformed.z();
      lin_accel_marker.points.push_back(start);
      lin_accel_marker.points.push_back(end);
      lin_accel_marker.scale.x = 0.005;
      lin_accel_marker.scale.y = 0.01;
      lin_accel_marker.color.a = 1;
      lin_accel_marker.color.r = lin_accel_marker.color.g = 1.0; // yellow
      pub_markers.publish(lin_accel_marker);

      // publish rc dynamics message as ros odometry
      // for this we need to transform lin velocity from world frame to imu
      auto lin_vel_transformed = pose_tf.inverse()*tf::Vector3(
              lin_vel.x(), lin_vel.y(), lin_vel.z());
      auto odom = boost::make_shared<nav_msgs::Odometry>();
      odom->header.frame_id = protoMsg->pose_frame(); // "world"
      odom->header.stamp = msgStamp;
      odom->child_frame_id = protoMsg->linear_acceleration_frame(); //"imu"
      odom->pose.pose = *toRosPose(protoMsg->pose());
      odom->twist.twist.linear.x = lin_vel_transformed.x();
      odom->twist.twist.linear.y = lin_vel_transformed.y();
      odom->twist.twist.linear.z = lin_vel_transformed.z();
      odom->twist.twist.angular.x = ang_vel.x();
      odom->twist.twist.angular.y = ang_vel.y();
      odom->twist.twist.angular.z = ang_vel.z();
      pub_odom.publish(odom);


      // check if still someone is listening
      someoneListens = pub_pose.getNumSubscribers() > 0 ||
                       pub_markers.getNumSubscribers() > 0 ||
                       pub_odom.getNumSubscribers() > 0;
    }

    ROS_INFO_STREAM("rc_visard_driver: Disabled rc-dynamics stream: " << _stream);
  }

  // return info about stream being stopped externally or failed internally
  return !failed;
}



}
