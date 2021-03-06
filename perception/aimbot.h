#ifndef AIMBOT_H_
#define AIMBOT_H_

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include "camera.h"
#include <algorithm>
#include <math.h>
#include "cv_config.h"
#include "cv_utils.h"
#include "aimbot_config.h"

using std::string;
using namespace cv;

#define MODEL_BATCH_SIZE     30
#define FEEDING_IMG_WIDTH    100
#define CLASSIFIER_THRESHOLD 0.41
#define FEEDING_IMG_HEIGHT   100

    /* Begin Common Functions */

/*
 * @brief Simple template function that return the largest variable of two variables of same type
 * @param a variable of type T
 * @param b another variable of type T
 * @return largest of the two
 */
template<class T> const T& max_of_two(const T& a, const T&b) {return (a>b)? a:b;}

    /* End Common Functions */

typedef struct {
    cv::RotatedRect armor;
    cv::RotatedRect left_light_bar;
    cv::RotatedRect right_light_bar;
} armor_t;

//@TODO: return calculated abs pitch and abs yaw (with res to calced trejectory)
typedef struct {
    float target_distance;
    float abs_pitch;
    float abs_yaw;
} aimbot_command_t;

/**
 * target of interest algorithm.
 * @brief
 * @param  tar vector of possible targets (RotatedRect, armor_t, Point2f)
 * @return Index of POI
 */
template<class T> size_t get_target(const vector<T> & ta);

/**
 * Magic function that highlights red or blue area
 * @param src_img ref to src_img
 * @param dst_img ref to an empty img
 * @param color_type string. It should be either "blue" or "red"
 */
void distill_color(const Mat & src_img, Mat & dst_img, string color_type);

/**
 * @brief helper function to draw cv::RotatedRect on Mat
 * @param mat_to_draw image to be modified
 * @param rect_to_draw rotated rectangle to be drawn
 */
void draw_rotated_rect(Mat & mat_to_draw, RotatedRect rect_to_draw);

/**
 * @brief helper function to calculate aspect ratio (y offset is considered height)
 * @param light a rotatedrect object (usually a light bar)
 * @return aspect ratio
 */
float _cal_aspect_ratio(RotatedRect light);

/**
 * @brief get POI with respect to original image size (for cropping)
 * @param crop_distilled preprocessed image
 */
Point2f _get_point_of_interest(const Mat & crop_distilled);

/**
 * @brief dynamically crop image
 */
Mat _image_cropper(const Mat & frame, Point2f poi);

/**
 * @brief make gray perspective transform of armor board for classifiers
 * @param src_img source image
 * @param roi rotatedrect to be cropped
 * @return square cropped image of FEEDING_IMG_HEIGHT * FEEDING_IMG_WIDTH
 */
Mat armor_perspective_transform(const Mat & src_img, const RotatedRect & roi);

/*
 * An abstract base class for assisted aiming
 */
class aimbot{
public:
    /**
     * constructor for generic aimbot class; initialize configuration into memory
     */
    aimbot() {
        //do nothing
    }

    /**
     * destructor for generic aimbot class;
     */
    ~aimbot(){
        //do nothing
    }

    /**
     * Process image (frame) in current video buffer; pure virtual function to be implemented
     * @return vector of cv::Rect object(s)
     */
    virtual std::vector<armor_t> get_hitboxes(CameraBase * my_cam) = 0;

private:
    /* to be added */
};

/**
 * A mod for assisted aiming; inspired by algorithm from DJI RoboRTS
 */
class ir_aimbot: public aimbot{
public:
    /**
     * constructor; initialize configuration into memory
     */
    ir_aimbot(string color_type_str);

    /**
     * class destructor
     */
    ~ir_aimbot();

    /**
     * Process image (frame) in current video buffer; pure virtual function to be implemented
     * @return vector of cv::Rect object(s)
     */
    vector<armor_t> get_hitboxes(CameraBase *my_cam);

    /**
     * @brief
     * @return get current frame updated by hit box (for debugging purpose only)
     */
    inline Mat & get_cur_frame(void) { return ori_cur_frame; }

    /**
     * get desired gimbal command (when attacking an enemy). Should be good enough to pour directly to COMM
     * @brief
     * @param  my_cam An abstract camera Base object. Can be a video feed or a real camera.
     * @return        gimbal command as per defined by struct.
     */
    aimbot_command_t get_desired_command(CameraBase *my_cam);

    inline vector<armor_t> get_latest_visible_armors(void) { return cur_visible_armors; }

private:

    Mat cur_frame, ori_cur_frame;

    vector<armor_t> cur_visible_armors;

    std::string my_color;

    int my_distillation_threshold;

    /**
     * helper function for estimating armor distance to camera. Uses LIGHT_BAR_HEIGHT_ONE_METER in aimbot_config.h
     * @brief
     * @param  single_armor a struct that represents a single piece of armor. Assumes the armor is good.
     * @return              estimated distance
     */
    float get_armor_distance(armor_t single_armor);

    /**
     * @brief preprocess frame with magical methods
     * @param cur_frame_distilled ref to return value
     * @param cur_frame original image
     */
    void preprocess_frame(Mat & cur_frame_distilled, const Mat & cur_frame, Mat color_kernel, Mat gray_kernel);

    /**
     * @brief naive model of detecting light bars with findcontours
     * @param distilled_color
     * @param gray_bin binarized gray img
     * @return vector of cv::RotatedRect objects
     */
    vector<RotatedRect> detect_lights(Mat & distilled_color);

    /**
     * @brief naive filtering model
     * @param ori_img original image captured by camera
     * @param detect_lights detected light bars pulled from last step
     * @return vector of cv::RotatedRect objects
     */
    vector<RotatedRect> filter_lights(const Mat & ori_img, const vector<RotatedRect> & detected_light);

    /*
     * @brief naive armor detecting model based on armor angle, size, aspect ratio, light bar relative angles
     * @param filtered_light_bars filtered light bars
     * @param ori_img ref to original image captured by camera
     * @return vector of armor_loc objects
     */
    vector<armor_t> detect_armor(vector<RotatedRect> & filtered_light_bars, const Mat & ori_img);

    /*
     * @brief armor filtering model. Not implemented for now
     * @param armor_obtained armor_loc objects from last step
     * @return filtered armor_loc objects
     */
    vector<armor_t> filter_armor(const vector<armor_t> & armor_obtained);
};

#endif
