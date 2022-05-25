#include "../modules/CPP-test-unit/tester.hh"
#include "../csv.hh"
#include <stdexcept>
#include <fstream>
#include <cstdio>
#include <random>

constexpr auto input = R"("2018-08-01-00:44:59","NATAS31-GISU-30","itna2ex01019.omnitel.it","022987f8-b505-4acf-8b7c-6761f2c93a81","","","","","","","","","","","","","0.0","1.0","0.0","","0.0","","","0.0","3145728.0","","","","0.0","","6592.36376953125","","","","0.0","","","","0.0","","0.0","0.0","","","0.0","","","","100.0","","","","0.0","","","","","","","","","","","","","","0.0","","","","","","","","0.0","","","","3145728.0","","","","","","","","","-3145728.0","","0.00390625","","","","","0.0","","","","0.0","","0.00390625","-6592.36376953125","","0.0","3.0","","","","0.0","","","0.0","","","0.0","-6592.36376953125","","","","","","","","","","","","","83.0","","","","","","","","","","","","","","","","97.0","","","","","","100.0","0.0","","","1.0","","0.0","","0.0","0.0039088064804673195","","","","","0.0","","","100.0","","","","0.0","","0.0","","","","","","","","0.0","-1.0","0.0","0.0","","","","","","","0.0","","100.0","","1.0","","","","","","","","","","0.0","","","","","","0.00390625","","","0.0","","","","0.0","0.0","","0.0","-3145728.0","","","","0.0","","","","","17.0","","","","","","","0.0","","","","0.0","","","","","","","0.0","","","51.0","","","","","","","0.0","1.0","","","","0.0","100.0","","","","0.0","","","0.0","","","83.0","","","0.0","","","","","","","","","","0.0","","0.10573741048574448","","1.0","","","","","","","","","","","","","0.0","","","","","","","","","","","","","","3.694293260574341","","","1.0","","","","","","","","0.0","100.0","","","0.0","","","","","6592.36376953125","","","","","")";

[[noreturn]] void panic(const std::string& msg) {
    throw std::runtime_error(msg);
}

void assert_or_panic(bool assert, const std::string& msg = "Failed assertion") {
    if (!assert) {
        throw std::runtime_error(msg);
    }
}

tester t1([]() {
    auto parsed = csv::split_csv_line(input);
    auto reconstructed = csv::merge_csv_line(parsed);
    auto check = input == reconstructed;
    // std::cout << "Input:  " << input << std::endl;
    // std::cout << "Output: " << reconstructed << std::endl;
    std::cout << check << std::endl;
});

tester t2([]() {
    using namespace std::literals::string_literals;
    auto file = "data.csv"s;
    std::ifstream fin(file);
    if (!fin) {
        panic("Error opening file"s + file);
    }
    csv::reader r(fin, false, 1);
    while (r.can_read()) {
        std::cout << (std::string)r.getline() << std::endl;
    }
});

tester t3([](){
    using namespace std::literals::string_literals;
    auto file = "data.csv"s;
    std::ifstream fin(file);
    if (!fin) {
        panic("Error opening file"s + file);
    }
    csv::reader r(fin);
    std::cout << r.header_string() << std::endl;
    while (r.can_read()) {
        auto line = (std::string)r.getline();
        //std::cout << line << std::endl;
    }
});

tester t4([](){
    using namespace std::literals;
    constexpr int N = 10;
    std::vector<std::string> columns{"col1", "col1"};
    csv::writer w(std::cout, columns);
    assert_or_panic(w.column_count() == columns.size(), "Mismatch on column count"s);

    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution(30,77);

    for (int i{}; i!=N; ++i) {
        std::vector<double> v{distribution(generator), distribution(generator)};
        w.write_line(v);
    }
    assert_or_panic(w.line_count() == N+1, "Mismatch on line count"s);
});
