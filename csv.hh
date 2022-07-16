
#ifndef CSV
#define CSV

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <istream>
#include <map>
#include <memory>
#include <assert.h>

namespace csv
{
    class eof : public std::exception {};

    inline std::vector<std::string> split_csv_line(const std::string& line, char delimiter = ',', char escape_char = '\\') {
        constexpr char quote = '"';
        // returned sequence of strings
        std::vector<std::string> ans;
        // current item breing parsed
        std::string tmp;
        // next char must be inserted in a new string
        bool next_new = true;
        // is current item quoted?
        bool quoted = false;
        // previous string has been completely read, next character should be a delimiter
        // it is used for quoted strings, when termination caracter is " and not delimiter
        bool ended = false;
        // escape character found, next char should be captured as is
        bool escaped = false;
        for (auto c : line) {
            // handle begin of a new string
            if (next_new) {
                next_new = false;
                // new character of string
                // is this quoted?
                if (c == quote) {
                    // will read until a new quote "
                    quoted = true;
                } else if (c == delimiter) {
                    // empty string found! delimiter caracter found
                    ans.push_back(std::string());
                    // next will be new!
                    next_new = true;
                } else {
                    // add char to string
                    tmp.push_back(c);
                }
            // handle termination of a string, when a delimiter is expected
            // If delimiter is escaped it should be ignored
            } else if (ended) {
                // check for delimiter character
                if (c != delimiter) {
                    throw std::runtime_error("Malformed input");
                }
                ended = false;
                next_new = true;
            } else if (c == delimiter && !quoted && !escaped) {
                // delimiter successfully found!
                // next will be a new string
                ended = false;
                next_new = true;
                // add to list
                ans.push_back(std::move(tmp));
            // found non-escaped escape character, next char must be get as is
            } else if (c == escape_char && !escaped) {
                escaped = true;
            // take char as is
            } else if (escaped) {
                escaped = false;
                tmp.push_back(c);
            // found a quote
            } else if (c == quote) {
                // if this string was not quoted this is a mess!
                if (!quoted) {
                    throw std::runtime_error("Malformed input");
                }
                // else quoted string is terminated
                quoted = false;
                // and next char should be a delimiter or end of line
                ended = true;
                // now string should be pushed
                ans.push_back(std::move(tmp));
            // take char after having verified all previous check
            } else {
                tmp.push_back(c);
            }
        }
        // check status is coherent with end of line
        // last char cannot be an escape character, and
        // we cannot be looking for a quote
        if (escaped || quoted) {
            throw std::runtime_error("Malformed input");
        }
        // then, if string was not quoted and partially read it's ok and
        // must be pushed
        if (!next_new && !ended) {
            ans.push_back(std::move(tmp));
        }

        return ans;
    }

    inline std::string merge_csv_line(const std::vector<std::string>& strings, char delimiter = ',', char escape_char = '\\', bool quoted = true) {
        constexpr char quote = '"';
        std::remove_const<std::remove_reference<decltype(strings)>::type>::type escaped;
        std::string ans;
        bool first = true;
        for (const auto& s : strings) {
            if (first) {
                first = false;
            } else {
                ans += delimiter;
            }
            // not quoted strings cannot contain escape characters
            // so nothing is escaped
            if (!quoted) {
                ans += s;
            // escape and then add quotes
            } else {
                std::remove_const<std::remove_reference<decltype(s)>::type>::type tmp;
                for (auto c : s)
                {
                    switch (c)
                    {
                    case quote: tmp.push_back(escape_char); tmp.push_back(quote); break;
                    default:    tmp.push_back(c); break;
                    }
                }
                ans += '"' + tmp + '"';
            }
        }
        return ans;
    }

    template <typename T>
    inline std::vector<std::string> cast_line(const std::vector<T>& line) {
        std::vector<std::string> ans; ans.reserve(line.size());
        for (const auto& x : line) {
            ans.push_back(std::to_string(x));
        }
        return ans;
    }

    template <>
    inline std::vector<std::string> cast_line(const std::vector<std::string>& line) {
        return line;
    }

    class line
    {
    private:
        // if true indexes is valied
        bool header_included = false;
        // reference to the field into the reader
        std::shared_ptr<const std::map<std::string, int>> _indexes;
        // contains parsed data
        decltype(split_csv_line("")) _data;
    public:
        // with header
        line(const std::shared_ptr<std::map<std::string, int>>& indexes, decltype(_data)&& data)
        : _indexes{indexes}, _data{std::move(data)}
        {}

        line(const std::shared_ptr<const std::map<std::string, int>>& indexes, decltype(_data)&& data)
        : _indexes{indexes}, _data{std::move(data)}
        {}

        // without header
        line(decltype(_data)&& data)
        : _indexes{}, _data{std::move(data)}
        {}

        const std::string& operator[](const std::string& key) const {
            return _data[_indexes->at(key)];
        }

        const auto& data() const {
            return _data;
        }

        std::size_t size() const {
            return _data.size();
        }

        explicit operator std::string() {
            return merge_csv_line(_data);
        }
    };

    class writer
    {
    public:
        writer(std::unique_ptr<std::ostream>&& out, const std::vector<std::string>& header)
        : _output(std::move(out)), _out{*_output}, _header{header}, _line_length{header.size()}, _header_written{true}
        {
            write_line(header);
        }

        writer(std::ostream& out, std::size_t line_length)
        : _out{out}, _line_length{line_length}
        {}

        writer(std::ostream& out, const std::vector<std::string>& header)
        : _out{out}, _header{header}, _line_length{header.size()}, _header_written{true}
        {
            write_line(header);
        }

        auto line_length() const {
            return _line_length;
        }

        template <typename T>
        writer& write_line(const std::vector<T>& line) {
            return write_line(cast_line(line));
        }

        writer& write_line(const std::vector<std::string>& line) {
            ++_written;
            _out << merge_csv_line(line) << '\n';
            return *this;
        }

        // Return total number of written lines, header included
        auto line_count() const {
            return _written;
        }

        // Return number of written lines, header escluded
        auto row_count() const {
            return decltype(_written)(_written - _header_written);
        }

        auto column_count() const {
            return _line_length;
        }
    private:
        // output stream
        // used to take stream ownership
        std::unique_ptr<std::ostream> _output;
        // use a reference to caputer all cases
        std::ostream& _out;
        // vector of column names, if empty header will not be included
        std::vector<std::string> _header;
        // number of items per row elements
        std::size_t _line_length{};
        // count written lines
        std::size_t _written{};
        // was header written
        bool _header_written{};
        // count the number of written lines
        decltype(0LL) _line_counter{};
    };

    class reader
    {
    public:
        reader(std::unique_ptr<std::istream>&& in, bool include_header = true, long long skip_lines = 0, bool skip_duplicate = true)
        : _input(std::move(in)), _in{*_input}, _read_header{include_header}, _skip_duplicate{skip_duplicate}
        {
            skip(skip_lines);
            handle_header();
        }

        reader(std::istream& in, bool include_header = true, long long skip_lines = 0, bool skip_duplicate = true)
        : _in{in}, _read_header{include_header}, _skip_duplicate{skip_duplicate}
        {
            skip(skip_lines);
            handle_header();
        }

        bool can_read() {
            // reached eof
            if (_in.eof()) {
                return false;
            }
            char c;
            if (_in >> c) {
                _in.putback(c);
                return true;
            } else {
                return false;
            }
        }

        // Was the header read?
        auto has_header() const {
            return _read_header;
        }

        // retrive const reference to header column names
        const auto& header() const {
            if (!_read_header) {
                throw std::logic_error("Header has not been previously read");
            }
            return _header;
        }

        auto header_string() const {
            return merge_csv_line(header());
        }

        line getline() {
            if (!_buffered_line.empty()) {
                return csv::line(std::move(_buffered_line));
            }
            auto data = getline_internal();
            return csv::line(_indexes, std::move(data));
        }

        // skip n input lines
        void skip(long long n) {
            while (n--) {
                std::string line;
                if (!std::getline(_in, line)) {
                    throw csv::eof();
                }
            }
        }

        // Number of columns available in the .csv
        auto column_count() const {
            return _line_length;
        }

        // Return the number of data line read
        auto line_count() const {
            return _line_counter - !_buffered_line.empty();
        }
    private:
        void handle_header() {
            if (this->_read_header) {
                std::string line;
                if (!std::getline(_in, line)) {
                    throw csv::eof();
                }
                _header = split_csv_line(line);
                _line_length = _header.size();
                _indexes = std::make_shared<std::map<std::string, int>>(std::map<std::string, int>());
                // populate map
                for (std::size_t i{}, inserted{}; i!=_header.size(); ++i) {
                    const auto& column = _header[i];
                    if (_indexes->find(column) != _indexes->end()) {
                        // element already exists
                        if (_skip_duplicate) {
                            // column should be ignored when
                            // parsing data
                            _duplicated_columns.push_back(i);
                        } else {
                            // error!
                            throw std::runtime_error("Duplicate field '" + column + "' in .csv");
                        }
                    } else {
                        // insert new element
                        _indexes->operator[](column) = inserted++;
                    }
                }
                // remove duplicated columns by header
                if (!_duplicated_columns.empty()) {
                    remove_duplicated_columns(_header);
                }
                // check header coerence
                if (!_skip_duplicate && _indexes->size() != _header.size()) {
                    throw std::runtime_error("Duplicate field name in .csv");
                }
            } else {
                // read first data row
                _buffered_line = getline_internal();
            }
            // check 
            if (!_line_length) {
                throw std::runtime_error("Initial line found empty");
            }
        }

        std::vector<std::string> getline_internal() {
            using namespace std::literals;

            std::string line;
            if (!can_read() || !std::getline(_in, line)) {
                throw csv::eof();
            }
            auto data = split_csv_line(line);
            if (_line_counter == 0 && !_read_header) {
                // if first read (header was ignored)
                // take current line
                _line_length = data.size();
            } else if (data.size() != _line_length) {
                throw std::runtime_error("Malformed line "s + std::to_string(_line_counter));
            }
            if (_skip_duplicate) {
                remove_duplicated_columns(data);
            }
            ++_line_counter;
            if (_read_header && data.size() != _header.size()) {
                throw std::runtime_error("Malformed line "s + std::to_string(_line_counter));
            }
            return data;
        }

        // if duplicated colums were to be ignore,
        // remove them
        void remove_duplicated_columns(std::vector<std::string>& data) {
            if (_skip_duplicate) {
                for (auto it = _duplicated_columns.rbegin(); it != _duplicated_columns.rend(); ++it) {
                    data.erase(data.begin() + *it);
                }
            }
        }

        // number of field for single line,
        // calculated before removing duplicate
        // columns
        std::size_t _line_length{};
        // count the number of parsed lines
        std::size_t _line_counter{};
        // input stream
        // used to take stream ownership
        std::unique_ptr<std::istream> _input;
        // use a reference to caputer all cases
        std::istream& _in;
        // first csv line is header
        bool _read_header = true;
        // associate each column to its offset
        // in the csv
        std::shared_ptr<std::map<std::string, int>> _indexes;
        // header
        std::vector<std::string> _header;

        // should column with duplicated names
        // be ignored in csv?
        // valid only if _read_header
        bool _skip_duplicate = false;
        // if _read_header && !_skip_duplicate
        // contains indexes of columns to be skipped
        std::vector<int> _duplicated_columns;

        // Buffer line potentially read to detect
        // column number if header is not required
        // to be read
        std::vector<std::string> _buffered_line;
    };
} // namespace csv

#endif
