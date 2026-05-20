#include <iostream>
#include <cstring>
#include <utility>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <omp.h>

class CString {
protected:
    void cleanup();
    void copyFrom(const CString& other);
    void copyFrom(const char* str);
    char* s = nullptr;
    size_t n = 0;

public:
    static bool verbose;

    CString();
    CString(size_t size);
    CString(const char* str);
    CString(const CString& other);
    CString(CString&& other);
    virtual ~CString();

    CString& operator=(const CString& other);
    CString& operator=(CString&& other);
    CString& operator=(const char* str);
    char& operator[](size_t index);
    const char& operator[](size_t index) const;
    size_t length() const { return n; }

    friend std::ostream& operator<<(std::ostream& os, const CString& str);

    virtual void Show() = 0;

    static int Input(const char* filename, std::vector<CString*>& v, const std::vector<class CFabric*>& fabrics);
    static void autotest();
};

bool CString::verbose = true;

class CFabric {
public:
    virtual ~CFabric() {}
    virtual CString* Create(const std::string& data) = 0;
    virtual int GetType() const = 0;
};

class RoundFabric : public CFabric {
public:
    CString* Create(const std::string& data) override;
    int GetType() const override { return 1; }
};

class SquareFabric : public CFabric {
public:
    CString* Create(const std::string& data) override;
    int GetType() const override { return 2; }
};

class Round_Child : public CString {
public:
    using CString::CString;
    ~Round_Child() override { if (verbose) std::cout << "Round_Child destructor" << std::endl; }
    void Show() override {
        std::cout << "(";
        if (s) std::cout << s;
        std::cout << ")" << std::endl;
    }
    Round_Child operator+(const CString& other) const {
        if (verbose) std::cout << "Round_Child operator+" << std::endl;
        Round_Child result(n + other.n);
        if (n > 0) strcpy(result.s, s);
        if (other.n > 0) strcpy(result.s + n, other.s);
        return result;
    }
};

class Square_Child : public CString {
public:
    using CString::CString;
    ~Square_Child() override { if (verbose) std::cout << "Square_Child destructor" << std::endl; }
    void Show() override {
        std::cout << "[";
        if (s) std::cout << s;
        std::cout << "]" << std::endl;
    }
    Square_Child operator+(const CString& other) const {
        if (verbose) std::cout << "Square_Child operator+" << std::endl;
        Square_Child result(n + other.n);
        if (n > 0) strcpy(result.s, s);
        if (other.n > 0) strcpy(result.s + n, other.s);
        return result;
    }
};

CString* RoundFabric::Create(const std::string& data) {
    Round_Child* obj = new Round_Child;
    if (!data.empty()) *obj = data.c_str();
    return obj;
}

CString* SquareFabric::Create(const std::string& data) {
    Square_Child* obj = new Square_Child;
    if (!data.empty()) *obj = data.c_str();
    return obj;
}

CString::CString() : s(nullptr), n(0) {
    if (verbose) std::cout << "Default constructor" << std::endl;
}

CString::CString(size_t size) : n(size) {
    if (verbose) std::cout << "Constructor" << std::endl;
    if (n > 0) { s = new char[n + 1]; s[n] = '\0'; }
}

CString::CString(const char* str) {
    if (verbose) std::cout << "C-string Constructor" << std::endl;
    if (str) { n = strlen(str); s = new char[n + 1]; strcpy(s, str); }
    else { s = nullptr; n = 0; }
}

CString::CString(const CString& other) {
    if (verbose) std::cout << "Copy constructor" << std::endl;
    copyFrom(other);
}

CString::CString(CString&& other) : s(other.s), n(other.n) {
    if (verbose) std::cout << "Using move constructor" << std::endl;
    other.s = nullptr; other.n = 0;
}

CString::~CString() {
    if (verbose) std::cout << "CString destructor" << std::endl;
    cleanup();
}

CString& CString::operator=(const CString& other) {
    if (verbose) std::cout << "Copy assignment operator" << std::endl;
    if (this != &other) { cleanup(); copyFrom(other); }
    return *this;
}

CString& CString::operator=(CString&& other) {
    if (verbose) std::cout << "Move assignment operator" << std::endl;
    if (this != &other) { cleanup(); s = other.s; n = other.n; other.s = nullptr; other.n = 0; }
    return *this;
}

CString& CString::operator=(const char* str) {
    if (verbose) std::cout << "Assignment from C-string" << std::endl;
    if (s != str) { cleanup(); if (str) { n = strlen(str); s = new char[n + 1]; strcpy(s, str); } }
    return *this;
}

char& CString::operator[](size_t index) { return s[index]; }
const char& CString::operator[](size_t index) const { return s[index]; }

std::ostream& operator<<(std::ostream& os, const CString& str) {
    if (str.s) os << str.s;
    return os;
}

void CString::cleanup() { delete[] s; s = nullptr; n = 0; }

void CString::copyFrom(const CString& other) {
    if (other.s) { n = other.n; s = new char[n + 1]; strcpy(s, other.s); }
    else { s = nullptr; n = 0; }
}

void CString::copyFrom(const char* str) {
    if (str) { n = strlen(str); s = new char[n + 1]; strcpy(s, str); }
    else { s = nullptr; n = 0; }
}

int CString::Input(const char* filename, std::vector<CString*>& v, const std::vector<CFabric*>& fabrics) {
    std::ifstream f(filename);
    if (!f.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return -1;
    }
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        int type;
        ss >> type;
        std::string data;
        if (ss.peek() == ' ') ss.get();
        std::getline(ss, data);
        CString* obj = nullptr;
        for (auto* fab : fabrics) {
            if (fab->GetType() == type) {
                obj = fab->Create(data);
                break;
            }
        }
        if (obj) v.push_back(obj);
        else std::cerr << "Unknown type " << type << " in line: " << line << std::endl;
    }
    return 0;
}

std::string random_string(size_t length) {
    static const char charset[] = 
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    const size_t max_index = sizeof(charset) - 1;
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i)
        result += charset[rand() % max_index];
    return result;
}

void create_random_strings(FILE* f) {
    if (!f) return;
    srand(static_cast<unsigned>(time(nullptr)));
    for (int i = 0; i < 1000; ++i) {
        int type = (rand() % 2) + 1;
        size_t str_len = rand() % 16 + 5;
        std::string text = random_string(str_len);
        fprintf(f, "%d %s\n", type, text.c_str());
    }
}

void CString::autotest() {
    std::cout << "\n--- CString base functionality test (using Round_Child) ---" << std::endl;
    Round_Child s1("Hello");
    Round_Child s2 = "World";
    Round_Child s3 = s1 + s2;
    std::cout << "s3: " << s3 << std::endl;

    std::cout << "\n--- Copy operations ---" << std::endl;
    Round_Child s4 = s3;
    Round_Child s5;
    s5 = s4;
    std::cout << "s5: " << s5 << std::endl;

    std::cout << "\n--- Move operations ---" << std::endl;
    Round_Child s6 = std::move(s5);
    std::cout << "s6: " << s6 << std::endl;
    std::cout << "s5 after move: " << s5 << " (empty)" << std::endl;

    Round_Child s7;
    s7 = std::move(s6);
    std::cout << "s7: " << s7 << std::endl;
    std::cout << "s6 after move assignment: " << s6 << " (empty)" << std::endl;

    std::cout << "\n--- Testing Input with factory vector ---" << std::endl;
    std::vector<CFabric*> fabrics;
    fabrics.push_back(new RoundFabric);
    fabrics.push_back(new SquareFabric);
    const char* testFileName = "test_input.txt";
    std::ofstream testFile(testFileName);
    testFile << "1 Hello world\n2 Square brackets test\n1 Another round\n3 Unknown ignore\n";
    testFile.close();
    std::vector<CString*> objects;
    CString::Input(testFileName, objects, fabrics);
    for (auto* obj : objects) obj->Show();
    for (auto* obj : objects) delete obj;
    for (auto* fab : fabrics) delete fab;
    std::remove(testFileName);

    std::cout << "\n--- Test of polymorphic destructors ---" << std::endl;
    std::vector<CString*> vec;
    vec.push_back(new Round_Child("Round object"));
    vec.push_back(new Square_Child("Square object"));
    for (auto* p : vec) p->Show();
    for (auto* p : vec) delete p;

    std::cout << "\n========== PARALLEL PERFORMANCE TEST (OpenMP) ==========" << std::endl;
    CString::verbose = false;

    const int N = 10000;
    const int THREADS = 4;

    std::vector<Round_Child> strings;
    strings.reserve(N);
    for (int i = 0; i < N; ++i)
        strings.emplace_back("Hello");

    auto start_seq = std::chrono::high_resolution_clock::now();
    Round_Child sum_seq;
    for (const auto& str : strings) {
        sum_seq = sum_seq + str;
    }
    auto end_seq = std::chrono::high_resolution_clock::now();
    auto seq_time = std::chrono::duration<double, std::milli>(end_seq - start_seq).count();

    auto start_par = std::chrono::high_resolution_clock::now();
    std::vector<Round_Child> partial_sums(THREADS);

    #pragma omp parallel num_threads(THREADS)
    {
        int tid = omp_get_thread_num();
        int begin = tid * (N / THREADS);
        int end = (tid == THREADS - 1) ? N : (tid + 1) * (N / THREADS);
        Round_Child local_sum;
        for (int i = begin; i < end; ++i) {
            local_sum = local_sum + strings[i];
        }
        partial_sums[tid] = std::move(local_sum);
    }

    Round_Child total_par;
    for (auto& ps : partial_sums) {
        total_par = total_par + ps;
    }
    auto end_par = std::chrono::high_resolution_clock::now();
    auto par_time = std::chrono::duration<double, std::milli>(end_par - start_par).count();

    std::cout << "Sequential result length: " << sum_seq.length() << std::endl;
    std::cout << "Parallel   result length: " << total_par.length() << std::endl;
    std::cout << "\nTime (ms):" << std::endl;
    std::cout << "  Sequential: " << seq_time << " ms" << std::endl;
    std::cout << "  Parallel:   " << par_time << " ms" << std::endl;
    std::cout << "  Speedup:    " << (seq_time / par_time) << "x" << std::endl;

    CString::verbose = true;
    std::cout << "========== END PARALLEL TEST ==========" << std::endl;
}

int main() {
    int for_test_input;
    std::cout << "Input 2 for autotest (manual test removed)" << std::endl;
    std::cin >> for_test_input;
    if (for_test_input == 2) {
        CString::autotest();
    }
    return 0;
}
