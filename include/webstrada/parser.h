#pragma once

#include "string.h"

#include <textparser.hpp>


namespace webstrada {

class parser {
public:
    virtual ~parser() noexcept;

    void parse(const string &pathname, textparser_encoding text_format);
    // Parses an in-memory buffer. `filename` (optional) is passed to the
    // textparser so per-extension grammar overrides (e.g. the .cfc script
    // component start tokens) apply.
    void parse(const char* buffer, size_t buffer_size, textparser_encoding text_format, const char *filename = nullptr);
    textparser_token_item *next_token();
    const char *get_text() const;
    size_t get_text_size() const;
    // The textparser handle (its line map feeds textparser_get_line_number_at_position).
    textparser_t handle() const { return m_handle; }

private:
    void close() noexcept;

    textparser_t m_handle = nullptr;
    textparser_token_item *m_next_token = nullptr;
};

}
