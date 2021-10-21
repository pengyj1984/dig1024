#include <vector>
#include <thread>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <cassert>

// 得到当前时间点的 epoch 秒
inline double NowEpochSeconds() noexcept {
    return (double) std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() / 1000000.0;
}

template<class F>
auto MakeScopeGuard(F &&f) noexcept {
    struct ScopeGuard {
        F f;
        bool cancel;

        explicit ScopeGuard(F &&f) noexcept: f(std::move(f)), cancel(false) {}

        ~ScopeGuard() noexcept { if (!cancel) { f(); }}

        inline void Cancel() noexcept { cancel = true; }

        inline void operator()(bool cancel = false) {
            f();
            this->cancel = cancel;
        }
    };
    return ScopeGuard(std::forward<F>(f));
}

inline int ReadAllBytes(std::filesystem::path const& path, std::unique_ptr<uint8_t[]>& outBuf, size_t& outSize) noexcept {
    outBuf.reset();
    outSize = 0;
    std::ifstream f(path, std::ifstream::binary);
    if (!f) return -1;													// not found? no permission? locked?
    auto sg = MakeScopeGuard([&] { f.close(); });
    f.seekg(0, f.end);
    auto&& siz = f.tellg();
    if ((uint64_t)siz > std::numeric_limits<size_t>::max()) return -2;	// too big
    f.seekg(0, f.beg);
    auto&& buf = new(std::nothrow) uint8_t[siz];
    if (!buf) return -3;												// not enough memory
    outBuf = std::unique_ptr<uint8_t[]>(buf);
    f.read((char*)buf, siz);
    if (!f) return -3;													// only f.gcount() could be read
    outSize = siz;
    return 0;
}

#define NUM_THREADS 4

int main() {
    std::cout << std::filesystem::current_path() << std::endl;
    std::vector<std::filesystem::path> files;
    for (auto&& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path())) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".data") continue;
        files.push_back(entry.path());
    }

    auto start = NowEpochSeconds();
    {
        std::array<std::thread, NUM_THREADS> ts;
        for (int i = 0; i < NUM_THREADS; ++i) {
            ts[i] = std::thread([&, i = i]{
                std::unique_ptr<uint8_t[]> buf;
                size_t siz = 0;
                for (int j = i; j < (int)files.size(); j += NUM_THREADS) {
                    auto& f = files[j];
                    int r = ReadAllBytes(f, buf, siz);
                    assert(r);
                }
                std::cout << "thread " << i << ": siz " << siz << std::endl;
            });
        }
        for(auto& t : ts) t.join();
    }
    std::cout << "secs = " << (NowEpochSeconds() - start) << std::endl;
    return 0;
}