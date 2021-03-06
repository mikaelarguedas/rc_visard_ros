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

#ifndef RC_DISPARITYCOLORPUBLISHER_H
#define RC_DISPARITYCOLORPUBLISHER_H

#include "genicam2ros_publisher.h"

#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <sensor_msgs/Image.h>

namespace rc
{

class DisparityColorPublisher : public GenICam2RosPublisher
{
  public:

    /**
      Initialization of publisher.

      @param it    Image transport handle.
      @param left  True for left and false for right camera.
      @param color True for sending color instead of monochrome images.
    */

    DisparityColorPublisher(image_transport::ImageTransport &it, std::string frame_id_prefix, double scale);

    /**
      Set the disparity range for scaling of images.

      @param disprange Disparity range for scaling.
    */

    void setDisprange(int disprange);

    bool used() override;

    void publish(const rcg::Buffer *buffer, uint64_t pixelformat) override;

  private:

    DisparityColorPublisher(const DisparityColorPublisher &); // forbidden
    DisparityColorPublisher &operator=(const DisparityColorPublisher &); // forbidden

    uint32_t seq;
    double   scale;
    int      disprange;

    image_transport::Publisher pub;
};

}

#endif
