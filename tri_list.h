#ifndef TRI_LIST_H
#define TRI_LIST_H

#include <variant>
#include <vector>
#include <tuple>
#include <functional>
#include "tri_list_concepts.h"

template <typename T>
T identity(T x) {return x;}

template <typename T, modifier<T> F, modifier<T> G>
auto compose(F&& a, G&& b) {
    return std::bind(a, std::bind(std::move(b), std::placeholders::_1));
}

template <typename T1, typename T2, typename T3>
class tri_list {
private:
    using value_t = std::variant<T1, T2, T3>;
    std::vector<value_t> values;
    
    using modifier_set = 
        std::tuple<std::function<T1(T1)>, std::function<T2(T2)>, std::function<T3(T3)>>;
    modifier_set modifiers = {identity<T1>, identity<T2>, identity<T3>};

public:
    tri_list() {}
    tri_list(std::initializer_list<std::variant<T1, T2, T3>> _values) : values(_values) {}

    template <typename T>
    void push_back(const T& t) {
        values.push_back(t);
    }

    template <typename T, modifier<T> F>
    void modify_only(F m = F{}) {
        auto& current_modifier = std::get<std::function<T(T)>>(modifiers);
        std::get<std::function<T(T)>>(modifiers) = compose<T>(m, current_modifier);
    }

    template <typename T>
    void reset() {
        std::get<std::function<T(T)>>(modifiers) = identity<T>;
    }

    class iterator {
    private:
        const modifier_set* modifiers;
        std::vector<value_t>::const_iterator it;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::variant<T1, T2, T3>;

        iterator() {}
        iterator(const auto _it, const modifier_set* _modifiers) : it(_it), modifiers(_modifiers) {}

        value_type operator*() const {
            std::variant<T1, T2, T3> val = *it;
            return std::visit([&] <typename T> (T x) {
                auto& modifier = std::get<std::function<T(T)>>(*modifiers);
                return std::variant<T1,T2,T3>(modifier(x));
            }, val);
        }

        iterator& operator++() {
            it++;
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const {
			return it == other.it;
		}
    };

    iterator begin() const {return iterator(values.begin(), &modifiers);}

    iterator end() const {return iterator(values.end(), &modifiers);}

    template <typename T>
    auto range_over() const {
        return std::ranges::subrange(begin(), end()) 
            | std::views::filter([&](value_t x) {return std::holds_alternative<T>(x);})
            | std::views::transform([&](value_t x) {return std::get<T>(x);});
    }
};

#endif /* TRI_LIST_H */
