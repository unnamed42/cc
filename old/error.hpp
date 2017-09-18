#ifndef __COMPILER_ERROR__
#define __COMPILER_ERROR__

namespace compiler {

// To implement warning/error message like:
// In file [name]:[line]:[column]:\n[source code]
struct file_pos {
    const char *m_name; // name of the file
    const char *m_begin; // beginning position of current message
    unsigned int m_line;
    unsigned int m_column;
    
    file_pos()
        :m_name(nullptr), m_begin(nullptr), m_line(1), m_column(1) {}
};

struct token;

extern const file_pos *epos;

void print_fpos(const file_pos&) noexcept;

void error(const char*, ...) throw(int);
void error(const token*, const char*, ...) throw(int);
void error(const file_pos&, const char*, ...) throw(int);

void warning(const char*, ...) noexcept;
void warning(const token*, const char*, ...) noexcept;
void warning(const file_pos&, const char*, ...) noexcept;

} // namespace compiler

#endif // __COMPILER_ERROR__
