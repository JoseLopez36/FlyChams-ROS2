#include "flychams_operator/metrics/mission_metrics.hpp"

using namespace flychams::common;

using namespace flychams::operator_pkg;

// ════════════════════════════════════════════════════════════════════════════
// INIT / SHUTDOWN
// ════════════════════════════════════════════════════════════════════════════

void MissionMetrics::onModuleInit()
{
    // Get parameters
    update_rate_ = node_->getParameterOr<float>("update_rate", 1.0f);
    enable_performance_metrics_ = node_->getParameterOr<bool>("enable_performance_metrics", true);

    // Get hardware vendor from environment variable
    gpu_vendor_ = std::getenv("GPU_VENDOR") ? std::getenv("GPU_VENDOR") : "none";

    // Initialize data
    total_agents_ = 0;
    total_targets_ = 0;
    total_clusters_ = 0;
    has_mission_started_ = false;
    resetHwSamples();

    // Publishers
    metrics_pub_ = node_->createMissionMetricsPublisher();

    // Update timer
    update_timer_ = node_->createTimer(update_rate_, std::bind(&MissionMetrics::update, this));
}

void MissionMetrics::onModuleShutdown()
{
    metrics_pub_.reset();
    update_timer_.reset();
}

// ════════════════════════════════════════════════════════════════════════════
// ELEMENT MANAGEMENT
// ════════════════════════════════════════════════════════════════════════════

void MissionMetrics::addAgent()
{
    total_agents_++;
}

void MissionMetrics::removeAgent()
{
    total_agents_--;
}

void MissionMetrics::addTarget()
{
    total_targets_++;
}

void MissionMetrics::removeTarget()
{
    total_targets_--;
}

void MissionMetrics::addCluster()
{
    total_clusters_++;
}

void MissionMetrics::removeCluster()
{
    total_clusters_--;
}

// ════════════════════════════════════════════════════════════════════════════
// UPDATE
// ════════════════════════════════════════════════════════════════════════════

void MissionMetrics::update()
{
    // Skip update if status is not valid
    if (!checkStatus())
    {
        return;
    }

    // Compute elapsed time
    float time_elapsed = 0.0f;
    if (has_mission_started_)
    {
        time_elapsed = static_cast<float>((node_->now() - mission_start_time_).seconds());
    }
    else if (node_->isMissionActive())
    {
        // Mission just started
        mission_start_time_ = node_->now();
        has_mission_started_ = true;
        resetHwSamples();
    }

    // Sample hardware metrics
    if (enable_performance_metrics_)
    {
        sampleHardware();
    }

    // Build and publish message
    MissionMetricsMsg msg;
    msg.header = node_->createHeader(node_->getGlobalFrame());
    msg.total_agents = total_agents_;
    msg.total_targets = total_targets_;
    msg.total_clusters = total_clusters_;
    msg.time = time_elapsed;

    // CPU
    msg.cpu_instant = cpu_.instant;
    msg.cpu_mean    = (cpu_.count > 0) ? (cpu_.sum / static_cast<float>(cpu_.count)) : 0.0f;
    msg.cpu_max     = cpu_.max;

    // GPU
    msg.gpu_instant = gpu_.instant;
    msg.gpu_mean    = (gpu_.count > 0) ? (gpu_.sum / static_cast<float>(gpu_.count)) : 0.0f;
    msg.gpu_max     = gpu_.max;

    // RAM
    msg.ram_instant = ram_.instant;
    msg.ram_mean    = (ram_.count > 0) ? (ram_.sum / static_cast<float>(ram_.count)) : 0.0f;
    msg.ram_max     = ram_.max;

    // VRAM
    msg.vram_instant = vram_.instant;
    msg.vram_mean    = (vram_.count > 0) ? (vram_.sum / static_cast<float>(vram_.count)) : 0.0f;
    msg.vram_max     = vram_.max;

    metrics_pub_->publish(msg);
}

// ════════════════════════════════════════════════════════════════════════════
// HARDWARE SAMPLING
// ════════════════════════════════════════════════════════════════════════════

void MissionMetrics::resetHwSamples()
{
    cpu_  = {0.0f, 0.0f, 0.0f, 0};
    gpu_  = {0.0f, 0.0f, 0.0f, 0};
    ram_  = {0.0f, 0.0f, 0.0f, 0};
    vram_ = {0.0f, 0.0f, 0.0f, 0};
}

void MissionMetrics::updateSample(HwSample& s, float value)
{
    s.instant = value;
    s.sum    += value;
    s.count  += 1;
    if (value > s.max)
    {
        s.max = value;
    }
}

void MissionMetrics::sampleHardware()
{
    updateSample(cpu_,  readCpuUsage());
    updateSample(gpu_,  readGpuUsage());
    updateSample(ram_,  readRamUsageGb());
    updateSample(vram_, readVramUsageGb());

    RCLCPP_DEBUG(node_->get_logger(),
        "HW sample [%s] — CPU: %.1f%% | GPU: %.1f%% | RAM: %.2f GB | VRAM: %.2f GB",
        gpu_vendor_.c_str(), cpu_.instant, gpu_.instant, ram_.instant, vram_.instant);
}

float MissionMetrics::readCpuUsage() const
{
    // Read two consecutive snapshots of /proc/stat to compute CPU utilisation
    struct CpuTick
    {
        long long user, nice, system, idle, iowait, irq, softirq, steal;
        long long busy() const { return user + nice + system + irq + softirq + steal; }
        long long total() const { return busy() + idle + iowait; }
    };

    static CpuTick prev = {};
    static bool initialised = false;

    CpuTick cur = {};
    std::ifstream fs("/proc/stat");
    if (!fs.is_open())
    {
        return 0.0f;
    }

    std::string label;
    fs >> label >> cur.user >> cur.nice >> cur.system >> cur.idle
       >> cur.iowait >> cur.irq >> cur.softirq >> cur.steal;
    fs.close();

    float usage = 0.0f;
    if (initialised)
    {
        const long long delta_total = cur.total() - prev.total();
        const long long delta_busy  = cur.busy()  - prev.busy();
        if (delta_total > 0)
        {
            usage = 100.0f * static_cast<float>(delta_busy) / static_cast<float>(delta_total);
        }
    }

    prev = cur;
    initialised = true;
    return usage;
}

float MissionMetrics::readGpuUsage() const
{
    if (gpu_vendor_ == "nvidia")
    {
        // nvidia-smi returns GPU utilisation in percent
        FILE* pipe = popen("nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits 2>/dev/null", "r");
        if (!pipe)
        {
            return 0.0f;
        }
        float value = 0.0f;
        std::array<char, 64> buf{};
        if (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        {
            value = std::stof(buf.data());
        }
        pclose(pipe);
        return value;
    }
    else if (gpu_vendor_ == "amd")
    {
        // Read GPU busy percent directly from AMDGPU sysfs
        for (int card = 0; card < 8; ++card)
        {
            const std::string path = "/sys/class/drm/card" + std::to_string(card) +
                                     "/device/gpu_busy_percent";
            std::ifstream fs(path);
            if (!fs.is_open())
            {
                continue;
            }
            int value = 0;
            fs >> value;
            return static_cast<float>(value);
        }
    }
    return 0.0f;
}

float MissionMetrics::readRamUsageGb() const
{
    // Parse /proc/meminfo for MemTotal and MemAvailable
    std::ifstream fs("/proc/meminfo");
    if (!fs.is_open())
    {
        return 0.0f;
    }

    long long mem_total_kb = 0;
    long long mem_available_kb = 0;
    std::string line;
    while (std::getline(fs, line))
    {
        std::istringstream ss(line);
        std::string key;
        long long value;
        ss >> key >> value;
        if (key == "MemTotal:")
        {
            mem_total_kb = value;
        }
        else if (key == "MemAvailable:")
        {
            mem_available_kb = value;
        }
        if (mem_total_kb > 0 && mem_available_kb > 0)
        {
            break;
        }
    }
    fs.close();

    const long long used_kb = mem_total_kb - mem_available_kb;
    return static_cast<float>(used_kb) / (1024.0f * 1024.0f);
}

float MissionMetrics::readVramUsageGb() const
{
    if (gpu_vendor_ == "nvidia")
    {
        // nvidia-smi reports used VRAM in MiB; convert to GB
        FILE* pipe = popen("nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits 2>/dev/null", "r");
        if (!pipe)
        {
            return 0.0f;
        }
        float used_mib = 0.0f;
        std::array<char, 64> buf{};
        if (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        {
            used_mib = std::stof(buf.data());
        }
        pclose(pipe);
        return used_mib / 1024.0f;
    }
    else if (gpu_vendor_ == "amd")
    {
        // AMDGPU sysfs reports used VRAM in bytes; convert to GB
        for (int card = 0; card < 8; ++card)
        {
            const std::string path = "/sys/class/drm/card" + std::to_string(card) +
                                     "/device/mem_info_vram_used";
            std::ifstream fu(path);
            if (!fu.is_open())
            {
                continue;
            }
            long long used = 0;
            fu >> used;
            return static_cast<float>(used) / (1024.0f * 1024.0f * 1024.0f);
        }
    }
    return 0.0f;
}

// ════════════════════════════════════════════════════════════════════════════
// STATUS: Status check
// ════════════════════════════════════════════════════════════════════════════

bool MissionMetrics::checkStatus()
{
    // Only publish when mission is active
    if (!node_->isMissionActive())
    {
        // Reset mission started flag when mission ends
        has_mission_started_ = false;
        return false;
    }

    // All checks passed
    return true;
}