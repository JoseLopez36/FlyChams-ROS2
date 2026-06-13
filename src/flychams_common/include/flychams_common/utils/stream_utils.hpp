#pragma once

// Standard includes
#include <chrono>
#include <mutex>
#include <thread>

// OpenCV includes
#include <opencv2/opencv.hpp>

namespace flychams::common
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief RTSP → OpenCV GStreamer pipeline helpers
     *
     * @details
     * Builds H.265 decode pipelines and opens captures in a
     * thread-safe way. Hardware decode for nvidia/amd; software
     * decode only when hw_vendor is none.
     * ════════════════════════════════════════════════════════════════
     * @author Jose Francisco Lopez Ruiz
     * @date 2026-06-13
     * ════════════════════════════════════════════════════════════════
     */
    class StreamUtils
    {
    public:
        enum class DecoderMode
        {
            Hardware,
            Software
        };

        /**
         * @brief Build an RTSP source + H.265 decode pipeline string
         */
        static std::string buildPipeline(const std::string& rtsp_url, const std::string& hw_vendor, DecoderMode mode);

        /**
         * @brief Open an RTSP capture using the decoder for hw_vendor
         */
        static bool openCapture(cv::VideoCapture& capture, const std::string& rtsp_url, const std::string& hw_vendor, std::string& decoder_used);

    private:
        static bool tryOpenPipeline(cv::VideoCapture& capture, const std::string& pipeline);

        static std::mutex gstreamer_open_mutex_;
    };

}  // namespace flychams::common
