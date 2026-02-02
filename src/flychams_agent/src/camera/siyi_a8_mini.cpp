#include "flychams_agent/camera/siyi_a8_mini.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cmath>
#include <algorithm>

namespace flychams::agent
{

    // ════════════════════════════════════════════════════════════════════════════
    // CONSTANTS
    // ════════════════════════════════════════════════════════════════════════════

    const uint16_t SiyiA8Mini::CRC16_TAB[256] = {
        0x0,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
        0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
        0x1231,0x210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
        0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
        0x2462,0x3443,0x420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
        0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
        0x3653,0x2672,0x1611,0x630,0x76d7,0x66f6,0x5695,0x46b4,
        0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
        0x48c4,0x58e5,0x6886,0x78a7,0x840,0x1861,0x2802,0x3823,
        0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
        0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0xa50,0x3a33,0x2a12,
        0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
        0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0xc60,0x1c41,
        0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
        0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0xe70,
        0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
        0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
        0x1080,0xa1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
        0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
        0x2b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
        0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
        0x34e2,0x24c3,0x14a0,0x481,0x7466,0x6447,0x5424,0x4405,
        0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
        0x26d3,0x36f2,0x691,0x16b0,0x6657,0x7676,0x4615,0x5634,
        0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
        0x5844,0x4865,0x7806,0x6827,0x18c0,0x8e1,0x3882,0x28a3,
        0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
        0x4a75,0x5a54,0x6a37,0x7a16,0xaf1,0x1ad0,0x2ab3,0x3a92,
        0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
        0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0xcc1,
        0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
        0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0xed1,0x1ef0
    };

    // ════════════════════════════════════════════════════════════════════════════
    // CONSTRUCTOR & DESTRUCTOR
    // ════════════════════════════════════════════════════════════════════════════

    SiyiA8Mini::SiyiA8Mini()
        : sockfd_(-1), connected_(false), port_(0), seq_(0)
    {
    }

    SiyiA8Mini::~SiyiA8Mini()
    {
        disconnect();
    }

    // ════════════════════════════════════════════════════════════════════════════
    // CONNECTION
    // ════════════════════════════════════════════════════════════════════════════

    bool SiyiA8Mini::connect(const std::string& ip, int port)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connected_) {
            return true;
        }

        ip_ = ip;
        port_ = port;

        // Create socket
        if ((sockfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            std::cerr << "SiyiA8Mini: Failed to create socket" << std::endl;
            return false;
        }

        // Set timeout for receiving
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 350000;
        if (setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            std::cerr << "SiyiA8Mini: Failed to set socket timeout" << std::endl;
        }

        // Configure server address
        memset(&server_addr_, 0, sizeof(server_addr_));
        server_addr_.sin_family = AF_INET;
        server_addr_.sin_addr.s_addr = inet_addr(ip_.c_str());
        server_addr_.sin_port = htons(port_);

        connected_ = true;
        return true;
    }

    void SiyiA8Mini::disconnect()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (sockfd_ >= 0) {
            close(sockfd_);
            sockfd_ = -1;
        }
        connected_ = false;
    }

    bool SiyiA8Mini::isConnected() const
    {
        return connected_;
    }

    // ════════════════════════════════════════════════════════════════════════════
    // STATUS METHODS
    // ════════════════════════════════════════════════════════════════════════════

    bool SiyiA8Mini::getAttitude(float& yaw, float& pitch, float& roll)
    {
        // Command 0x0D
        // Data: empty
        sendCommand(CMD_ID_ACQUIRE_ATTITUDE, {});
        
        // Try to receive response
        uint8_t cmd_id;
        std::vector<uint8_t> data;
        
        // Try reading a few times to find the correct packet
        int attempts = 3;
        while (attempts--) {
            if (receivePacket(cmd_id, data)) {
                if (cmd_id == CMD_ID_ACQUIRE_ATTITUDE && data.size() >= 12) {
                    // Parse data
                    // Format: 6 int16s (yaw, pitch, roll, yaw_vel, pitch_vel, roll_vel) * 10
                    int16_t yaw_i, pitch_i, roll_i;
                    
                    std::memcpy(&yaw_i, &data[0], sizeof(int16_t));
                    std::memcpy(&pitch_i, &data[2], sizeof(int16_t));
                    std::memcpy(&roll_i, &data[4], sizeof(int16_t));
                    
                    yaw = yaw_i / 10.0f;
                    pitch = pitch_i / 10.0f;
                    roll = roll_i / 10.0f;
                    return true;
                }
            }
        }
        
        return false;
    }


    // ════════════════════════════════════════════════════════════════════════════
    // CONTROL METHODS
    // ════════════════════════════════════════════════════════════════════════════

    void SiyiA8Mini::setAngles(float yaw_deg, float pitch_deg)
    {
        // Clamp values
        yaw_deg = std::max(YAW_MIN, std::min(YAW_MAX, yaw_deg));
        pitch_deg = std::max(PITCH_MIN, std::min(PITCH_MAX, pitch_deg));

        // Command 0x0E
        // Data: Yaw (int16, deg*10), Pitch (int16, deg*10) - Little Endian
        int16_t yaw_val = static_cast<int16_t>(std::round(yaw_deg * 10.0f));
        int16_t pitch_val = static_cast<int16_t>(std::round(pitch_deg * 10.0f));

        std::vector<uint8_t> data;
        data.push_back(static_cast<uint8_t>(yaw_val & 0xFF));
        data.push_back(static_cast<uint8_t>((yaw_val >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(pitch_val & 0xFF));
        data.push_back(static_cast<uint8_t>((pitch_val >> 8) & 0xFF));

        sendCommand(CMD_ID_SET_ANGLES, data);
    }

    void SiyiA8Mini::zoom(int step)
    {
        // Command 0x05
        // Zoom +1: Data 0x01
        // Zoom -1: Data 0xFF (255)
        
        uint8_t val = (step > 0) ? 0x01 : 0xFF;
        if (step == 0) return; 

        std::vector<uint8_t> data = {val};
        sendCommand(CMD_ID_ZOOM, data);
    }
    
    // ════════════════════════════════════════════════════════════════════════════
    // INTERNAL HELPERS
    // ════════════════════════════════════════════════════════════════════════════

    void SiyiA8Mini::sendCommand(uint8_t cmd_id, const std::vector<uint8_t>& data)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!connected_) return;

        // Protocol Structure:
        // STX (2) | CTRL (1) | Data_len (2) | SEQ (2) | CMD_ID (1) | DATA (N) | CRC16 (2)
        
        std::vector<uint8_t> buffer;
        buffer.reserve(10 + data.size());

        // STX
        buffer.push_back(0x55);
        buffer.push_back(0x66);
        
        // CTRL (1 = Need ACK)
        buffer.push_back(0x01);

        // Data_len (Low byte first)
        uint16_t len = static_cast<uint16_t>(data.size());
        buffer.push_back(static_cast<uint8_t>(len & 0xFF));
        buffer.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));

        // SEQ
        buffer.push_back(static_cast<uint8_t>(seq_ & 0xFF));
        buffer.push_back(static_cast<uint8_t>((seq_ >> 8) & 0xFF));
        seq_++; // Increment sequence

        // CMD_ID
        buffer.push_back(cmd_id);

        // DATA
        buffer.insert(buffer.end(), data.begin(), data.end());

        // CRC16
        uint16_t crc = calculateCRC16(buffer.data(), buffer.size(), 0);
        buffer.push_back(static_cast<uint8_t>(crc & 0xFF));
        buffer.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));

        // Send
        sendto(sockfd_, buffer.data(), buffer.size(), 0, (struct sockaddr*)&server_addr_, sizeof(server_addr_));
    }

    bool SiyiA8Mini::receivePacket(uint8_t& cmd_id, std::vector<uint8_t>& data)
    {
        uint8_t buf[2048];
        socklen_t addr_len = sizeof(server_addr_);
        
        ssize_t recv_len = recvfrom(sockfd_, buf, sizeof(buf), 0, (struct sockaddr*)&server_addr_, &addr_len);
        
        if (recv_len < 10) return false; // Minimum size
        
        // Check STX
        if (buf[0] != 0x55 || buf[1] != 0x66) return false;
        
        // Check CRC
        // Received CRC is last 2 bytes
        uint16_t recv_crc = buf[recv_len-2] | (buf[recv_len-1] << 8);
        uint16_t calc_crc = calculateCRC16(buf, recv_len-2, 0);
        
        if (recv_crc != calc_crc) return false;
        
        // Parse
        // CTRL (2), LEN (3-4), SEQ (5-6), CMD_ID (7), DATA (8...)
        uint16_t data_len = buf[3] | (buf[4] << 8);
        
        if (recv_len != 10 + data_len) return false; // Size mismatch
        
        cmd_id = buf[7];
        data.clear();
        data.insert(data.end(), buf + 8, buf + 8 + data_len);
        
        return true;
    }

    uint16_t SiyiA8Mini::calculateCRC16(const uint8_t* ptr, uint32_t len, uint16_t crc_init) const
    {
        uint16_t crc, oldcrc16;
        uint8_t temp;
        crc = crc_init;
        while (len-- != 0) {
            temp = (crc >> 8) & 0xff;
            oldcrc16 = CRC16_TAB[*ptr ^ temp];
            crc = (crc << 8) ^ oldcrc16;
            ptr++;
        }
        return crc;
    }

} // namespace flychams::agent