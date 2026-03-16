// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021-2026 Harald Sitter <sitter@kde.org>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>

using namespace std::chrono_literals;

namespace
{
// Heart of the cleanup logic. May throw.
bool cleanup(const std::filesystem::directory_entry &path)
{
    const auto lastModified = path.last_write_time();
    const auto now = decltype(lastModified)::clock().now();
    const auto age = now - lastModified;
    // Plenty of time so we won't take away the file from underneath drkonqi.
    if (age >= std::chrono::weeks(1)) {
        std::filesystem::remove_all(path);
    }
    return true;
}

template<typename Precondition>
bool clean(const auto &path, const Precondition &precondition)
try {
    if (!std::filesystem::exists(path)) {
        return true;
    }
    return std::ranges::all_of(std::filesystem::directory_iterator(path), [&](const auto &path) {
        if (!precondition(path)) {
            return true;
        }
        return cleanup(path);
    });
} catch (const std::filesystem::filesystem_error &error) {
    std::cerr << "Failed to clean: " << path << " " << error.what() << "\n";
    return false;
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Need cache path as argument\n";
        return 1;
    }

    const std::filesystem::path cachePath = argv[1];
    if (!std::filesystem::exists(cachePath)) {
        std::cerr << "Cache path doesn't exist " << cachePath << "\n";
        return 1;
    }

    auto ret = 0;
    if (!clean(cachePath / "kcrash-metadata", [](const auto &path) {
            return path.path().extension() == ".ini";
        })) {
        std::cerr << "Failed to clean KCrash metadata\n";
        ret = 1;
    }
    if (!clean(cachePath / "drkonqi/cores", [](const auto &path) {
            return path.is_directory();
        })) {
        std::cerr << "Failed to clean cores\n";
        ret = 1;
    }
    return ret;
}
