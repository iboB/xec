// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <vector>

namespace xec {

template <typename T, template <typename ...> typename Container = std::vector>
class ordered_linear_set : private Container<T> {
public:
    using container = Container<T>;
    using iterator = typename container::iterator;

    container& c() { return *this; }
    const container& c() const { return *this; }

    bool insert(const T& t) {
        for (const auto& e : c()) {
            if (e == t) return false;
        }
        c().push_back(t);
        return true;
    }

    iterator find(const T& t) {
        auto i = begin();
        for (; i != end(); ++i) {
            if (*i == t) break;
        }
        return i;
    }

    using container::empty;

    using container::begin;
    using container::end;

    using container::erase;
    bool erase(const T& t) {
        auto i = find(t);
        if (i == end()) return false;
        erase(i);
        return true;
    }
};

} // namespace xec