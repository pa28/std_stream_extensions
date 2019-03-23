//
// Created by richard on 2019-03-22.
//

#pragma once

#include <locale>
#include "streams.h"

namespace fmt {

    /**
     * Set values of the formatting control codes.
     */
    enum ControlCodes {
        EndOfLine = 0xa,
        Indent = 0xf,
        Undent = 0xe,
    };

    template<typename CharT,
            typename Traits = std::char_traits<CharT>>
    struct control_codes {
        typedef CharT char_type;
        typedef Traits traits_type;
        static constexpr char_type end_of_line = traits_type::to_char_type(EndOfLine);
        static constexpr char_type indent_code = traits_type::to_char_type(Indent);
        static constexpr char_type undent_code = traits_type::to_char_type(Undent);
    };

    template<typename CharT,
            typename Traits = std::char_traits<CharT>,
            size_t WriteBufferSize = 64,
            size_t ReadBufferSize = 8>
    class basic_fmtstreambuf : public exp::basic_filter_streambuf<CharT, Traits, WriteBufferSize, ReadBufferSize> {
    public:
        typedef CharT char_type;
        typedef Traits traits_type;
        typedef typename Traits::int_type int_type;
        typedef typename Traits::pos_type pos_type;
        typedef typename Traits::off_type off_type;
        static constexpr size_t write_buffer_size = WriteBufferSize;
        static constexpr size_t read_buffer_size = ReadBufferSize;

        size_t indent_increment{4};

        basic_fmtstreambuf() = delete;

        /**
         * @brief (constructor)
         * @details Creates a format buffer and attaches its input/output to the next buffer.
         * @param next the next buffer in the chain
         */
        explicit basic_fmtstreambuf(std::basic_streambuf<CharT, Traits> *next)
                : exp::basic_filter_streambuf<CharT, Traits, WriteBufferSize, ReadBufferSize>{next} {
        }

        ~basic_fmtstreambuf() override {
            this->sync();
        }

        basic_fmtstreambuf &indent() {
            ++indent_level;
            return *this;
        }

        basic_fmtstreambuf &undent() {
            if (indent_level)
                --indent_level;
            return *this;
        }

    protected:
        bool at_start_of_line{true};
        size_t indent_level{0};
        std::locale locale{};

        /**
         * @brief A virtual method which implements the filter function.
         * @details The waiting output data is passed to the filter_write method for filtering
         * and insertion into the next buffer. Any unprocessed characters are left in obuf.
         * @param obuf a char_type* to the waiting data.
         * @param count the number of characters in obuf.
         * @return the number of characters process by the filter.
         */
        std::streamsize filter_write(const char_type *obuf, std::streamsize count) override {
            for (off_type idx = 0; idx < count; ++idx) {
                if (traits_type::eq(obuf[idx], control_codes<char_type, traits_type>::indent_code)) {
                    indent();
                    continue;
                }

                if (traits_type::eq(obuf[idx], control_codes<char_type, traits_type>::undent_code)) {
                    undent();
                    continue;
                }

                if (at_start_of_line) {
                    // Do not print whitespace at the start of a line,
                    if (std::isspace(obuf[idx], locale)) {
                        continue;
                    }
                    // Except as the indicated indentation before the first non-space character.
                    for (off_type ind = 0; ind < indent_level * indent_increment; ++ind)
                        if (this->next->sputc(' ') == traits_type::eof())
                            return idx;
                }

                at_start_of_line = traits_type::eq(obuf[idx], EndOfLine);

                if (this->next->sputc(obuf[idx]) == traits_type::eof())
                    return idx;
            }

            return count;
        }

        /**
         * @brief Read data from the next buffer, filtering it before writing in this buffer.
         * @param ibuf The buffer to accept the filtered input.
         * @param count The number of characters which may be written into ibuf
         * @return the actual number of characters written into ibuf
         */
        std::streamsize filter_read(char_type *ibuf, std::streamsize count) override {
            return count;
        }
    };

    template<typename CharT,
            typename Traits = std::char_traits<CharT>>
    class basic_fmtstream : public std::basic_ostream<CharT, Traits> {
    public:
        typedef CharT char_type;
        typedef Traits traits_type;
        typedef typename Traits::int_type int_type;
        typedef typename Traits::pos_type pos_type;
        typedef typename Traits::off_type off_type;

        basic_fmtstream() = delete;

        explicit basic_fmtstream(std::basic_streambuf<CharT, Traits> *next)
                : std::basic_ostream<CharT, Traits>{next} {
            filter = new basic_fmtstreambuf<CharT, Traits>{next};
            this->set_rdbuf(filter);
        }

        ~basic_fmtstream() override {
            this->flush();
            delete filter;
        }

    protected:
        basic_fmtstreambuf<CharT, Traits> *filter{nullptr};
    };

    template<typename CharT, typename Traits>
    std::basic_ostream<CharT, Traits> &eol(std::basic_ostream<CharT, Traits> &ostream) {
        return ostream << control_codes<CharT, Traits>::end_of_line;
    }

    template<typename CharT, typename Traits>
    std::basic_ostream<CharT, Traits> &indent(std::basic_ostream<CharT, Traits> &ostream) {
        return ostream << control_codes<CharT, Traits>::indent_code;
    }

    template<typename CharT, typename Traits>
    std::basic_ostream<CharT, Traits> &undent(std::basic_ostream<CharT, Traits> &ostream) {
        return ostream << control_codes<CharT, Traits>::undent_code;
    }

    template<typename CharT, typename Traits = std::char_traits<CharT>>
    std::basic_string<CharT, Traits> basic_begin(CharT open_brace) {
        std::basic_string<CharT, Traits> code{};
        code.push_back(open_brace);
        code.push_back(control_codes<CharT, Traits>::indent_code);
        code.push_back(control_codes<CharT, Traits>::end_of_line);
        return code;
    }

    template<typename CharT, typename Traits = std::char_traits<CharT>>
    std::basic_string<CharT, Traits> basic_end(CharT close_brace) {
        std::basic_string<CharT, Traits> code{};
        code.push_back(control_codes<CharT, Traits>::undent_code);
        code.push_back(control_codes<CharT, Traits>::end_of_line);
        code.push_back(close_brace);
        code.push_back(control_codes<CharT, Traits>::end_of_line);
        return code;
    }

    std::string begin(char open_brace) {
        return basic_begin<char>(open_brace);
    }

    std::string end(char close_brace) {
        return basic_end<char>(close_brace);
    }
}