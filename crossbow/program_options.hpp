﻿#pragma once
#include "program_options/exceptions.hpp"
#include "program_options/parser.hpp"
#include "program_options/type_printer.hpp"

#include <initializer_list>
#include <type_traits>
#include <utility>
#include <crossbow/string.hpp>

namespace crossbow
{

namespace program_options {

namespace impl
{

enum class o_option_t
{
    ShowOption, IgnoreShort, IgnoreLong, Description
};

}

namespace tag
{

template<bool V>
struct show
{
    constexpr static bool show_option = V;
    constexpr static impl::o_option_t type = impl::o_option_t::ShowOption;
};

template<bool V>
struct ignore_short
{
    constexpr static bool ignore_short_option = V;
    constexpr static impl::o_option_t type = impl::o_option_t::IgnoreShort;
};

template<bool V>
struct ignore_long
{
    constexpr static bool ignore_long_option = V;
    constexpr static impl::o_option_t type = impl::o_option_t::IgnoreLong;
};

struct description
{
    std::string value;
    constexpr static impl::o_option_t type = impl::o_option_t::Description;
};

}

template<class... Opts>
struct o_options;

template<class Head, class... Tail>
struct o_options<Head, Tail...> : o_options<Tail...>
{
    using base = o_options<Tail...>;
    constexpr static bool show_option = std::conditional<Head::type == impl::o_option_t::ShowOption, Head, base>::type::show_option;
    constexpr static bool ignore_short_option = std::conditional<Head::type == impl::o_option_t::IgnoreShort, Head, base>::type::ignore_short_option;
    constexpr static bool ignore_long_option = std::conditional<Head::type == impl::o_option_t::IgnoreLong, Head, base>::type::ignore_long_option;
    
    template<class H>
    typename std::enable_if<H::type == impl::o_option_t::Description, string>::type desc_impl() const
    {
        return this_opt.value;
    }
    
    template<class H>
    typename std::enable_if<H::type != impl::o_option_t::Description, string>::type desc_impl() const
    {
        return base::description();
    }
    
    string description() const
    {
        return desc_impl<Head>();
    }
    
    Head this_opt;
    o_options(Head h, Tail... t) : base(t...), this_opt(h) {}
};

template<>
struct o_options<>
{
    constexpr static bool show_option = true;
    constexpr static bool ignore_short_option = false;
    constexpr static bool ignore_long_option = false;
    
    string description() const {
        return "";
    }
};

template<char Name, class T, class... Opts>
struct option
{
    using my_options = o_options<Opts...>;
    using value_type = T;
    constexpr static bool ignore_short = my_options::ignore_short_option;
    constexpr static bool ignore_long = my_options::ignore_long_option;
    constexpr static char name = Name;
    static_assert(!(ignore_long && ignore_short), "Option will get ignored");
    type_printer<value_type> tprinter;
    string longoption;
    value_type* value_ptr = nullptr;
    value_type value;
    my_options this_opts;
    template<class V>
    void set_value(V&& v) {
        value = std::forward<V>(v);
        if (value_ptr)
            *value_ptr = value;
    }
    option(string longoption, Opts... opts)
        : longoption(longoption), this_opts(opts...) {}
    option(string longoption, value_type* value, Opts... opts)
        : longoption(longoption), value_ptr(value), this_opts(opts...) {}
    option(value_type* value, Opts... opts)
        : value_ptr(value), this_opts(opts...) {}
    option(Opts... opts) : this_opts(opts...) {}

    unsigned print_length() const {
        unsigned res = 0;
        if (!ignore_short) {
            res += 2;
            if (!ignore_long)
                ++res;
        }
        if (!ignore_long)
            res += 2 + longoption.size();
        if (tprinter.length() > 0)
            res += 2 + tprinter.length();
        return res;
    }
    
    std::ostream& print_help(std::ostream& os, unsigned align_with) const {
        if (ignore_short && ignore_long) return os;
        os << " ";
        if (!ignore_short) {
            os << '-' << Name;
            if (!ignore_long)
                os << ',';
        }
        if (!ignore_long)
            os << "--" << longoption;
        if (tprinter.length() > 0) os << " ";
        tprinter(os);
        assert(align_with >= print_length());
        unsigned spaces = align_with - print_length();
        for (unsigned i = 0u; i < spaces + 2; ++i)
            os << " ";
        os << this_opts.description();
        os << std::endl;
        return os;
    }
};

template<char Name, bool F, class... Opts>
struct type_of_impl;

template<char Name, bool F, class Head, class... Tail>
struct type_of_impl<Name, F, Head, Tail...>
{
    constexpr static bool is_this = Name == Head::name;
    constexpr static bool already_found = F || is_this;
    constexpr static bool found = already_found || type_of_impl<Name, already_found, Tail...>::found;
    using type = typename std::conditional<is_this, typename Head::value_type, typename type_of_impl<Name, found, Tail...>::type>::type;
};

template<char Name, bool F, class Head>
struct type_of_impl<Name, F, Head>
{
    constexpr static bool is_this = Name == Head::name;
    constexpr static bool found = F || is_this;
    using type = typename Head::value_type;
};

template<char Name, class... Opts>
struct type_of
{
    using imp = type_of_impl<Name, false, Opts...>;
    constexpr static bool found = imp::found;
    using type = typename imp::type;
    static_assert(found, "Option not found");
};

template<class... Opts>
struct options;

template<class First, class... Rest>
struct options<First, Rest...> : options<Rest...>
{
    using base = options<Rest...>;
    static constexpr char name = First::name;
    using value_type = typename First::value_type;
    constexpr static bool is_unique(char name) {
        return First::name != name && base::is_unique(name);
    }
    static_assert(base::is_unique(First::name), "Argument names must be unique");

    First this_option;
    string global_name;
    bool is_root = false;
	
	options(First&& f, Rest... opts) : base(std::forward<Rest>(opts)...), this_option(std::move(f)) {}
    options(const string& n, First&& f, Rest... opts)
        : base(std::forward<Rest>(opts)...), this_option(std::move(f)),
          global_name(n),
          is_root(true) {}

    template<char Name>
    const typename std::enable_if<Name == name, typename type_of<Name, First, Rest...>::type>::type get() const
    {
        return this->this_option.value;
    }

    template<char Name>
    const typename std::enable_if<Name != name, typename type_of<Name, First, Rest...>::type>::type get() const
    {
        return this->base::template get<Name>();
    }

    template<class T>
    typename std::enable_if<std::is_same<bool, T>::value, bool>::type parse_impl(int& i, const char**) {
        this_option.set_value(true);
        return true;
    }

    template<class T>
    typename std::enable_if<!std::is_same<bool, T>::value, bool>::type parse_impl(int& i, const char** args) {
        parser<T> p;
        this_option.set_value(p(args[++i]));
        return true;
    }
    
    bool parse_long(int &i, const char** args) {
        const char* arg = args[i];
        while (*arg == '-') ++arg;
        if (First::ignore_long || arg != this_option.longoption) {
            return base::parse_long(i, args);
        } else {
            return parse_impl<value_type>(i, args);
        }
    }

    bool parse_short(char n, int& i, const char** argv) {
        if (name == n && !First::ignore_short) {
            return parse_impl<value_type>(i, argv);
        } else {
            return base::parse_short(n, i, argv);
        }
    }
    
    unsigned max_print_lenght() const {
        return std::max(this_option.print_length(), base::max_print_lenght());
    }
    
    std::ostream& print_help(std::ostream& out) const {
        if (is_root) {
            out << "Usage: " << global_name << " [OPTION...]" << std::endl;
        }
        this_option.print_help(out, max_print_lenght());
        return base::print_help(out);
    }
};

template<>
struct options<>
{
    bool parse_short(char n, int& i, const char** argv)
    {
        char err[3];
        err[0] = '-';
        err[1] = n;
        err[2] = '\0';
        throw argument_not_found(err);
    }

    bool parse_long(int& i, const char** args)
    {
        string err = "--";
        err += args[i];
        throw argument_not_found(err.c_str());
    }

    constexpr static bool is_unique(char) {
        return true;
    }
    
    unsigned max_print_lenght() const {
        return 0u;
    }
    
    std::ostream& print_help(std::ostream& out) const {
        return out;
    }
};

template<char Name, template<class...> class Opts, class... Opt>
const typename type_of<Name, Opt...>::type& get(Opts<Opt...>& opts) {
}

template<class... Opts>
options<Opts...> create_options(const string& name, Opts&&... opts)
{
    using res_type = options<Opts...>;
    return res_type{name, std::forward<Opts>(opts)...};
}

template<char Name, class T, class... Opts>
option<Name, T, Opts...> value(const string& longopt, T& val, Opts... opts) {
    return option<Name, T, Opts...>{longopt, &val, opts...};
}

template<char Name, class T, class... Opts>
option<Name, T, Opts...> value(const char* longopt, T& val, Opts... opts) {
    return value<Name, T, Opts...>(string{longopt}, val, opts...);
}

template<char Name, class T, class... Opts>
option<Name, T, tag::ignore_long<true>, Opts...> value(T& val, Opts... opts) {
    return option<Name, T, tag::ignore_long<true>, Opts...>{&val, tag::ignore_long<true>{}, opts...};
}

template<char Name, class... Opts>
option<Name, bool, tag::ignore_long<true>, Opts...> toggle(Opts... opts) {
    return option<Name, bool, tag::ignore_long<true>, Opts...>{tag::ignore_long<true>{}, opts...};
}

template<char Name, class... Opts>
option<Name, bool, tag::ignore_long<true>, Opts...> toggle(bool& value, Opts... opts)
{
    return option<Name, bool, tag::ignore_long<true>, Opts...>{&value};
}

template<char Name, class... Opts>
option<Name, bool, Opts...> toggle(const string& longopt, Opts... opts)
{
    return option<Name, bool, Opts...>{longopt, opts...};
}

template<char Name, class... Opts>
option<Name, bool, Opts...> toggle(const char* longopt, Opts... opts)
{
    return toggle<Name, Opts...>(string{longopt}, opts...);
}

template<char Name, class... Opts>
option<Name, bool, Opts...> toggle(const string& longopt, bool& value, Opts... opts)
{
    return option<Name, bool, Opts...>{longopt, &value, opts...};
}

template<char Name, class... Opts>
option<Name, bool, Opts...> toggle(const char* longopt, bool& value, Opts... opts)
{
    return toggle<Name, Opts...>(string{longopt}, value, opts...);
}

template<class O>
std::ostream& print_help(std::ostream& s, O& opts)
{
    return opts.print_help(s);
}

template<class O>
int parse(O& opts, int argc, const char** argv) {
    int i;
    for (i = 1; i < argc; ++i) {
        const char* str = argv[i];
        if (str[0] == '-' && str[1] == '-' && (std::isspace(str[2]) || str[2] == '\0')) {
            return i + 1;
        } else if (str[0] == '-' && str[1] == '-') {
            // parse a long option
            opts.parse_long(i, argv);
        } else if (str[0] == '-') {
            if (str[2] != '\0')
                throw parse_error("Long options must start with '--'");
            // parse short option
            opts.parse_short(str[1], i, argv);
        } else {
            // either an error or remaining options
            for (int j = i; j < argc; ++j) {
                if (argv[j][0] == '-' && argv[j][1] == '-' && argv[j][2] == '\0')
                    return i;
                if (argv[j][0] == '-')
                    throw inexpected_value(str);
            }
            return i;
        }
    }
    return i;
}

}// namespace program_options
} // namespace crossbow