#include <contracts>
#include <iostream>
#include <optional>
#include <print>
#include <stacktrace>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "clap/clap.h"

void handle_contract_violation(std::contracts::contract_violation cv);
void handle_contract_violation(std::contracts::contract_violation cv)
{
    std::println(std::cerr, "{}:{} {}", cv.location().file_name(), cv.location().line(), cv.comment());
    std::println(std::cerr, "{}", std::stacktrace::current());
}

namespace
{
struct Args
{
    std::string colour;
    auto operator==(const Args &) const -> bool = default;
};

struct Kebab
{
    std::string snake_case;
    std::string camelCase;
    std::string pascalCase;

    auto operator==(const Kebab &) const -> bool = default;
};

struct Default
{
    std::string colour = "red";
    auto operator==(const Default &) const -> bool = default;
};

struct Optional
{
    std::optional<std::string> colour;
    auto operator==(const Optional &) const -> bool = default;
};

struct Int
{
    int number;
    auto operator==(const Int &) const -> bool = default;
};

}

TEST(clap, single_string)
{
    const auto expected = Args{.colour = "red"};
    const auto args = std::vector{"./program", "--colour", "red"};

    ASSERT_EQ(clap::parse<Args>(args.size(), args.data()), expected);
}

TEST(clap, missing_arg)
{
    const auto expected = Args{.colour = "red"};
    const auto args = std::vector{"./program", "red"};

    ASSERT_THAT(
        [&] { clap::parse<Args>(args.size(), args.data()); },
        ::testing::ThrowsMessage<clap::Exception>(::testing::Eq("missing arg: --colour")));
}

TEST(clap, missing_arg_value)
{
    const auto expected = Args{.colour = "red"};
    const auto args = std::vector{"./program", "--colour"};

    ASSERT_THAT(
        [&] { clap::parse<Args>(args.size(), args.data()); },
        ::testing::ThrowsMessage<clap::Exception>(::testing::Eq("missing value for arg: --colour")));
}

TEST(clap, kebab_case)
{
    const auto expected = Kebab{.snake_case = "1", .camelCase = "2", .pascalCase = "3"};
    const auto args = std::vector{"./program", "--snake-case", "1", "--camel-case", "2", "--pascal-case", "3"};

    ASSERT_EQ(clap::parse<Kebab>(args.size(), args.data()), expected);
}

TEST(clap, default_with_arg)
{
    const auto expected = Default{.colour = "blue"};
    const auto args = std::vector{"./program", "--colour", "blue"};

    ASSERT_EQ(clap::parse<Default>(args.size(), args.data()), expected);
}

TEST(clap, default_without_arg)
{
    const auto expected = Default{};
    const auto args = std::vector{"./program"};

    ASSERT_EQ(clap::parse<Default>(args.size(), args.data()), expected);
}

TEST(clap, optional_with_arg)
{
    const auto expected = Optional{.colour = "green"};
    const auto args = std::vector{"./program", "--colour", "green"};

    ASSERT_EQ(clap::parse<Optional>(args.size(), args.data()), expected);
}

TEST(clap, optional_without_arg)
{
    const auto expected = Optional{.colour = std::nullopt};
    const auto args = std::vector{"./program"};

    ASSERT_EQ(clap::parse<Optional>(args.size(), args.data()), expected);
}

TEST(clap, single_int)
{
    const auto expected = Int{.number = 42};
    const auto args = std::vector{"./program", "--number", "42"};

    ASSERT_EQ(clap::parse<Int>(args.size(), args.data()), expected);
}

TEST(clap, single_int_invalid)
{
    const auto args = std::vector{"./program", "--number", "hello"};

    ASSERT_THAT(
        [&] { clap::parse<Int>(args.size(), args.data()); },
        ::testing::ThrowsMessage<clap::Exception>(::testing::Eq("failed to convert 'hello' to integral type")));
}
