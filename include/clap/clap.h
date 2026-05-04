#pragma once

#include <algorithm>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <format>
#include <iterator>
#include <memoryapi.h>
#include <meta>
#include <optional>
#include <ranges>
#include <span>
#include <sstream>
#include <stacktrace>
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
    pre(argv != nullptr)                                                                                  //
    post(r : std::ranges::size(r) == argc - 1zu)
{
    return std::span{argv, argv + argc} |                                                 //
           std::views::drop(1) |                                                          //
           std::views::transform([](const auto *arg) { return std::string_view{arg}; }) | //
           std::ranges::to<std::vector>();
}

constexpr auto try_find_arg_index(const std::span<const std::string_view> args, const std::string_view arg)
    -> std::optional<std::size_t> //
    pre(!std::ranges::empty(arg))
{
    const auto arg_iter = std::ranges::find(args, arg);
    return arg_iter == std::ranges::cend(args)
               ? std::nullopt
               : std::make_optional(std::ranges::distance(std::ranges::cbegin(args), arg_iter));
}

constexpr auto try_find_short_arg_index(const std::span<const std::string_view> args, const std::string_view arg)
    -> std::optional<std::size_t>    //
    pre(std::ranges::size(arg) == 2) //
    pre(arg[0] == '-')
{
    constexpr auto is_short_arg = [](const auto a) { return std::ranges::size(a) >= 2 && a[0] == '-' && a[1] != '-'; };

    for (const auto &[index, short_arg] : std::views::enumerate(args))
    {
        if (is_short_arg(short_arg) && std::ranges::contains(short_arg, arg[1]))
        {
            return index;
        }
    }

    return std::nullopt;
}

constexpr auto format_member_as_arg(const std::string_view member_name) -> std::string //
    pre(!std::ranges::empty(member_name))
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
            if (!std::ranges::empty(formatted))
            {
                formatted += '-';
            }
            formatted += chr + 32;
            continue;
        }

        formatted += chr;
    }

    return std::format("--{}", formatted);
}

consteval auto format_short_name(std::meta::info) -> std::string_view
{
    return {};
}

template <class T>
    requires std::same_as<T, std::string>
constexpr auto convert_value(const std::string_view value) -> T //
    pre(!std::ranges::empty(value))                             //
    post(r : std::ranges::size(value) == std::ranges::size(r))
{
    return T{value};
}

template <class T>
    requires(std::integral<T> && !std::same_as<T, bool>)
constexpr auto convert_value(const std::string_view value) -> T //
    pre(!std::ranges::empty(value))
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
constexpr auto convert_value(const std::string_view value) -> T::value_type //
    pre(!std::ranges::empty(value))
{
    return convert_value<typename T::value_type>(value);
}

template <std::size_t N>
struct TemplatedString
{
    consteval TemplatedString(const char (&s)[N])
    {
        std::ranges::copy(s, str);
    }

    char str[N];
};

template <class T>
    requires std::same_as<T, std::string>
constexpr auto friendly_type_name() -> std::string_view
{
    return "string";
}

template <class T>
    requires(std::integral<T> && !std::same_as<T, bool>)
constexpr auto friendly_type_name() -> std::string_view
{
    return "number";
}

template <class T>
    requires std::same_as<T, bool>
constexpr auto friendly_type_name() -> std::string_view
{
    return "bool";
}

template <class T>
    requires IsOptional<T>
constexpr auto friendly_type_name() -> std::string_view
{
    return friendly_type_name<std::ranges::range_value_t<T>>();
}

template <impl::TemplatedString S>
struct TextAnnotation
{
    constexpr static auto templated_string()
    {
        return S;
    }

    constexpr static auto str() -> std::string_view
    {
        return {S.str};
    }
};

template <class T>
struct DebugType;

}

template <char S>
struct ShortName
{
    constexpr static auto letter = S;
};

template <class T>
concept IsShortName = std::same_as<std::remove_cvref_t<T>, ShortName<T::letter>>;

template <impl::TemplatedString S>
struct Description : impl::TextAnnotation<S>
{
};

template <class T>
concept IsDescription = std::same_as<std::remove_cvref_t<T>, Description<T::templated_string()>>;

template <impl::TemplatedString S>
struct Env : impl::TextAnnotation<S>
{
};

template <class T>
concept IsEnv = std::same_as<std::remove_cvref_t<T>, Env<T::templated_string()>>;

template <class T>
    requires std::is_default_constructible_v<T>
constexpr auto help(int argc, char const *const *argv) -> std::string //
    pre(argc > 0)
{
    auto strm = std::stringstream{};

    strm << argv[0] << "\n\n";

    auto required = std::vector<std::string>{};
    auto optional = std::vector<std::string>{};
    auto flags = std::vector<std::string>{};

    constexpr auto ctx = std::meta::access_context::current();

    template for (constexpr auto member : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx)))
    {
        using MemberType = typename[:std::meta::type_of(member):];

        auto arg_str_short = std::optional<std::string>{};
        auto description = std::optional<std::string_view>{};

        template for (constexpr auto annotation : std::define_static_array(std::meta::annotations_of(member)))
        {
            using AnnotationType = typename[:std::meta::type_of(annotation):];

            if constexpr (IsShortName<AnnotationType>)
            {
                constexpr auto letter = AnnotationType::letter;

                if (!!arg_str_short)
                {
                    throw Exception("cannot have multiple ShortName annotations");
                }

                arg_str_short = std::format("-{}", letter);
            }

            if constexpr (IsDescription<AnnotationType>)
            {
                description = AnnotationType::str();
            }
        }

        const auto arg_str_long = impl::format_member_as_arg(std::meta::identifier_of(member));

        strm
            << (arg_str_short ? std::format("  {}, {}", *arg_str_short, arg_str_long)
                              : std::format("  {}", arg_str_long));
        strm << std::format("\n      {}", description.value_or(""));
        strm << std::format("\n      [type: {}]", impl::friendly_type_name<MemberType>());

        if constexpr (impl::IsOptional<MemberType>)
        {
            strm << "\n      [optional]";
        }

        if constexpr (std::meta::has_default_member_initializer(member))
        {
            strm << std::format("\n      [default: {}]", T{}.[:member:]);
        }

        strm << "\n\n";
    }

    const auto help_message = strm.str();
    return help_message.substr(0zu, std::ranges::size(help_message) - 2zu);
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

        auto arg_str_short = std::optional<std::string>{};
        auto env_name = std::optional<std::string>{};

        // parse all the annotations and pull out anything useful
        template for (constexpr auto annotation : std::define_static_array(std::meta::annotations_of(member)))
        {
            using AnnotationType = typename[:std::meta::type_of(annotation):];

            if constexpr (IsShortName<AnnotationType>)
            {
                constexpr auto letter = AnnotationType::letter;

                if (!!arg_str_short)
                {
                    throw Exception("cannot have multiple ShortName annotations");
                }

                arg_str_short = std::format("-{}", letter);
            }
            else if constexpr (IsEnv<AnnotationType>)
            {
                env_name = std::string{AnnotationType::str()};
            }
        }

        // chuff for handling short and long args

        const auto arg_str_short_index = arg_str_short.and_then(
            [&](const auto &short_arg) { return impl::try_find_short_arg_index(args, short_arg); });

        const auto arg_str_long = impl::format_member_as_arg(std::meta::identifier_of(member));
        const auto arg_str_long_index = impl::try_find_arg_index(args, arg_str_long);

        if (!!arg_str_short_index && !!arg_str_long_index)
        {
            throw Exception("cannot have both {} {} in args", args[*arg_str_short_index], args[*arg_str_long_index]);
        }

        const auto arg_str = arg_str_short_index ? std::string{args[*arg_str_short_index]} : arg_str_long;

        if constexpr (std::same_as<MemberType, bool>)
        {
            // bool is a special case as it just represents a flag, it is inherently defaulted to false
            // also some extra bookkeeping to handle the compressed short flags

            res.[:member:] = (arg_str_short && !!impl::try_find_short_arg_index(args, *arg_str_short)) ||
                             !!impl::try_find_arg_index(args, arg_str);
        }
        else
        {
            const auto arg_index = impl::try_find_arg_index(args, arg_str);

            const auto is_in_args = !!arg_index;
            const auto is_backed_by_env = !!env_name;
            constexpr auto is_defaulted = std::meta::has_default_member_initializer(member);
            constexpr auto is_optional = impl::IsOptional<MemberType>;

            if (is_defaulted && is_optional)
            {
                throw Exception("cannot have defaulted and optional arg: {}", arg_str);
            }
            else if (is_in_args)
            {
                // providing an arg should have the highest precedence, so try and parse it

                if (*arg_index == std::ranges::size(args) - 1zu)
                {
                    throw Exception("missing value for arg: {}", arg_str);
                }

                const auto arg_value = args[*arg_index + 1zu];
                res.[:member:] = impl::convert_value<MemberType>(arg_value);
            }
            else if (is_backed_by_env)
            {
                // if there was no arg but it can be backed by and env var then try and find it

                if (const auto *env_value = std::getenv(env_name->c_str()); env_value)
                {
                    res.[:member:] = impl::convert_value<MemberType>(env_value);
                }
                else
                {
                    throw Exception("missing arg: {} (and not in env {})", arg_str, *env_name);
                }
            }
            else if (is_defaulted || is_optional)
            {
                // if the value is defaulted or optional then it has already been handled by the default constructed
                // res object, so just go to the next member

                continue;
            }
            else
            {
                throw Exception("missing arg: {}", arg_str);
            }
        }
    }

    return res;
}
}
