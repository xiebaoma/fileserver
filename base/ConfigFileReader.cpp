/**
 * @file ConfigFileReader.cpp
 * @brief A simple configuration file reader and writer.
 *
 * This class provides basic functionality to load key-value pairs from a config file,
 * retrieve values by key, and update values with optional file persistence.
 *
 * @author xiebaoma
 * @date 2025-06-04
 */

#include "ConfigFileReader.h"
#include <stdio.h> //for snprintf
#include <string.h>

CConfigFileReader::CConfigFileReader(const char *filename)
{
    loadFile(filename);
}

CConfigFileReader::~CConfigFileReader()
{
}

char *CConfigFileReader::getConfigName(const char *name)
{
    if (!m_load_ok)
        return NULL;

    char *value = NULL;
    std::map<std::string, std::string>::iterator it = m_config_map.find(name);
    if (it != m_config_map.end())
    {
        value = (char *)it->second.c_str();
    }

    return value;
}

int CConfigFileReader::setConfigValue(const char *name, const char *value)
{
    if (!m_load_ok)
        return -1;

    std::map<std::string, std::string>::iterator it = m_config_map.find(name);
    if (it != m_config_map.end())
    {
        it->second = value;
    }
    else
    {
        m_config_map.insert(std::make_pair(name, value));
    }

    return writeFile();
}
void CConfigFileReader::loadFile(const char *filename)
{
    m_config_file.clear();
    m_config_file.append(filename);
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return;

    char buf[256];
    for (;;)
    {
        char *p = fgets(buf, 256, fp);
        if (!p)
            break;

        size_t len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = 0; // remove \n at the end

        char *ch = strchr(buf, '#'); // remove string start with #
        if (ch)
            *ch = 0;

        if (strlen(buf) == 0)
            continue;

        parseLine(buf);
    }

    fclose(fp);
    m_load_ok = true;
}

int CConfigFileReader::writeFile(const char *filename)
{
    FILE *fp = NULL;
    if (filename == NULL)
    {
        fp = fopen(m_config_file.c_str(), "w");
    }
    else
    {
        fp = fopen(filename, "w");
    }
    if (fp == NULL)
    {
        return -1;
    }

    char szPaire[128];
    std::map<std::string, std::string>::iterator it = m_config_map.begin();
    for (; it != m_config_map.end(); it++)
    {
        memset(szPaire, 0, sizeof(szPaire));
        snprintf(szPaire, sizeof(szPaire), "%s=%s\n", it->first.c_str(), it->second.c_str());
        size_t ret = fwrite(szPaire, strlen(szPaire), 1, fp);
        if (ret != 1)
        {
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return 0;
}

void CConfigFileReader::parseLine(char *line)
{
    char *p = strchr(line, '=');
    if (p == NULL)
        return;

    *p = 0;
    char *key = trimSpace(line);
    char *value = trimSpace(p + 1);
    if (key && value)
    {
        m_config_map.insert(std::make_pair(key, value));
    }
}

char *CConfigFileReader::trimSpace(char *name)
{
    // remove starting space or tab
    char *start_pos = name;
    while ((*start_pos == ' ') || (*start_pos == '\t') || (*start_pos == '\r'))
    {
        start_pos++;
    }

    if (strlen(start_pos) == 0)
        return NULL;

    // remove ending space or tab
    char *end_pos = name + strlen(name) - 1;
    while ((*end_pos == ' ') || (*end_pos == '\t') || (*end_pos == '\r'))
    {
        *end_pos = 0;
        end_pos--;
    }

    int len = (int)(end_pos - start_pos) + 1;
    if (len <= 0)
        return NULL;

    return start_pos;
}
