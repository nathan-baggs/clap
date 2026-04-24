# 👏 clap (command line argument parser)

A modern C++26 tool for parsing command line arguments using reflection

### Basic usage

```cpp
struct Args
{
    std::string first_name;
    int age;
    bool active;
};

// ./program --first-name John --age 99 --active

const auto args = clap::parse<Args>(argc, argv);

assert(args.first_name == "John");
assert(args.age == 99);
assert(args.active);
```

# Features

- Default values
- Optional values
- Short arguments
- Env var backing
- Combined short name (flags only)

```cpp
struct Args
{
    std::string host = "localhost";

    [[=clap::ShortName<'p'>{}]]
    std::uint16_t port;

    [[=clap::Env<"RETRY_COUNT">{}]]
    std::uint32_t retry_count;

    std::optional<std::string> log_file;

    [[=clap::ShortName<'e'>{}]]
    bool encrypted;

    [[=clap::ShortName<'c'>{}]]
    bool compressed;

    [[=clap::ShortName<'h'>{}]]
    bool hashed;
};

// ./program -p 8080 -ec

const auto args = clap::parse<Args>(argc, argv);

assert(args.host == "localhost");
assert(args.port == 8080);
assert(args.retry_count == std::stoul(std::getenv("RETRY_COUNT")));
assert(!args.log_file);
assert(args.encrypted);
assert(args.compressed);
assert(!args.hashed);
```

# Descriptions

Also supported are annotations for adding descriptions to fields, these can then be converted to a help message

```cpp
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

std::println("{}", clap::help<Args>(argc, argv));

//  ./program
//  
//    --first-name
//        first name of user
//        [type: string]
//  
//    --last-name
//        last name of user
//        [type: string]
//        [optional]
//  
//    -i, --id
//        id of user
//        [type: number]
//        [default: -1]
//  
//    --active
//        if user is active
//        [type: bool]
```

# Casing

clap will heuristically try and kebaberise all members

```cpp
struct Args
{
    std::string snake_case;
    std::string camelCase;
    std::string PascalCase;
};

// ./program --snake-case a --camel-case b --pascal-case c
```
