#pragma once

#include <charconv>
#include <concepts>
#include <exception>
#include <format>
#include <iterator>
#include <meta>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace clap
{

class Exception : public std::runtime_error
{
  public:
    template <class... Args>
    constexpr Exception(std::format_string<Args...> msg, Args &&...args)
        : std::runtime_error{std::format(msg, std::forward<Args>(args)...)}
    {
    }
};

namespace impl
{

template <class T>
concept IsOptional = std::same_as<T, std::optional<std::ranges::range_value_t<T>>>;

constexpr auto convert_raw_args(const int argc, char const *const *argv) -> std::vector<std::string_view> //
    pre(argc > 0)                                                                                         //
    post(r : std::ranges::size(r) == argc - 1zu)
{
    return std::span{argv, argv + argc} |                                                 //
           std::views::drop(1) |                                                          //
           std::views::transform([](const auto *arg) { return std::string_view{arg}; }) | //
           std::ranges::to<std::vector>();
}

constexpr auto try_find_arg_index(std::span<const std::string_view> args, std::string_view arg)
    -> std::optional<std::size_t>
{
    const auto arg_iter = std::ranges::find(args, arg);
    return arg_iter == std::ranges::cend(args)
               ? std::nullopt
               : std::make_optional(std::ranges::distance(std::ranges::cbegin(args), arg_iter));
}

constexpr auto format_member_as_arg(std::string_view member_name) -> std::string
{
    auto formatted = std::string{};

    for (auto chr : member_name)
    {
        if (chr == '_')
        {
            formatted += '-';
            continue;
        }

        if (chr >= 'A' && chr <= 'Z')
        {
            formatted += '-';
            formatted += chr + 32;
            continue;
        }

        formatted += chr;
    }

    return std::format("--{}", formatted);
}

template <class T>
    requires std::same_as<T, std::string>
constexpr auto convert_value(std::string_view value) -> T
{
    return T{value};
}

template <class T>
    requires std::integral<T>
constexpr auto convert_value(std::string_view value) -> T
{
    auto res = T{};

    if (const auto ec = std::from_chars(value.data(), value.data() + value.size(), res); !ec)
    {
        throw Exception("failed to convert '{}' to integral type", value);
    }

    return res;
}

template <class T>
    requires IsOptional<T>
constexpr auto convert_value(std::string_view value) -> T::value_type
{
    return convert_value<typename T::value_type>(value);
}

}

template <class T>
    requires std::is_default_constructible_v<T>
constexpr auto parse(int argc, char const *const *argv) -> T //
    pre(argc > 0)
{
    auto args = impl::convert_raw_args(argc, argv);
    constexpr auto ctx = std::meta::access_context::current();

    auto res = T{};

    template for (constexpr auto member : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx)))
    {
        using MemberType = typename[:std::meta::type_of(member):];

        const auto arg_str = impl::format_member_as_arg(std::meta::identifier_of(member));
        if (const auto arg_index = impl::try_find_arg_index(args, arg_str); arg_index)
        {
            if (*arg_index == std::ranges::size(args) - 1zu)
            {
                throw Exception("missing value for arg: {}", arg_str);
            }

            const auto arg_value = args[*arg_index + 1zu];
            res.[:member:] = impl::convert_value<MemberType>(arg_value);
        }
        else
        {
            if constexpr (!std::meta::has_default_member_initializer(member) && !impl::IsOptional<MemberType>)
            {
                throw Exception("missing arg: {}", arg_str);
            }
        }
    }

    return res;
}

}
