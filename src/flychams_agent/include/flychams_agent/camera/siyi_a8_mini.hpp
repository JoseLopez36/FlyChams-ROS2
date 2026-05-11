#pragma once

// C++ includes
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>
#include <netinet/in.h>

// Core includes
#include "flychams_common/types/core_types.hpp"

namespace flychams::agent
{
    /**
     * ════════════════════════════════════════════════════════════════
     * @brief Driver for SIYI A8 Mini Gimbal Camera
     * ════════════════════════════════════════════════════════════════
     * @details
     * This class provides utilities for managing the communication
     * with the SIYI A8 Mini Gimbal Camera.
     * 
     * Based on https://github.com/thiagolages/A8mini-gimbal-camera-control
     * 
	 * @author Jose Francisco Lopez Ruiz
     * @date 2026-02-02
     * ════════════════════════════════════════════════════════════════
     */
    class SiyiA8Mini
    {
    public:
        // Constructor & Destructor
        SiyiA8Mini();
        ~SiyiA8Mini();

        // Connection methods
        bool connect(const std::string& ip, int port);
        void disconnect();
        bool isConnected() const;

        // Status methods
        bool getAttitude(float& yaw, float& pitch, float& roll);

        // Control methods
        void setAngles(float yaw_deg, float pitch_deg); // Absolute angles (Command 0x0E)
        void zoom(int step); // 1 (in) or -1 (out)

    private:
        // Internal helpers
        uint16_t calculateCRC16(const uint8_t* ptr, uint32_t len, uint16_t crc_init) const;
        void sendCommand(uint8_t cmd_id, const std::vector<uint8_t>& data);
        bool receivePacket(uint8_t& cmd_id, std::vector<uint8_t>& data);
        
        // Socket details
        int sockfd_;
        struct sockaddr_in server_addr_;
        bool connected_;
        std::string ip_;
        int port_;
        uint16_t seq_; // Sequence number
        
        // Mutex for thread safety
        std::mutex mutex_;
        
        // Constants
        static const uint16_t CRC16_TAB[256];
        static const uint8_t CMD_ID_ACQUIRE_ATTITUDE = 0x0D;
        static const uint8_t CMD_ID_SET_ANGLES = 0x0E;
        static const uint8_t CMD_ID_ZOOM = 0x05;
        
        // Limits
        static constexpr float YAW_MIN = -135.0f;
        static constexpr float YAW_MAX = 135.0f;
        static constexpr float PITCH_MIN = -90.0f;
        static constexpr float PITCH_MAX = 45.0f;
    };

} // namespace flychams::agent
