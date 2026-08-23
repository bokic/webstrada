#include <webstrada/parser.h>
#include <webstrada/exceptions.h>

#include <textparser.hpp>
#include <cfml_definition.json.h>
#include <threads.h>


using namespace webstrada;

parser::~parser() noexcept
{
    close();
}

void parser::parse(const string &pathname, textparser_encoding text_format)
{
    // Clean up any existing resources first.
    close();

    // Validate input
    if (pathname.isEmpty()) {
        throw webstrada::exception("Parsing error!", "Pathname is empty!");
    }

    // Open the file
    int res = textparser_openfile(pathname.constData(), text_format, cfml_definition.supported_bom, &m_handle);
    if (res) {
        throw webstrada::exception("Parsing error!", "Error opening file for parsing!");
    }

    // Parse the content
    res = textparser_parse(m_handle, &cfml_definition);
    if (res) {
        close();
        throw webstrada::exception("Parsing error!", "Parsing failed!");
    }

    // On success, keep the handle open and retrieve the token list.
    m_next_token = textparser_get_first_token(m_handle);
    // Build the line map so textparser_get_line_number_at_position works
    // (positions are byte offsets, so the map is encoding-independent).
    textparser_build_line_map(m_handle);
}

void parser::parse(const char* buffer, size_t buffer_size, textparser_encoding text_format, const char *filename)
{
    // Clean up any existing resources first.
    close();

    // Validate input
    if (buffer == nullptr) {
        throw webstrada::exception("Parsing error!", "Buffer is null or empty!");
    }

    // Open the memory buffer
    int res = textparser_openmem(buffer, buffer_size, text_format, &m_handle);
    if (res) {
        throw webstrada::exception("Parsing error!", "Error opening memory buffer for parsing!");
    }

    // Carry the source filename so per-extension grammar overrides (.cfc)
    // apply to the tokenization.
    if (filename != nullptr) {
        textparser_set_filename(m_handle, filename);
    }

    // Parse the content
    res = textparser_parse(m_handle, &cfml_definition);
    if (res) {
        close();
        throw webstrada::exception("Parsing error!", "Parsing failed!");
    }

    // On success, keep the handle open and retrieve the token list.
    m_next_token = textparser_get_first_token(m_handle);
    // Build the line map so textparser_get_line_number_at_position works
    // (positions are byte offsets, so the map is encoding-independent).
    textparser_build_line_map(m_handle);
}

textparser_token_item *parser::next_token()
{
    while (m_next_token && (m_next_token->token_id < TextParser_cfml_ScriptTagPair ||
                            m_next_token->token_id > TextParser_cfml_ArrayIndex)) {
        m_next_token = m_next_token->next;
    }
    textparser_token_item *ret = m_next_token;

    if (ret) {
        m_next_token = m_next_token->next;
    }

    return ret;
}

const char *parser::get_text() const
{
    if (m_handle == nullptr) {
        return nullptr;
    }

    return textparser_get_text(m_handle);
}

size_t parser::get_text_size() const
{
    if (m_handle == nullptr) {
        return 0;
    }

    return textparser_get_text_size(m_handle);
}

void parser::close() noexcept
{
    if (m_handle) {
        textparser_close(m_handle);
        m_handle = nullptr;
        m_next_token = nullptr;
    }
}
