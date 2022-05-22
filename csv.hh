#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <istream>
#include <map>

namespace csv
{
    class eof : public std::exception {};

    std::vector<std::string> split_csv_line(const std::string& line, char delimiter = ',', char escape_char = '\\') {
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

    std::string merge_csv_line(const std::vector<std::string>& strings, char delimiter = ',', char escape_char = '\\', bool quoted = true) {
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

    class line
    {
    private:
        // if true indexes is valied
        bool header_included = false;
        // reference to the field into the reader
        const std::map<std::string, int>& _indexes;
        // contains parsed data
        decltype(split_csv_line("")) _data;
    public:
        // with header
        line(const std::map<std::string, int>& indexes, decltype(_data)&& data)
        : _indexes{indexes}, _data{data}
        {}

        // without header
        line(decltype(_data)&& data)
        : _indexes{std::map<std::string, int>()}, _data{data}
        {}

        const std::string& operator[](const std::string& key) const {
            return _data[_indexes.at(key)];
        }

        const auto& data() const {
            return _data;
        }

        const std::size_t size() const {
            return _data.size();
        }

        explicit operator std::string() {
            return merge_csv_line(_data);
        }
    };

    class reader
    {
    public:
        reader(std::istream& in, bool include_header = true, long long skip_lines = 0)
        : _in{in}, _read_header{include_header}
        {
            while (skip_lines--) {
                std::string line;
                if (!std::getline(in, line)) {
                    throw csv::eof();
                }
            }
            if (this->_read_header) {
                std::string line;
                if (!std::getline(in, line)) {
                    throw csv::eof();
                }
                _header = split_csv_line(line);
                // populate map
                for (int i{}; i!=_header.size(); ++i) {
                    _indexes[_header[i]] = i;
                }
                // check header coerence
                if (_indexes.size() != _header.size()) {
                    throw std::runtime_error("Duplicate field name in .csv");
                }
            }
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

        line getline() {
            std::string line;
            if (!can_read() || !std::getline(_in, line)) {
                throw csv::eof();
            }
            ++_line_counter;
            auto data = split_csv_line(line);
            if (_read_header && data.size() != _header.size()) {
                throw std::runtime_error("Malformed line " + _line_counter);
            }
            return csv::line(_indexes, std::move(data));
        }
    private:
        // number of field for single line
        int _line_length{};
        // count the number of parsed lines
        decltype(0LL) _line_counter{};
        // input stream
        std::istream& _in;
        // first csv line is header
        bool _read_header = true;
        // associate each column to its offset
        // in the csv
        std::map<std::string, int> _indexes;
        // header
        std::vector<std::string> _header;
    };
} // namespace csv
