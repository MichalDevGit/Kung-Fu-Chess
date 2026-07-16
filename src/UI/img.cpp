#include "img.h"
#include <iostream>
#include <stdexcept>


Img::Img()
{
    // Constructor - img is automatically initialized as empty
}


Img::Img(const std::string& path)
{
    read(path);
}


Img::Img(const Img& other)
{
    img = other.img.clone();
}


Img& Img::operator=(const Img& other)
{
    if (this != &other)
    {
        img = other.img.clone();
    }

    return *this;
}


Img& Img::read(const std::string& path,
               const std::pair<int, int>& size,
               bool keep_aspect,
               int interpolation)
{
    img = cv::imread(path, cv::IMREAD_UNCHANGED);

    if (img.empty())
    {
        throw std::runtime_error("Cannot load image: " + path);
    }


    if (size.first != 0 && size.second != 0)
    {
        int target_w = size.first;
        int target_h = size.second;

        int h = img.rows;
        int w = img.cols;


        if (keep_aspect)
        {
            double scale = std::min(static_cast<double>(target_w) / w,
                                    static_cast<double>(target_h) / h);

            int new_w = static_cast<int>(w * scale);
            int new_h = static_cast<int>(h * scale);

            cv::resize(img,
                       img,
                       cv::Size(new_w, new_h),
                       0,
                       0,
                       interpolation);
        }
        else
        {
            cv::resize(img,
                       img,
                       cv::Size(target_w, target_h),
                       0,
                       0,
                       interpolation);
        }
    }

    return *this;
}


Img Img::clone() const
{
    Img result;

    result.img = img.clone();

    return result;
}


// void Img::draw_on(Img& other_img, int x, int y) const
// {
//     if (img.empty() || other_img.img.empty())
//     {
//         throw std::runtime_error(
//             "Both images must be loaded before drawing.");
//     }


//     cv::Mat source_img = img;
//     cv::Mat target_img = other_img.img;


//     if (source_img.channels() != target_img.channels())
//     {
//         if (source_img.channels() == 3 &&
//             target_img.channels() == 4)
//         {
//             cv::cvtColor(source_img,
//                          source_img,
//                          cv::COLOR_BGR2BGRA);
//         }
//         else if (source_img.channels() == 4 &&
//                  target_img.channels() == 3)
//         {
//             cv::cvtColor(source_img,
//                          source_img,
//                          cv::COLOR_BGRA2BGR);
//         }
//     }


//     int h = source_img.rows;
//     int w = source_img.cols;

//     int H = target_img.rows;
//     int W = target_img.cols;


//     if (x < 0 || y < 0 || x + w > W || y + h > H)
//     {
//         throw std::runtime_error(
//             "Image does not fit at the specified position.");
//     }


//     cv::Mat roi = target_img(cv::Rect(x, y, w, h));


//     if (source_img.channels() == 4)
//     {
//         std::vector<cv::Mat> channels;

//         cv::split(source_img, channels);

//         cv::Mat alpha = channels[3];

//         for (int row = 0; row < h; row++)
//         {
//             for (int col = 0; col < w; col++)
//             {
//                 double a = alpha.at<uchar>(row, col) / 255.0;

//                 for (int c = 0; c < 3; c++)
//                 {
//                     roi.at<cv::Vec3b>(row, col)[c] =
//                         static_cast<uchar>(
//                             a * channels[c].at<uchar>(row, col) +
//                             (1 - a) * roi.at<cv::Vec3b>(row, col)[c]);
//                 }
//             }
//         }
//     }
//     else
//     {
//         source_img.copyTo(roi);
//     }
// }

void Img::draw_on(Img& other_img, int x, int y) {
    if (img.empty() || other_img.img.empty()) {
        throw std::runtime_error("Both images must be loaded before drawing.");
    }

    const cv::Mat& source_img = img;
    cv::Mat& target_img = other_img.img;

    int h = source_img.rows, w = source_img.cols;
    int H = target_img.rows, W = target_img.cols;
    if (y + h > H || x + w > W) {
        throw std::runtime_error("Image does not fit at the specified position.");
    }

    cv::Mat roi = target_img(cv::Rect(x, y, w, h));

    if (source_img.channels() == 4) {
        // למקור יש אלפא - תמיד לבצע בלנדינג, בלי קשר למספר הערוצים של היעד
        std::vector<cv::Mat> srcChannels;
        cv::split(source_img, srcChannels);
        cv::Mat alpha;
        srcChannels[3].convertTo(alpha, CV_32FC1, 1.0 / 255.0);

        std::vector<cv::Mat> roiChannels;
        cv::split(roi, roiChannels);

        for (int c = 0; c < 3; ++c) {
            cv::Mat srcF, dstF, blendedF;
            srcChannels[c].convertTo(srcF, CV_32FC1);
            roiChannels[c].convertTo(dstF, CV_32FC1);
            blendedF = alpha.mul(srcF) + (cv::Scalar(1.0) - alpha).mul(dstF);
            blendedF.convertTo(roiChannels[c], roiChannels[c].type());
        }
        cv::merge(roiChannels, roi);
    }
    else if (source_img.channels() == target_img.channels()) {
        source_img.copyTo(roi);
    }
    else {
        throw std::runtime_error(
            "draw_on: unsupported channel combination (source has " +
            std::to_string(source_img.channels()) + " channels, target has " +
            std::to_string(target_img.channels()) + ").");
    }
}

void Img::put_text(const std::string& txt,
                   int x,
                   int y,
                   double font_size,
                   const cv::Scalar& color,
                   int thickness)
{
    if (img.empty())
    {
        throw std::runtime_error("Image not loaded.");
    }


    cv::putText(img,
                txt,
                cv::Point(x, y),
                cv::FONT_HERSHEY_SIMPLEX,
                font_size,
                color,
                thickness,
                cv::LINE_AA);
}


void Img::show()
{
    if (img.empty())
    {
        throw std::runtime_error("Image not loaded.");
    }


    cv::imshow("Image", img);

    cv::waitKey(0);

    cv::destroyAllWindows();
}