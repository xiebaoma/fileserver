/**
 * @brief File Transfer Protocol Type Definitions
 * @file FileMsg.h
 * @author xiebaoma
 * @date: 2025-05-25
 **/
#pragma once

#include <stdint.h>

/**
 * Enumeration of file message types used in the protocol
 */
enum file_msg_type
{
    file_msg_type_unknown,  // Unknown message type
    msg_type_upload_req,    // Upload request message
    msg_type_upload_resp,   // Upload response message
    msg_type_download_req,  // Download request message
    msg_type_download_resp, // Download response message
};

/**
 * Error codes for file transfer operations
 */
enum file_msg_error_code
{
    file_msg_error_unknown,  // Unknown error
    file_msg_error_progress, // File upload or download in progress
    file_msg_error_complete, // File upload or download completed
    file_msg_error_not_exist // File does not exist
};

/**
 * Client network type classification
 */
enum client_net_type
{
    client_net_type_broadband, // Broadband connection
    client_net_type_cellular   // Mobile/cellular network
};

#pragma pack(push, 1)
/**
 * Protocol header structure
 * Uses packed structure to ensure consistent memory layout
 */
struct file_msg_header
{
    int64_t packagesize; // Specifies the size of the message body in bytes
};

#pragma pack(pop)
