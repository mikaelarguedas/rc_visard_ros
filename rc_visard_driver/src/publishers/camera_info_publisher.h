/*
 * Copyright (c) 2017 Roboception GmbH
 * All rights reserved
 *
 * Author: Heiko Hirschmueller
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

#ifndef RC_CAMERAINFOPUBLISHER_H
#define RC_CAMERAINFOPUBLISHER_H

#include "publisher.h"

#include <ros/ros.h>
#include <sensor_msgs/CameraInfo.h>

namespace rc
{

class CameraInfoPublisher : public GenICam2RosPublisher
{
  public:

    /**
      Initialization of publisher.

      @param nh   Node handle.
      @param f    Focal length, normalized to image width 1.
      @param t    Baseline in m.
      @param left True for left and false for right camera.
    */

    CameraInfoPublisher(ros::NodeHandle &nh, std::string _frame_id, double f, double t, bool left);

    bool used() override;

    void publish(const rcg::Buffer *buffer, uint64_t pixelformat) override;

  private:

    CameraInfoPublisher(const CameraInfoPublisher &); // forbidden
    CameraInfoPublisher &operator=(const CameraInfoPublisher &); // forbidden

    float f;

    sensor_msgs::CameraInfo info;
    ros::Publisher pub;
};

}

#endif