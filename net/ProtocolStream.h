/**
 *  @file Protocolstream.h
 *  @brief Binary protocol serialization and deserialization utilities
 *  @author xiebaoma
 *  @date 2025-05-25
 */

#ifndef __PROTOCOL_STREAM_H__
#define __PROTOCOL_STREAM_H__

#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <sstream>
#include <stdint.h>

// Binary protocol packing and unpacking classes for internal server communication
namespace net
{
    /**
     * @brief Protocol constants for package length and size limits
     */
    enum
    {
        TEXT_PACKLEN_LEN = 4,           // Text package length field size (4 bytes)
        TEXT_PACKAGE_MAXLEN = 0xffff,   // Maximum text package length (65535 bytes)
        BINARY_PACKLEN_LEN = 2,         // Binary package length field size (2 bytes)
        BINARY_PACKAGE_MAXLEN = 0xffff, // Maximum binary package length (65535 bytes)

        TEXT_PACKLEN_LEN_2 = 6,           // Extended text package length field size (6 bytes)
        TEXT_PACKAGE_MAXLEN_2 = 0xffffff, // Extended maximum text package length (16777215 bytes)

        BINARY_PACKLEN_LEN_2 = 4,             // Extended binary package length field size (4 bytes)
        BINARY_PACKAGE_MAXLEN_2 = 0x10000000, // Extended maximum binary package length (256MB)

        CHECKSUM_LEN = 2, // Checksum field length (2 bytes)
    };

    /**
     * @brief Calculate checksum for data verification
     * @param buffer Pointer to the data buffer
     * @param size Size of the buffer in bytes
     * @return Calculated checksum value
     */
    unsigned short checksum(const unsigned short *buffer, int size);

    /**
     * @brief Compress a 32-bit integer into 1-5 bytes using 7-bit encoding
     * @param value The 32-bit value to encode
     * @param buf The output buffer to store the encoded value
     */
    void write7BitEncoded(uint32_t value, std::string &buf);

    /**
     * @brief Compress a 64-bit integer into 1-10 bytes using 7-bit encoding
     * @param value The 64-bit value to encode
     * @param buf The output buffer to store the encoded value
     */
    void write7BitEncoded(uint64_t value, std::string &buf);

    /**
     * @brief Decode a 1-5 byte array back to a 32-bit integer
     * @param buf The input buffer containing the encoded value
     * @param len Length of the input buffer
     * @param value The output decoded 32-bit value
     */
    void read7BitEncoded(const char *buf, uint32_t len, uint32_t &value);

    /**
     * @brief Decode a 1-10 byte array back to a 64-bit integer
     * @param buf The input buffer containing the encoded value
     * @param len Length of the input buffer
     * @param value The output decoded 64-bit value
     */
    void read7BitEncoded(const char *buf, uint32_t len, uint64_t &value);

    /**
     * @class BinaryStreamReader
     * @brief Reads and deserializes data from a binary stream
     */
    class BinaryStreamReader final
    {
    public:
        /**
         * @brief Constructor
         * @param ptr Pointer to the binary data
         * @param len Length of the binary data
         */
        BinaryStreamReader(const char *ptr, size_t len);

        /**
         * @brief Default destructor
         */
        ~BinaryStreamReader() = default;

        /**
         * @brief Get the raw data pointer
         * @return Pointer to the binary data
         */
        virtual const char *GetData() const;

        /**
         * @brief Get the total size of the binary data
         * @return Size of the binary data in bytes
         */
        virtual size_t GetSize() const;

        /**
         * @brief Check if the stream is empty
         * @return True if empty, false otherwise
         */
        bool IsEmpty() const;

        /**
         * @brief Read a string from the stream
         * @param str Output string
         * @param maxlen Maximum length to read
         * @param outlen Actual length read
         * @return True if successful, false otherwise
         */
        bool ReadString(std::string *str, size_t maxlen, size_t &outlen);

        /**
         * @brief Read a C-style string from the stream
         * @param str Output buffer
         * @param strlen Size of the output buffer
         * @param len Actual length read
         * @return True if successful, false otherwise
         */
        bool ReadCString(char *str, size_t strlen, size_t &len);

        /**
         * @brief Read a C-style string from the stream without copying
         * @param str Output pointer to the string in the stream
         * @param maxlen Maximum length to read
         * @param outlen Actual length read
         * @return True if successful, false otherwise
         */
        bool ReadCCString(const char **str, size_t maxlen, size_t &outlen);

        /**
         * @brief Read a 32-bit integer from the stream
         * @param i Output integer value
         * @return True if successful, false otherwise
         */
        bool ReadInt32(int32_t &i);

        /**
         * @brief Read a 64-bit integer from the stream
         * @param i Output integer value
         * @return True if successful, false otherwise
         */
        bool ReadInt64(int64_t &i);

        /**
         * @brief Read a short integer from the stream
         * @param i Output short value
         * @return True if successful, false otherwise
         */
        bool ReadShort(short &i);

        /**
         * @brief Read a character from the stream
         * @param c Output character value
         * @return True if successful, false otherwise
         */
        bool ReadChar(char &c);

        /**
         * @brief Read all remaining data from the stream
         * @param szBuffer Output buffer
         * @param iLen Size of the output buffer
         * @return Number of bytes read
         */
        size_t ReadAll(char *szBuffer, size_t iLen) const;

        /**
         * @brief Check if the end of the stream has been reached
         * @return True if at the end, false otherwise
         */
        bool IsEnd() const;

        /**
         * @brief Get the current position in the stream
         * @return Pointer to the current position
         */
        const char *GetCurrent() const { return cur; }

    public:
        /**
         * @brief Read a length field from the stream and advance the cursor
         * @param len Output length value
         * @return True if successful, false otherwise
         */
        bool ReadLength(size_t &len);

        /**
         * @brief Read a length field without advancing the cursor
         * @param headlen Output header length
         * @param outlen Output content length
         * @return True if successful, false otherwise
         */
        bool ReadLengthWithoutOffset(size_t &headlen, size_t &outlen);

    private:
        BinaryStreamReader(const BinaryStreamReader &) = delete;
        BinaryStreamReader &operator=(const BinaryStreamReader &) = delete;

    private:
        const char *const ptr; /**< Pointer to the beginning of the data */
        const size_t len;      /**< Total length of the data */
        const char *cur;       /**< Current position in the data */
    };

    /**
     * @class BinaryStreamWriter
     * @brief Writes and serializes data to a binary stream
     */
    class BinaryStreamWriter final
    {
    public:
        /**
         * @brief Constructor
         * @param data Pointer to the output string buffer
         */
        BinaryStreamWriter(std::string *data);

        /**
         * @brief Default destructor
         */
        ~BinaryStreamWriter() = default;

        /**
         * @brief Get the raw data pointer
         * @return Pointer to the binary data
         */
        virtual const char *GetData() const;

        /**
         * @brief Get the total size of the binary data
         * @return Size of the binary data in bytes
         */
        virtual size_t GetSize() const;

        /**
         * @brief Write a C-style string to the stream
         * @param str Pointer to the string
         * @param len Length of the string
         * @return True if successful, false otherwise
         */
        bool WriteCString(const char *str, size_t len);

        /**
         * @brief Write a string to the stream
         * @param str String to write
         * @return True if successful, false otherwise
         */
        bool WriteString(const std::string &str);

        /**
         * @brief Write a double value to the stream
         * @param value Double value to write
         * @param isNULL Whether the value is NULL
         * @return True if successful, false otherwise
         */
        bool WriteDouble(double value, bool isNULL = false);

        /**
         * @brief Write a 64-bit integer to the stream
         * @param value Integer value to write
         * @param isNULL Whether the value is NULL
         * @return True if successful, false otherwise
         */
        bool WriteInt64(int64_t value, bool isNULL = false);

        /**
         * @brief Write a 32-bit integer to the stream
         * @param i Integer value to write
         * @param isNULL Whether the value is NULL
         * @return True if successful, false otherwise
         */
        bool WriteInt32(int32_t i, bool isNULL = false);

        /**
         * @brief Write a short integer to the stream
         * @param i Short value to write
         * @param isNULL Whether the value is NULL
         * @return True if successful, false otherwise
         */
        bool WriteShort(short i, bool isNULL = false);

        /**
         * @brief Write a character to the stream
         * @param c Character value to write
         * @param isNULL Whether the value is NULL
         * @return True if successful, false otherwise
         */
        bool WriteChar(char c, bool isNULL = false);

        /**
         * @brief Get the current position in the stream
         * @return Current position in bytes
         */
        size_t GetCurrentPos() const { return m_data->length(); }

        /**
         * @brief Flush the stream (currently a no-op)
         */
        void Flush();

        /**
         * @brief Clear the stream
         */
        void Clear();

    private:
        BinaryStreamWriter(const BinaryStreamWriter &) = delete;
        BinaryStreamWriter &operator=(const BinaryStreamWriter &) = delete;

    private:
        std::string *m_data; /**< Pointer to the output string buffer */
    };

} // end namespace

#endif //!__PROTOCOL_STREAM_H__