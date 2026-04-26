module;

#include "clap.h"

export module clap;

export namespace clap
{
    using clap::Exception;
    using clap::ShortName;
    using clap::IsShortName;
    using clap::Description;
    using clap::IsDescription;
    using clap::Env;
    using clap::IsEnv;

    using clap::help;
    using clap::parse;
}
