#include "flychams_common/utils/stream_utils.hpp"

using namespace flychams::common;

std::mutex StreamUtils::gstreamer_open_mutex_;

// ════════════════════════════════════════════════════════════════════════════
// PIPELINE BUILDING
// ════════════════════════════════════════════════════════════════════════════

std::string StreamUtils::buildPipeline(const std::string& rtsp_url, const std::string& hw_vendor, DecoderMode mode)
{
    const std::string source =
        "rtspsrc location=" + rtsp_url + " latency=0 protocols=tcp timeout=15000000 "
        "! rtph265depay ! h265parse ";

    const bool use_hw = (mode == DecoderMode::Hardware);

    if (use_hw && hw_vendor == "nvidia")
    {
        return source +
            "! nvh265dec ! videoconvert ! video/x-raw,format=BGR "
            "! appsink drop=true max-buffers=1 sync=false";
    }
    if (use_hw && hw_vendor == "amd")
    {
        return source +
            "! vah265dec ! vapostproc ! videoconvert ! video/x-raw,format=BGR "
            "! appsink drop=true max-buffers=1 sync=false";
    }

    return source +
        "! avdec_h265 ! videoconvert ! video/x-raw,format=BGR "
        "! appsink drop=true max-buffers=1 sync=false";
}

// ════════════════════════════════════════════════════════════════════════════
// CAPTURE OPENING
// ════════════════════════════════════════════════════════════════════════════

bool StreamUtils::tryOpenPipeline(cv::VideoCapture& capture, const std::string& pipeline)
{
    capture.release();
    {
        const std::lock_guard<std::mutex> lock(gstreamer_open_mutex_);
        capture.open(pipeline, cv::CAP_GSTREAMER);
    }
    if (!capture.isOpened())
    {
        capture.release();
        return false;
    }

    return true;
}

bool StreamUtils::openCapture(cv::VideoCapture& capture, const std::string& rtsp_url, const std::string& hw_vendor, std::string& decoder_used)
{
    const bool use_hw = (hw_vendor == "nvidia" || hw_vendor == "amd");
    const DecoderMode mode = use_hw ? DecoderMode::Hardware : DecoderMode::Software;
    const std::string pipeline = buildPipeline(rtsp_url, hw_vendor, mode);

    if (!tryOpenPipeline(capture, pipeline))
    {
        capture.release();
        decoder_used.clear();
        return false;
    }

    decoder_used = use_hw ? hw_vendor : "software";
    return true;
}