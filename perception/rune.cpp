#include <caffe/caffe.hpp>
#include <caffe/blob.hpp>
#include <caffe/util/io.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cstring>
#include <algorithm>
#include "cv_config.h"
#include "camera.h"
#include "rune.h"

using namespace caffe;
using namespace std;
using namespace cv;

bool cmp_x(Point &i, Point &j) { return i.x < j.x; }

bool cmp_y(Point &i, Point &j) { return i.y < j.y; }

bool cmp_px(pair<Point, int> &i, pair<Point, int> &j) { return i.first.x < j.first.x; }

bool cmp_py(pair<Point, int> &i, pair<Point, int> &j) { return i.first.y < j.first.y; }

Rune::Rune(string net_file, string param_file) {
#ifdef CPU_ONLY
    Caffe::set_mode(Caffe::CPU);
#else
    Caffe::set_mode(Caffe::GPU);
    Caffe::SetDevice(0);
#endif
    net = new Net<float>(net_file.c_str(), caffe::TEST);
    net->CopyTrainedLayersFrom(param_file.c_str());
    input_layer = net->input_blobs()[0];
    input_layer->Reshape(BATCH_SIZE, 1, DIGIT_SIZE, DIGIT_SIZE);
    net->Reshape();
    input_layer = net->input_blobs()[0];
    output_layer = net->output_blobs()[0];
}

Rune::~Rune() {}

void Rune::update(CameraBase *cam) {
    cam->get_img(raw_img);
    if (raw_img.size() != Size(IMAGE_WIDTH, IMAGE_HEIGHT))
        cv::resize(raw_img, raw_img, Size(IMAGE_WIDTH, IMAGE_HEIGHT));
#ifdef DEBUG
    raw_img.copyTo(debug_img);
    imshow("raw image", raw_img);
    waitKey(1);
#endif
}

void Rune::white_binarize() {
    cv::cvtColor(raw_img, gray_img, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray_img, white_bin, Size(3,3), 0);
    cv::threshold(white_bin, white_bin, 0, 255, cv::THRESH_BINARY+cv::THRESH_OTSU);
    cv::morphologyEx(white_bin, white_bin, cv::MORPH_CLOSE, Mat::ones(4, 8, CV_8UC1));
#ifdef DEBUG
    imshow("white digit binarized", white_bin);
    waitKey(1);
#endif
}

void Rune::contour_detect() {
    vector<Vec4i>           hierarchy;
    vector<vector<Point> >  contours;

    w_contours.clear();

    cv::findContours(white_bin, contours, hierarchy,
            cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    for (auto cnt: contours) {
        float width = (*max_element(cnt.begin(), cnt.end(), cmp_x)).x -
            (*min_element(cnt.begin(), cnt.end(), cmp_x)).x;
        float height = (*max_element(cnt.begin(), cnt.end(), cmp_y)).y -
            (*min_element(cnt.begin(), cnt.end(), cmp_y)).y;
        if (width >= W_CTR_LOW && width <= W_CTR_HIGH && 
                height >= H_CTR_LOW && height <= H_CTR_HIGH &&
                height / width >= HW_MIN_RATIO && height / width <= HW_MAX_RATIO) {
            vector<Point> approx;
            cv::approxPolyDP(cnt, approx, 0.05 * cv::arcLength(cnt, true), true);
            if (approx.size() == 4 && cv::isContourConvex(approx))
                w_contours.push_back(approx);
        }
    }
#ifdef DEBUG
    cv::drawContours(debug_img, w_contours, -1, Scalar(0, 255, 0), 3);
    imshow("contour detection", debug_img);
    waitKey(1);
#endif
}

void Rune::batch_generate() {
    int offset = (CROP_SIZE - DIGIT_SIZE) / 2;
    Mat M;
    Point2f cnt_points[4];
    w_digits.clear();
    int i = 0;

    for (vector<Point> &cnt: w_contours) {
        sort(cnt.begin(), cnt.end(), cmp_y);
        if (cnt[0].x > cnt[1].x)
            swap(cnt[0], cnt[1]);
        if (cnt[2].x > cnt[3].x)
            swap(cnt[2], cnt[3]);
        for (size_t i = 0; i < 4; i++)
            cnt_points[i] = Point2f(cnt[i]);
        Mat digit_img;
        M = cv::getPerspectiveTransform(cnt_points, dst_points);
        cv::warpPerspective(gray_img, digit_img, M, Size(CROP_SIZE, CROP_SIZE));
        cv::bitwise_not(digit_img(cv::Rect(offset, offset, DIGIT_SIZE, DIGIT_SIZE)), digit_img);
        w_digits.push_back(digit_img);
    }
}

int argmax(float *data, size_t length) {
    int max_idx = 0;
    for (size_t i = 1; i < length; i++)
        if (data[i] > data[max_idx])
            max_idx = i;

    return max_idx;
}

void Rune::network_inference(vector<pair<int, int> > &predictions) {
#ifdef CPU_ONLY
    float *input_data = input_layer->mutable_cpu_data();
    float *output_data = output_layer->mutable_cpu_data();
#else
    float *input_data = input_layer->mutable_gpu_data();
    float *output_data = output_layer->mutable_gpu_data();
#endif
    for (auto dig: w_digits) {
        Mat channel(DIGIT_SIZE, DIGIT_SIZE, CV_32FC1, input_data);
        dig.convertTo(channel, CV_32FC1);
        channel /= 255;
        input_data += DIGIT_SIZE * DIGIT_SIZE;
    }

    net->Forward();

    for (size_t i = 0; i < w_digits.size(); i++) {
        int dig_id = argmax(output_data, output_layer->channels());
        output_data += output_layer->channels();
        if (dig_id != 10)    // irrelevent class number
            predictions.push_back(pair<int, int>(i, dig_id));
    }
#ifdef DEBUG
    for(auto p: predictions) {
        int dig_id = p.second;
        Point loc = w_contours[p.first][0];
        loc.y -= 20;
        putText(debug_img, to_string(dig_id), loc, FONT_HERSHEY_SIMPLEX, 0.9,
                Scalar(0, 150, 100), 2, LINE_AA);
    }
    imshow("digit recognition", debug_img);
    waitKey(1);
#endif
}

bool Rune::get_white_seq(vector<int> &seq) {
    white_binarize();
    contour_detect();
    if (w_contours.size() > BATCH_SIZE || w_contours.size() < 9)
        return false;
    batch_generate();

    vector<pair<int, int> > predictions; 
    network_inference(predictions);
    if (predictions.size() != 9)
        return false;
    
    vector<pair<Point, int> > loc_idx;
    for (size_t i = 0; i < predictions.size(); i++)
        loc_idx.push_back(pair<Point, int>(w_contours[predictions[i].first][0], 
                    predictions[i].second));
    sort(loc_idx.begin(), loc_idx.end(), cmp_py);
    sort(loc_idx.begin(), loc_idx.begin()+3, cmp_px);
    sort(loc_idx.begin()+3, loc_idx.begin()+6, cmp_px);
    sort(loc_idx.begin()+6, loc_idx.end(), cmp_px);

    for (auto li: loc_idx)
        seq.push_back(li.second);
    return true;
}

bool Rune::get_red_seq(vector<int> &seq) {
    return true;
}
