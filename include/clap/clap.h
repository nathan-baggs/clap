#pragma once

#include <format>
#include <meta>
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

constexpr auto convert_raw_args(const int argc, char const *const *argv) -> std::vector<std::string_view> //
    pre(argc > 1)                                                                                         //
    post(r : std::ranges::size(r) == argc - 1zu)
{
    return std::span{argv, argv + argc} |                                                 //
           std::views::drop(1) |                                                          //
           std::views::transform([](const auto *arg) { return std::string_view{arg}; }) | //
           std::ranges::to<std::vector>();
}

constexpr auto find_arg(std::span<const std::string_view> args, std::string_view arg) -> std::string_view
{
    const auto arg_iter = std::ranges::find(args, arg);
    if (arg_iter == std::ranges::cend(args))
    {
        throw Exception("missing argument: {}", arg);
    }

    if (arg_iter == std::ranges::cend(args) - 1u)
    {
        throw Exception("missing value for argument: {}", arg);
    }

    return *(arg_iter + 1u);
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

}

template <class T>
    requires std::is_default_constructible_v<T>
constexpr auto parse(int argc, char const *const *argv) -> T //
    pre(argc > 1)
{
    auto args = impl::convert_raw_args(argc, argv);
    constexpr auto ctx = std::meta::access_context::current();

    auto res = T{};

    template for (constexpr auto member : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx)))
    {
        const auto arg_str = impl::format_member_as_arg(std::meta::identifier_of(member));
        const auto arg = impl::find_arg(args, arg_str);
        res.[:member:] = arg;
    }

    return res;
}

}
