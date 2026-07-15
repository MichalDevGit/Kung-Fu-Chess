#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <filesystem>


class Img {
public:
    Img();

    /**
     * Load image from path during construction.
     *
     * @param path Image file to load
     */
    explicit Img(const std::string& path);

    /**
     * Copy constructor.
     *
     * Creates a deep copy of the image data.
     *
     * @param other Image to copy
     */
    Img(const Img& other);

    /**
     * Assignment operator.
     *
     * Creates a deep copy of the image data.
     *
     * @param other Image to copy
     * @return Reference to this object
     */
    Img& operator=(const Img& other);

    /**
     * Load image from path and optionally resize.
     * 
     * @param path Image file to load
     * @param size Target size in pixels (width, height). If empty, keep original
     * @param keep_aspect If true, shrink so the longer side fits size while preserving aspect ratio
     * @param interpolation OpenCV interpolation flag (e.g., cv::INTER_AREA for shrink, cv::INTER_LINEAR for enlarge)
     * @return Reference to this object for method chaining
     */
    Img& read(const std::string& path,
              const std::pair<int, int>& size = {},
              bool keep_aspect = false,
              int interpolation = cv::INTER_AREA);


    /**
     * Create a deep copy of this image.
     *
     * @return New Img object containing copied image data
     */
    Img clone() const;


    /**
     * Draw this image onto another image at position (x, y)
     * 
     * @param other_img The target image to draw on
     * @param x X coordinate for top-left corner
     * @param y Y coordinate for top-left corner
     */
    void draw_on(Img& other_img, int x, int y) const;


    /**
     * Put text on the image
     * 
     * @param txt Text to draw
     * @param x X coordinate for text position
     * @param y Y coordinate for text position (baseline)
     * @param font_size Font scale factor
     * @param color Text color (BGR or BGRA)
     * @param thickness Text thickness
     */
    void put_text(const std::string& txt, int x, int y, double font_size,
                  const cv::Scalar& color = cv::Scalar(255, 255, 255, 255),
                  int thickness = 1);


    /**
     * Display the image in a window
     */
    void show();


    /**
     * Get the underlying OpenCV Mat
     */
    const cv::Mat& get_mat() const { return img; }


    /**
     * Check if image is loaded
     */
    bool is_loaded() const { return !img.empty(); }


private:
    cv::Mat img;
};