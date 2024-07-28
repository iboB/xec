// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <queue>
#include <vector>
#include <optional>
#include "chrono.hpp"

namespace xec {

template <typename T>
struct TimedElementLater {
    bool operator()(const T& a, const T& b) const {
        return a.time > b.time;
    }
};

template <typename T>
struct TimedQueue
    : public std::priority_queue<T, std::vector<T>, TimedElementLater<T>>
{
    template <typename F>
    std::optional<T> tryExtract(F&& f) {
        auto& c = this->c;
        auto& comp = this->comp;
        for (auto i = c.begin(); i != c.end(); ++i) {
            if (f(*i)) {
                auto extracted = std::move(*i);
                c.erase(i);
                std::make_heap(c.begin(), c.end(), comp);
                return extracted;
            }
        }
        return {};
    }

    template <typename F>
    bool tryReschedule(clock_t::time_point newTime, F&& f) {
        auto& c = this->c;
        auto& comp = this->comp;
        for (auto i = c.begin(); i != c.end(); ++i) {
            if (f(*i)) {
                i->time = newTime;
                std::make_heap(c.begin(), c.end(), comp);
                return true;
            }
        }
        return false;
    }

    template <typename F>
    size_t eraseAll(F&& f) {
        auto& c = this->c;
        auto& comp = this->comp;
        auto newEnd = std::remove_if(c.begin(), c.end(), std::forward<F>(f));
        auto size = c.end() - newEnd;
        if (size) {
            c.erase(newEnd, c.end());
            std::make_heap(c.begin(), c.end(), comp);
        }
        return size;
    }

    T topAndPop() {
        auto& c = this->c;
        auto& comp = this->comp;
        std::pop_heap(c.begin(), c.end(), comp);
        auto ret = std::move(c.back());
        c.pop_back();
        return ret;
    }

    void clear() { this->c.clear(); }
};

} // namespace xec
