#ifndef _AIMBOT_CONFIG_H_
#define _AIMBOT_CONFIG_H_

#include <opencv2/opencv.hpp>

#define NET_FILE      "model/resnet18.prototxt"
#define PARAM_FILE    "model/armor_filter.caffemodel"

/* Constants */
#define rad_2_deg 57.2957795

/* Constants based on measurement and devices */
#define LIGHT_BAR_HEIGHT_ONE_METER 40 // measured in 1080p

/* threshold of avg value of blue channel of a specific ROI area. ROIs that hold a lower value will be ignored */
#define blue_armor_avgcolor_threshold 220
#define red_armor_avgcolor_threshold  220

/* general color/gray distillation threshold */
#define gray_threshold 200
#define blue_threshold 90
#define red_threshold  50

/* contour method and mode. This may affect performance speed */
#define cnt_method cv::CHAIN_APPROX_NONE
#define cnt_mode   cv::RETR_EXTERNAL

/* light detection constraint parameters */

    /* Distilled color config*/
#define light_bar_kernel_height 6
#define light_bar_kernel_width  6
#define light_bar_kernel_iter   1

    /* Binarized gray img config */
#define gray_bin_kernel_height  2
#define gray_bin_kernel_width   2

    /* Light bar filtering config */
#define light_bar_min_area      3.0
#define light_max_aspect_ratio  5.0
#define light_min_apsect_ratio  0.8
#define light_max_angle         40.0
#define light_min_area          4.0

/* armpr detection constraint */
#define light_max_angle_diff   30.0
#define armor_max_angle        20.0
#define armor_max_aspect_ratio 3.0
#define armor_min_area         35.0

#endif
