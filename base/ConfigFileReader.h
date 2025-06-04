/**
 * @file ConfigFileReader.h
 * @brief A simple configuration file reader and writer.
 *
 * This class provides basic functionality to load key-value pairs from a config file,
 * retrieve values by key, and update values with optional file persistence.
 *
 * @author xiebaoma
 * @date 2025-06-04
 */

#ifndef __CONFIG_FILE_READER_H__
#define __CONFIG_FILE_READER_H__

#include <map>
#include <string>

/**
 * @class CConfigFileReader
 * @brief Parses and stores configuration values from a file.
 *
 * Supports reading and writing plain-text configuration files with key=value format.
 */
class CConfigFileReader
{
public:
    /**
     * @brief Constructs the config reader with a given file.
     * @param filename Path to the configuration file.
     */
    CConfigFileReader(const char *filename);

    /**
     * @brief Destructor.
     */
    ~CConfigFileReader();

    /**
     * @brief Retrieves the value associated with the specified key.
     * @param name Key name.
     * @return Pointer to the value string, or nullptr if not found.
     */
    char *getConfigName(const char *name);

    /**
     * @brief Sets or updates a key-value pair in memory and optionally persists it.
     * @param name Key name.
     * @param value Value string.
     * @return 0 on success, -1 on failure.
     */
    int setConfigValue(const char *name, const char *value);

private:
    /**
     * @brief Loads and parses the configuration file into memory.
     */
    void loadFile(const char *filename);

    /**
     * @brief Writes the current configuration map to the file.
     * @param filename Optional file path; if NULL, uses original path.
     * @return 0 on success, -1 on failure.
     */
    int writeFile(const char *filename = NULL);

    /**
     * @brief Parses a single line of key=value format and stores it.
     */
    void parseLine(char *line);

    /**
     * @brief Trims leading/trailing whitespace from a string.
     * @param name Input string to be trimmed.
     * @return Pointer to the trimmed string.
     */
    char *trimSpace(char *name);

private:
    bool m_load_ok;                                  ///< Indicates whether the file was loaded successfully.
    std::map<std::string, std::string> m_config_map; ///< Internal map of key-value pairs.
    std::string m_config_file;                       ///< Path to the loaded configuration file.
};

#endif //!__CONFIG_FILE_READER_H__
