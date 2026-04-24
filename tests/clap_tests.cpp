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

struct AutoEnv
{
    AutoEnv(std::string env, const std::string &value)
        : env{std::move(env)}
    {
#if defined(_WIN32)
        ::_putenv_s(this->env.c_str(), value.c_str());
#else
        ::setenv(env.c_str(), value.c_str(), 1);
#endif
    }

    ~AutoEnv()
    {
#if defined(_WIN32)
        ::_putenv_s(env.c_str(), "");
#else
        ::unsetenv(env.c_str());
#endif
    }

    std::string env;
};

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

struct Bool
{
    bool enabled;
    auto operator==(const Bool &) const -> bool = default;
};

struct ShortName
{
    [[= clap::ShortName<'n'>{}]] int number;
    auto operator==(const ShortName &) const -> bool = default;
};

// clang-format off
struct Description
{
    [[= clap::Description<"first name of user">{}]] 
    std::string first_name;

    [[= clap::Description<"last name of user">{}]]
    std::optional<std::string> last_name;

    [[= clap::ShortName<'i'>{}]]
    [[= clap::Description<"id of user">{}]] 
    int id = -1;

    [[= clap::Description<"if user is active">{}]]
    bool active;
};
// clang-format on

struct Env
{
    [[= clap::Env<"CLAP_TEST_PORT">{}]] //
        std::uint16_t port;

    auto operator==(const Env &) const -> bool = default;
};

struct EnvWithDefault
{
    [[= clap::Env<"CLAP_TEST_PORT">{}]] //
        std::uint16_t port = 8000;

    auto operator==(const EnvWithDefault &) const -> bool = default;
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

TEST(clap, single_bool_present)
{
    const auto expected = Bool{.enabled = true};
    const auto args = std::vector{"./program", "--enabled"};

    ASSERT_EQ(clap::parse<Bool>(args.size(), args.data()), expected);
}

TEST(clap, single_bool_missing)
{
    const auto expected = Bool{.enabled = false};
    const auto args = std::vector{"./program"};

    ASSERT_EQ(clap::parse<Bool>(args.size(), args.data()), expected);
}

TEST(clap, short_name)
{
    const auto expected = ShortName{.number = 42};
    const auto args = std::vector{"./program", "-n", "42"};

    ASSERT_EQ(clap::parse<ShortName>(args.size(), args.data()), expected);
}

TEST(clap, short_name_missing)
{
    const auto args = std::vector{"./program"};

    ASSERT_THAT(
        [&] { clap::parse<ShortName>(args.size(), args.data()); },
        ::testing::ThrowsMessage<clap::Exception>(::testing::Eq("missing arg: -n")));
}

TEST(clap, short_name_and_long_name)
{
    const auto args = std::vector{"./program", "-n", "42", "--number", "42"};

    ASSERT_THAT(
        [&] { clap::parse<ShortName>(args.size(), args.data()); },
        ::testing::ThrowsMessage<clap::Exception>(::testing::Eq("cannot have both -n --number in args")));
}

TEST(clap, description)
{
    const auto expected = R"(./program

  --first-name
      first name of user
      [type: string]

  --last-name
      last name of user
      [type: string]
      [optional]

  -i, --id
      id of user
      [type: number]
      [default: -1]

  --active
      if user is active
      [type: bool])";
    const auto args = std::vector{"./program"};

    ASSERT_EQ(clap::help<Description>(args.size(), args.data()), expected);
}

TEST(clap, env_default)
{
    const auto auto_env = AutoEnv{"CLAP_TEST_PORT", "1234"};

    const auto expected = Env{.port = 1234};
    const auto args = std::vector{"./program"};

    ASSERT_EQ(clap::parse<Env>(args.size(), args.data()), expected);
}

TEST(clap, env_value_override)
{
    const auto auto_env = AutoEnv{"CLAP_TEST_PORT", "1234"};

    const auto expected = Env{.port = 4321};
    const auto args = std::vector{"./program", "--port", "4321"};

    ASSERT_EQ(clap::parse<Env>(args.size(), args.data()), expected);
}

TEST(clap, env_value_error_missing_env)
{
    const auto args = std::vector{"./program"};

    ASSERT_THAT(
        [&] { clap::parse<Env>(args.size(), args.data()); },
        ::testing::ThrowsMessage<clap::Exception>(
            ::testing::Eq("missing arg: --port (and not in env CLAP_TEST_PORT)")));
}

TEST(clap, env_override_default)
{
    const auto auto_env = AutoEnv{"CLAP_TEST_PORT", "1234"};

    const auto expected = Env{.port = 1234};
    const auto args = std::vector{"./program"};

    ASSERT_EQ(clap::parse<Env>(args.size(), args.data()), expected);
}
