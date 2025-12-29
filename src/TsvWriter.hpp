#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

/**
 * @brief RAII wrapper around std::ofstream for writing TSV files with UTF-8 BOM support
 *
 * Provides a simple interface for opening TSV files with UTF-8 BOM (Byte Order Mark)
 * and writing lines or accessing the underlying stream for direct writes.
 */
class TsvWriter
{
  public:
    /**
     * @brief Default constructor - creates an unopened writer
     */
    constexpr TsvWriter() noexcept = default;

    /**
     * @brief Construct and open a file with optional UTF-8 BOM
     * @param path File path to open
     * @param writeBom If true, writes UTF-8 BOM (0xEF 0xBB 0xBF) at the start
     */
    explicit TsvWriter(const std::filesystem::path &path, const bool writeBom = true)
    {
        open(path, writeBom);
    }

    /**
     * @brief Open a file with optional UTF-8 BOM
     * @param path File path to open
     * @param writeBom If true, writes UTF-8 BOM (0xEF 0xBB 0xBF) at the start
     * @return true if the file was opened successfully
     */
    bool open(const std::filesystem::path &path, const bool writeBom = true)
    {
        m_stream.open(path, std::ios::out | std::ios::binary);
        if (!m_stream)
        {
            return false;
        }

        if (writeBom)
        {
            // Write UTF-8 BOM (EF BB BF)
            constexpr std::array<unsigned char, 3> bom = {{0xEF, 0xBB, 0xBF}};
            m_stream.write(reinterpret_cast<const char *>(bom.data()), static_cast<std::streamsize>(bom.size()));
        }

        return m_stream.good();
    }

    /**
     * @brief Close the file if open
     */
    void close() noexcept
    {
        if (m_stream.is_open())
        {
            m_stream.close();
        }
    }

    /**
     * @brief Write a line to the file
     * @param line The line to write (newline will be appended)
     */
    void writeLine(const std::string_view line)
    {
        if (m_stream.is_open())
        {
            m_stream << line << '\n';
        }
    }

    /**
     * @brief Get direct access to the underlying ofstream
     * @return Reference to the internal ofstream
     */
    std::ofstream &stream() noexcept
    {
        return m_stream;
    }

    /**
     * @brief Check if the file is open
     * @return true if the file is open
     */
    [[nodiscard]] bool isOpen() const noexcept
    {
        return m_stream.is_open();
    }

    /**
     * @brief Check if the stream is in a good state
     * @return true if the stream is good
     */
    [[nodiscard]] bool good() const noexcept
    {
        return m_stream.good();
    }

    /**
     * @brief Flush the buffer to disk
     */
    void flush()
    {
        if (m_stream.is_open())
        {
            m_stream.flush();
        }
    }

    /**
     * @brief Destructor - closes the file if open
     */
    ~TsvWriter()
    {
        close();
    }

    // Delete copy operations (file streams are not copyable)
    TsvWriter(const TsvWriter &) = delete;
    TsvWriter &operator=(const TsvWriter &) = delete;

    // Default move operations
    TsvWriter(TsvWriter &&) noexcept = default;
    TsvWriter &operator=(TsvWriter &&) noexcept = default;

  private:
    std::ofstream m_stream;
};
