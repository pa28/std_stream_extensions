//
// Created by richard on 2019-03-22.
//

#pragma once

#include <locale>
#include <algorithm>

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

    /**
     * @brief An example of an output filter only stream buffer.
     * @details When data is written to this stream buffer xputs is called. This function filters the stream
     * and calls sputn on the next stream with the result.
     * @tparam CharT the type of character the buffer will process.
     * @tparam Traits the character traits type
     */
    template<typename CharT,
            typename Traits = std::char_traits<CharT>>
    class basic_fmtstreambuf : public std::basic_streambuf<CharT, Traits> {
    public:
        typedef CharT char_type;
        typedef Traits traits_type;
        typedef typename Traits::int_type int_type;
        typedef typename Traits::pos_type pos_type;
        typedef typename Traits::off_type off_type;

        size_t indent_increment{4};

        basic_fmtstreambuf() = delete;

        /**
         * @brief (constructor)
         * @details Creates a format buffer and attaches its input/output to the next buffer.
         * @param next the next buffer in the chain
         */
        explicit basic_fmtstreambuf(std::basic_streambuf<CharT, Traits> *next)
                : next{next} {
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
        std::basic_streambuf<CharT, Traits> *next{nullptr};
        bool at_start_of_line{true};
        size_t indent_level{0};
        std::locale locale{};
        size_t pending_indent{0};

        /**
         * @brief Output the number of spaces needed for indentation.
         * @param indentation_count required spaces
         * @return the number of spaces which could not be output and are left pending.
         */
        off_type do_indentation(off_type indentation_count) {
            // Spaces for making indentation more efficient.
            static char_type spaces[] = "                ";
            static constexpr size_t spaces_count = sizeof(spaces);

            for (pending_indent = indentation_count;
                 pending_indent > 0; pending_indent -= std::min(pending_indent, spaces_count))
                if (next->sputn(spaces, std::min(pending_indent, spaces_count)) !=
                    std::min(pending_indent, spaces_count))
                    return pending_indent; // Can not write all characters

            return 0;
        }

        /**
         * @brief A virtual method which implements the filter function.
         * @details The waiting output data is passed to the filter_write method for filtering
         * and insertion into the next buffer. Any unprocessed characters are left in obuf.
         * @param obuf a char_type* to the waiting data.
         * @param count the number of characters in obuf.
         * @return the number of characters process by the filter.
         */
        std::streamsize xsputn(const char_type *obuf, std::streamsize count) override {
            // Indentation left over.
            if (pending_indent > 0) {
                if (do_indentation(pending_indent))
                    return 0; // and still not done.
            }

            // Loop over the input buffer.
            for (off_type idx = 0; idx < count; ++idx) {
                // Indentation level increase request.
                if (traits_type::eq(obuf[idx], control_codes<char_type, traits_type>::indent_code)) {
                    indent();
                    continue;
                }

                // Indentation level decrease request.
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
                    if (do_indentation(indent_level * indent_increment))
                        return idx; // Can not write all characters
                }

                at_start_of_line = traits_type::eq(obuf[idx], EndOfLine);

                if (this->next->sputn(obuf + idx, 1) != 1)
                    return idx; // Can not write all characters
            }

            return count;
        }

        /**
         * @brief Read data from the next buffer, filtering it before writing in this buffer.
         * @param ibuf The buffer to accept the filtered input.
         * @param count The number of characters which may be written into ibuf
         * @return the actual number of characters written into ibuf
         */
        std::streamsize xsgetn(char_type *ibuf, std::streamsize count) override {
            return count;
        }
    };

    /**
     * @brief An output stream which uses basic_fmtstreambuf to format text.
     * @details Constructed with a std::basic_streambuf which is the ultimate destination, the stream inserts
     * a basic_fmtstreambuf which performs the formatting.
     * @tparam CharT the character type
     * @tparam Traits the character traits
     */
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
            delete filter;
        }

    protected:
        basic_fmtstreambuf<CharT, Traits> *filter{nullptr};
    };

    /**
     * Manipulators and support functions.
     */
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

    template<typename CharT, typename Traits = std::char_traits<CharT>>
    std::basic_string<CharT, Traits> basic_sft_end(CharT close_brace) {
        std::basic_string<CharT, Traits> code{};
        code.push_back(control_codes<CharT, Traits>::undent_code);
        code.push_back(control_codes<CharT, Traits>::end_of_line);
        code.push_back(close_brace);
        return code;
    }

    std::string begin(char open_brace) {
        return basic_begin<char>(open_brace);
    }

    std::string end(char close_brace) {
        return basic_end<char>(close_brace);
    }

    std::string sft_end(char close_brace) {
        return basic_sft_end<char>(close_brace);
    }

    using fmtstreambuf = basic_fmtstreambuf<char>;
    using fmtstream = basic_fmtstream<char>;

    using wfmtstreambuf = basic_fmtstreambuf<wchar_t>;
    using wfmtstream = basic_fmtstream<wchar_t>;
}