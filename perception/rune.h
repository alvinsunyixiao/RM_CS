#ifndef _DETECTION_
#define _DETECTION_

#include "cv_config.h"
#include <opencv2/opencv.hpp>
#include <stdlib.h>
#include <vector>
#include "camera.h"

using namespace cv;
using namespace std;

#define W_CTR_LOW       64
#define W_CTR_HIGH      192
#define H_CTR_LOW       32
#define H_CTR_HIGH      96
#define HW_MIN_RATIO    0.3
#define HW_MAX_RATIO    0.75

class Rune {
public:
    Rune();
    ~Rune();

    /**
     * this function should be called before calling any other processing function.
     * function call updates the current frame to the latest image captured from
     * the camera.
     * @param CameraBase type pointer that has been properly instantiated
     */
    void update(CameraBase *cam);

    /**
     * calculates the white digit sequence according to the current frame
     * sequence satisfies the following spatial relationship
     *      
     *      1   2   3
     *
     *      4   5   6
     *
     *      7   8   9
     *
     * @param an array of integer to be filled with the sequence
     * @return flag indicating whether successfully identified exact 9 digits or not
     */
    bool get_white_seq(vector<int> &seq);
    
    /**
     * calculates the red digit sequence according to the current frame
     * sequence satisfies the following spatial relationship
     *
     *      1   2   3   4   5
     *
     * @param an array of integer to be filled with the sequence
     * @return flag indicating whether successfully indentified exact 5 digits or not
     */
    bool get_red_seq(vector<int> &seq);

private:
    Mat white_bin;
    Mat red_bin;
    Mat raw_img;
    Mat debug_img;
    vector<Mat>             w_digits;   // white digits in 28x28 gray scale
    vector<Mat>             r_digits;   // red digits in 28x28 gray scale
    vector<vector<Point> >  w_contours; // white contours
    vector<vector<Point> >  r_contours; // red contours
    
    void white_binarize();
    void red_binarize();
    void contour_detect();
    void batch_generat();
    void digit_recog();
};

#endif