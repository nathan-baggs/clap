#include <contracts>
#include <iostream>
#include <print>
#include <string>

#include <clap/clap.h>

namespace
{

// clang-format off
struct Args
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

}

void handle_contract_violation(std::contracts::contract_violation cv)
{
    std::println(std::cerr, "{}:{} {}", cv.location().file_name(), cv.location().line(), cv.comment());
    std::println(std::cerr, "{}", std::stacktrace::current());
}

auto main(int argc, char **argv) -> int
{
    std::println("{}", ::clap::help<Args>(argc, argv));
    return 0;
}
