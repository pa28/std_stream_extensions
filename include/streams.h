//
// Created by richard on 2019-03-21.
//

#pragma once

#include <iostream>
#include <iomanip>

namespace exp {

    /**
     * @brief A sub-class of std::basic_streambuf that can be inserted, using a companion stream class,
     * on top of a streambuf to filter the byte stream.
     * @tparam CharT the character type
     * @tparam Traits the character traits type (usually std::char_traits<CharT>
     * @tparam BufferSize the size of the buffers implemented, default 4096 characters
     */
    template<
            typename CharT,
            typename Traits = std::char_traits<CharT>,
            size_t WriteBufferSize = 4096,
            size_t ReadBufferSize = 4096>
    class basic_filter_streambuf : public std::basic_streambuf<CharT, Traits> {
    public:
        typedef CharT char_type;
        typedef Traits traits_type;
        typedef typename Traits::int_type int_type;
        typedef typename Traits::pos_type pos_type;
        typedef typename Traits::off_type off_type;
        static constexpr size_t write_buffer_size = WriteBufferSize;
        static constexpr size_t read_buffer_size = ReadBufferSize;

        basic_filter_streambuf() = delete;

        /**
         * @brief (constructor)
         * @details Creates a filter buffer and attaches its input/output to the next buffer.
         * @param next the next buffer in the chain
         */
        explicit basic_filter_streambuf(std::basic_streambuf<CharT, Traits> *next)
                : next{next} {
            this->setp(obuf, obuf + sizeof(obuf));
            this->setg(ibuf, ibuf + 8, ibuf + 8);
        }

        /**
         * @brief (destructor)
         * @details Sync (flush) the output when the buffer is destroyed.
         */
        ~basic_filter_streambuf() override {
            sync();
        }

    protected:

        std::basic_streambuf<CharT, Traits> *next{nullptr};     ///< Pointer to the next buffer
        CharT obuf[write_buffer_size];        ///< The output buffer
        CharT ibuf[read_buffer_size + 8];    ///< The input buffer

        /**
         * @brief A virtual method which implements the filter function.
         * @details The waiting output data is passed to the filter_write method for filtering
         * and insertion into the next buffer. Any unprocessed characters are left in obuf.
         * @param obuf a char_type* to the waiting data.
         * @param count the number of characters in obuf.
         * @return the number of characters process by the filter.
         */
        virtual std::streamsize filter_write(const char_type *obuf, std::streamsize count) {
            return next->sputn(obuf, count);
        }

        /**
         * @brief Synchronize this buffer with the next, passing data through the buffer.
         * @return -1 on failure, 0 otherwise.
         */
        int sync() override {
            auto n = filter_write(obuf, this->pptr() - obuf);

            if (n < 0) {
                return -1;
            } else if (n == (this->pptr() - obuf)) {
                this->setp(obuf, obuf + sizeof(obuf));
            } else {
                for (ssize_t cp = n; cp < (sizeof(obuf) - n); ++cp) {
                    obuf[cp - n] = obuf[cp];
                }
                this->setp(obuf, obuf + sizeof(obuf));
                this->pbump(static_cast<int>(sizeof(obuf) - n));
            }
            return 0;
        }

        /**
         * @brief Handle overflow characters
         * @param c the overflow character
         * @return EOF if sync() fails, otherwise c as an integer.
         */
        int_type overflow(int_type c) override {
            if (sync() < 0)
                return traits_type::eof();

            if (traits_type::not_eof(c)) {
                char_type cc = traits_type::to_char_type(c);
                this->xsputn(&cc, 1);
            }

            return traits_type::to_int_type(c);
        }

        /**
         * @brief Read data from the next buffer, filtering it before writing in this buffer.
         * @param ibuf The buffer to accept the filtered input.
         * @param count The number of characters which may be written into ibuf
         * @return the actual number of characters written into ibuf
         */
        virtual std::streamsize filter_read(char_type *ibuf, std::streamsize count) {
            return next->sgetn(ibuf + 8, sizeof(ibuf) - 8);
        }

        /**
         * @brief Handle underflow conditions by reading data from the next buffer.
         * @return The number of characters read into this buffer.
         */
        int_type underflow() override {
            auto n = filter_read(ibuf + 8, sizeof(ibuf) - 8);

            if (n < 0) {
                return traits_type::eof();
            }
            this->setg(ibuf, ibuf + 8, ibuf + 8 + n);
            if (n) {
                return traits_type::to_int_type(*(ibuf + 8));
            }
            return traits_type::eof();
        }
    };

    /**
     * @brief The companion ostream to basic_filter_streambuf.
     * @tparam CharT the character type.
     * @tparam Traits the character traits type (usually std::char_traits<CharT>
     * @tparam BufferSize the number of characters to store in the buffer
     */
    template<
            typename CharT,
            typename Traits = std::char_traits<CharT>,
            size_t WriteBufferSize = 1024,
            size_t ReadBufferSize = 8>
    class basic_filter_ostream : public std::basic_ostream<CharT, Traits> {
    public:
        basic_filter_ostream() = delete;

        explicit basic_filter_ostream(std::basic_streambuf<CharT, Traits> *rdbuf)
                : std::basic_ostream<CharT, Traits>{rdbuf} {
            basic_filter_buffer = new basic_filter_streambuf<CharT, Traits, WriteBufferSize, ReadBufferSize>{rdbuf};
            this->set_rdbuf(basic_filter_buffer);
        }

        ~basic_filter_ostream() override {
            delete basic_filter_buffer;
        }

    protected:
        basic_filter_streambuf<CharT, Traits, WriteBufferSize, ReadBufferSize> *basic_filter_buffer{nullptr};
    };

    using filter_streambuf = basic_filter_streambuf<char>;
    using filter_ostream = basic_filter_ostream<char>;
}
