# 👏 clap (command line argument parser)

A modern C++26 tool for parsing command line arguments using reflection

**works with gcc trunk**

### Basic usage

```cpp
using std::string;

struct Args
{
    string first_name;
    int age;
    bool active;
};

// ./program --first-name John --age 99 --active

const Args args = clap::parse<Args>(argc, argv);

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
- C++ modules support

```cpp
using std::optional;
using std::string;

using clap::Env;
using clap::ShortName;

struct Args
{
    string host = "localhost";

    [[=ShortName<'p'>{}]]
    uint16_t port;

    [[=Env<"RETRY_COUNT">{}]]
    uint32_t retry_count;

    optional<string> log_file;

    [[=ShortName<'e'>{}]]
    bool encrypted;

    [[=ShortName<'c'>{}]]
    bool compressed;

    [[=ShortName<'h'>{}]]
    bool hashed;
};

// ./program -p 8080 -ec

const Args args = clap::parse<Args>(argc, argv);

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
using std::optional;
using std::string;

using clap::Description;
using clap::ShortName;

struct Args
{
    [[=Description<"first name of user">{}]] 
    string first_name;

    [[=Description<"last name of user">{}]]
    optional<string> last_name;

    [[=ShortName<'i'>{}]]
    [[=Description<"id of user">{}]] 
    int id = -1;

    [[=Description<"if user is active">{}]]
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
using std::string;

struct Args
{
    string snake_case;
    string camelCase;
    string PascalCase;
};

// ./program --snake-case a --camel-case b --pascal-case c
```
