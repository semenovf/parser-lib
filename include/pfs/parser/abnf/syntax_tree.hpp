////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.13 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../line_counter_iterator.hpp"
#include "parser.hpp"
#include "node.hpp"
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <cassert>

namespace pfs {
namespace parser {
namespace abnf {

#if PFS_SYNTAX_TREE_TRACE_ENABLE
#   define PFS_SYNTAX_TREE_TRACE(x) x
#else
#   define PFS_SYNTAX_TREE_TRACE(x)
#endif

// NOTE Below #if statement is borrowed from 'abseil-cpp' project.
// Gcc 4.8 has __cplusplus at 201301 but the libstdc++ shipped with it doesn't
// define make_unique.  Other supported compilers either just define __cplusplus
// as 201103 but have make_unique (msvc), or have make_unique whenever
// __cplusplus > 201103 (clang).
#if (__cplusplus > 201103L || defined(_MSC_VER)) && \
    !(defined(__GLIBCXX__) && !defined(__cpp_lib_make_unique))

using std::make_unique;

#else

// Naive implementation of pre-C++14 std::make_unique
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
    static_assert(!std::is_array<T>::value, "arrays not supported");
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif

////////////////////////////////////////////////////////////////////////////////
// parse_result
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
struct parse_result
{
    std::error_code ec;
    int lineno = 0;
    StringType what;
    std::unique_ptr<rulelist_node<StringType>> root;
};

////////////////////////////////////////////////////////////////////////////////
// syntax_tree_context
////////////////////////////////////////////////////////////////////////////////
template <typename StringType, typename ForwardIterator>
class syntax_tree_context
{
public:
    using string_type = StringType;
    using forward_iterator = line_counter_iterator<ForwardIterator>;

private:
    using prose_node_type = prose_node<string_type>;
    using number_node_type = number_node<string_type>;
    using quoted_string_node_type = quoted_string_node<string_type>;
    using rulename_node_type = rulename_node<string_type>;
    using repetition_node_type = repetition_node;
    using group_node_type = group_node;
    using option_node_type = option_node;
    using concatenation_node_type = concatenation_node;
    using alternation_node_type = alternation_node;
    using rule_node_type = rule_node<string_type>;
    using rulelist_node_type = rulelist_node<string_type>;
    using aggregate_node_type = aggregate_node;

private:
    size_t _max_quoted_string_length = 0;
    rulelist_node_type * _rulelist {nullptr};
    std::stack<std::unique_ptr<basic_node>> _stack;

    parse_result<string_type> _parse_result;

#if PFS_SYNTAX_TREE_TRACE_ENABLE
    int _indent_level = 0;
    int _indent_step = 4;

    string_type indent ()
    {
        //return string_type{1, '|'} + string_type(_indent_level * _indent_step, '_');

        string_type result {1, '|'};
        auto i = _indent_level;

        while (i--) {
            result += string_type(_indent_step, '-');

            if (i > 0)
                result += string_type(1, '|');
        }

        return result;
    }
#endif

private: // Helper methods
    inline aggregate_node_type * check_aggregate_node ()
    {
        assert(!_stack.empty());
        auto & node = _stack.top();
        assert(node->is_aggregate_node());
        return static_cast<aggregate_node_type *>(& *node);
    }

    inline number_node_type * check_number_node ()
    {
        assert(!_stack.empty());
        auto & node = _stack.top();
        assert(node->type() == node_enum::number);
        return static_cast<number_node_type *>(& *node);
    }

    inline repetition_node_type * check_repetition_node ()
    {
        assert(!_stack.empty());
        auto & node = _stack.top();
        assert(node->type() == node_enum::repetition);
        return static_cast<repetition_node_type *>(& *node);
    }

    inline rulelist_node_type * check_rulelist_node ()
    {
        assert(!_stack.empty());
        auto & node = _stack.top();
        assert(node->type() == node_enum::rulelist);
        return static_cast<rulelist_node_type *>(& *node);
    }

    inline basic_node * end_aggregate_component (bool success)
    {
        assert(!_stack.empty());

        basic_node * result {nullptr};
        auto item = std::move(_stack.top());
        _stack.pop();

        if (success) {
            auto cn = check_aggregate_node();
            result = & *item;
            cn->push_back(std::move(item));
        }

        return result;
    }

public:
    syntax_tree_context (size_t max_quoted_string_length = 0)
        : _max_quoted_string_length(max_quoted_string_length)
    {}

    syntax_tree_context (syntax_tree_context && other) = default;
    syntax_tree_context & operator = (syntax_tree_context && other) = default;

    parse_result<string_type> && result ()
    {
        return std::move(_parse_result);
    }

public: // Parse context requiremenets
    // xLCOV_EXCL_START
    void error (std::error_code ec, forward_iterator near_pos)
    {
        _parse_result.ec = ec;
        _parse_result.lineno = near_pos.lineno();
    }

    void syntax_error (std::error_code ec
        , forward_iterator near_pos
        , string_type const & what)
    {
        _parse_result.ec = ec;
        _parse_result.lineno = near_pos.lineno();
        _parse_result.what = what;
    }
    // xLCOV_EXCL_STOP

    size_t max_quoted_string_length ()
    {
        return _max_quoted_string_length;
    }

    bool begin_document ()
    {
        auto rulelist = make_unique<rulelist_node_type>();
        _rulelist = & *rulelist;
        _stack.push(std::move(rulelist));
        return true;
    }

    bool end_document (bool success)
    {
        assert(_stack.size() == 1);
        assert(_stack.top()->type() == node_enum::rulelist);

        // Static unique pointer cast
        _parse_result.root = std::unique_ptr<rulelist_node_type>(
            static_cast<rulelist_node_type*>(_stack.top().release()));

        _stack.pop();
        return success;
    }

    // ProseContext
    bool prose (forward_iterator first, forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "prose: \"" << value << "\"\n"
        ));

        auto cn = check_repetition_node();
        auto prose = make_unique<prose_node_type>(std::move(value));
        cn->set_element(std::move(prose));

        return true;
    }

    // NumberContext
    bool first_number (number_flag flag
        , forward_iterator first
        , forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "first_number: " <<  value << "\n"
        ));

        auto num = make_unique<number_node_type>(flag);
        num->set_first(std::move(value));
        _stack.push(std::move(num));

        return true;
    }

    bool last_number (number_flag
        , forward_iterator first
        , forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "last_number: " << value << "\n"
        ));

        auto cn = check_number_node();

        // Inequality of iterators is a flag of a range
        // so this is the last number in a range
        if (first != last) {
            cn->set_last(std::move(value));
        } else {
            ;
        }

        auto num = std::move(_stack.top());
        _stack.pop();

        auto rep = check_repetition_node();
        rep->set_element(std::move(num));

        return true;
    }

    bool next_number (number_flag
        , forward_iterator first
        , forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "next_number: " << value << "\n"
        ));

        auto cn = check_number_node();
        cn->push_next(std::move(value));

        return true;
    }

    // QuotedStringContext
    bool quoted_string (forward_iterator first, forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "quoted_string: \"" << value << "\"\n"
        ));

        auto cn = check_repetition_node();
        auto qs = make_unique<quoted_string_node_type>(std::move(value));
        cn->set_element(std::move(qs));

        return true;
    }

    // GroupContext
    bool begin_group ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN group\n"
        ));

        auto gr = make_unique<group_node_type>();
        _stack.push(std::move(gr));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    }

    bool end_group (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        assert(!_stack.empty());

        auto gr = std::move(_stack.top());
        _stack.pop();

        if (success) {
            auto cn = check_repetition_node();
            cn->set_element(std::move(gr));
        }

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END group: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return success;
    }

    // OptionContext
    bool begin_option ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN option\n"
        ));

        auto opt = make_unique<option_node_type>();
        _stack.push(std::move(opt));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    }

    bool end_option (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        assert(!_stack.empty());

        auto opt = std::move(_stack.top());
        _stack.pop();

        if (success) {
            auto cn = check_repetition_node();
            cn->set_element(std::move(opt));
        }

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END option: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return success;
    }

    // RepeatContext
    bool repeat (long from, long to)
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "repeat: " << from << '-' << to << "\n"
        ));

        auto cn = check_repetition_node();
        cn->set_range(from, to);

        return true;
    }

    // RulenameContext
    bool rulename (forward_iterator first, forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "rulename: \"" << value << "\"\n"
        ));

        auto cn = check_repetition_node();
        auto rn = make_unique<rulename_node_type>(std::move(value));
        cn->set_element(std::move(rn));

        return true;
    }

    // RepetitionContext
    bool begin_repetition ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN repetition\n"
        ));

        auto rep = make_unique<repetition_node_type>();
        _stack.push(std::move(rep));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    }

    bool end_repetition (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        end_aggregate_component(success);

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END repetition: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return success;
    }

    // AlternationContext
    bool begin_alternation ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN alternation\n"
        ));

        auto alt = make_unique<alternation_node_type>();
        _stack.push(std::move(alt));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    }

    bool end_alternation (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        end_aggregate_component(success);

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END alternation: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return success;
    }

    // ConcatenationContext
    bool begin_concatenation ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN concatenation\n"
        ));

        auto cat = make_unique<concatenation_node_type>();
        _stack.push(std::move(cat));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    }

    bool end_concatenation (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        end_aggregate_component(success);

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END concatenation: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return success;
    }

    bool begin_rule (forward_iterator first, forward_iterator last
        , bool is_incremental_alternatives)
    {
        auto value = string_type(first.base(), last.base());

        auto cn = check_rulelist_node();
        auto extracted = cn->extract(value);

        if (is_incremental_alternatives) {
            PFS_SYNTAX_TREE_TRACE((
                std::cout << indent() << "BEGIN incremental alternative rule: " << value << "\n"
            ));

            // Error: rule not found
            if (extracted.first == false) {
                syntax_error(make_error_code(errc::rule_undefined), first, value);
                return false;
            }

            _stack.push(std::move(extracted.second));
        } else {
            PFS_SYNTAX_TREE_TRACE((
                std::cout << indent() << "BEGIN basic rule definition: " << value << "\n"
            ));

            // Error: rule already exists
            if (extracted.first == true) {
                syntax_error(make_error_code(errc::rulename_duplicated), first, value);
                return false;
            }

            auto rule = make_unique<rule_node_type>();
            rule->set_name(std::move(value));
            _stack.push(std::move(rule));
        }

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    };

    bool end_rule (forward_iterator first, forward_iterator last
        , bool is_incremental_alternatives
        , bool success)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        assert(!_stack.empty());

        auto node = std::move(_stack.top());
        _stack.pop();

        assert(node->type() == node_enum::rule);

        if (success) {
            auto cn = check_rulelist_node();
            cn->emplace(std::move(value), std::move(node));
        }

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END rule: " << value
                << ": " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return success;
    }
};

////////////////////////////////////////////////////////////////////////////////
// syntax_tree
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class syntax_tree final
{
    using string_type = StringType;
    using rulelist_node_type = rulelist_node<string_type>;

    parse_result<string_type> _d;

protected:
    syntax_tree (parse_result<string_type> && result)
        : _d(std::move(result))
    {}

public:
    syntax_tree (syntax_tree const &) = delete;
    syntax_tree & operator = (syntax_tree const &) = delete;

    syntax_tree (syntax_tree &&) = default;
    syntax_tree & operator = (syntax_tree &&) = default;

    std::error_code error_code () const
    {
        return _d.ec;
    }

    int error_line () const
    {
        return _d.lineno;
    }

    string_type error_text () const
    {
        return _d.what;
    }

    size_t rules_count () const
    {
        return _d.root ? _d.root->size() : 0;
    }

private:
    template <typename Visitor>
    void traverse_helper (Visitor & vis, basic_node const * n) const
    {
        switch (n->type()) {
            case node_enum::prose: {
                auto pn = static_cast<prose_node<string_type> const *>(n);
                vis.prose(pn->text());
                break;
            }

            case node_enum::number: {
                auto nn = static_cast<number_node<string_type> const *>(n);

                if (nn->is_range()) {
                    auto first = nn->begin();
                    auto second = ++nn->begin();
                    vis.number_range(*first, *second);
                } else {
                    auto first = nn->begin();
                    auto last = nn->end();

                    for (; first != last; ++first)
                        vis.number(*first);
                }
                break;
            }

            case node_enum::quoted_string: {
                auto qn = static_cast<quoted_string_node<string_type> const *>(n);
                vis.quoted_string(qn->text());
                break;
            }

            case node_enum::rulename: {
                auto rn = static_cast<rulename_node<string_type> const *>(n);
                vis.rulename(rn->text());
                break;
            }

            case node_enum::repetition: {
                auto rn = static_cast<repetition_node const *>(n);
                vis.begin_repetition();
                traverse_helper(vis, rn->element());
                vis.end_repetition();
                break;
            }

            case node_enum::group: {
                auto gn = static_cast<group_node const *>(n);
                vis.begin_group();
                gn->visit([this, & vis] (basic_node const * n) {
                    traverse_helper(vis, n);
                });
                vis.end_group();
                break;
            }

            case node_enum::option: {
                auto on = static_cast<option_node const *>(n);
                vis.begin_option();
                on->visit([this, & vis] (basic_node const * n) {
                    traverse_helper(vis, n);
                });
                vis.end_option();
                break;
            }

            case node_enum::concatenation: {
                auto cn = static_cast<concatenation_node const *>(n);
                vis.begin_concatenation();
                cn->visit([this, & vis] (basic_node const * n) {
                    traverse_helper(vis, n);
                });
                vis.end_concatenation();
                break;
            }

            case node_enum::alternation: {
                auto an = static_cast<alternation_node const *>(n);
                vis.begin_alternation();
                an->visit([this, & vis] (basic_node const * n) {
                    traverse_helper(vis, n);
                });
                vis.end_alternation();
                break;
            }

            case node_enum::rule: {
                auto rn = static_cast<rule_node<string_type> const *>(n);
                vis.begin_rule(rn->name());
                rn->visit([this, & vis] (basic_node const * n) {
                    traverse_helper(vis, n);
                });
                vis.end_rule();
                break;
            }

            case node_enum::rulelist: {
                auto rn = static_cast<rulelist_node<string_type> const *>(n);
                vis.begin_document();
                rn->visit([this, & vis] (basic_node const * n) {
                    traverse_helper(vis, n);
                });
                vis.end_document();
                break;
            }

            default:
                break;
        }
    }

public:
    // Visitor requiremenets:
    //      * void prose (string_type const &)
    //      * void number_range (string_type const & from, string_type const & to)
    //      * void number (string_type const &)
    //      * void quoted_string () (string_type const &)
    //      * void rulename (string_type const &)
    //      * void begin_repetition ()
    //      * void end_repetition ()
    //      * void begin_group ()
    //      * void end_group ()
    //      * void begin_option ()
    //      * void end_option ()
    //      * void begin_concatenation ()
    //      * void end_concatenation ()
    //      * void begin_alternation ()
    //      * void end_alternation ()
    //      * void begin_rule (string_type const &)
    //      * void end_rule ()
    //      * void begin_document ()
    //      * void end_document ()
    template <typename Visitor>
    inline void traverse (Visitor && vis) const
    {
        basic_node const * n = & *_d.root;
        traverse_helper(vis, n);
    }

    template <typename S, typename F>
    friend syntax_tree<S> parse (F & first, F last);
};

/**
 * @brief Parse the grammar.
 * @param first First position.
 * @param last Last position.
 * @return Syntax tree.
 */
template <typename StringType, typename ForwardIterator>
inline syntax_tree<StringType> parse (ForwardIterator & first, ForwardIterator last)
{
    using context_type = syntax_tree_context<StringType, ForwardIterator>;
    using forward_iterator = typename context_type::forward_iterator;
    forward_iterator f(first);
    forward_iterator l(last);
    context_type ctx;

    auto r = advance_rulelist(f, l, ctx);
    first = f.base();

    return syntax_tree<StringType>{ctx.result()};
}

/**
 * @brief Parse the grammar.
 * @param source String representation of grammar.
 * @return Syntax tree.
 */
template <typename StringType>
inline syntax_tree<StringType> parse_source (StringType const & source)
{
    return parse(source.begin(), source.end());
}

}}} // pfs::parser::abnf
