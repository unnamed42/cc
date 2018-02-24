#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <cstdint>

namespace Compiler {
namespace Text {

class UChar;
class UString;

/**
 * Buffer for constructing UTF-8 strings.
 */
class Buffer {
    private:
        /**< capacity */
        uint32_t m_cap;
        /**< string object */
        UString *m_str;
    public:
        Buffer();
        
        ~Buffer() noexcept;
        
        void append(UChar ch);
        
        /**
         * Finish working, prepare for constructing next string
         * @return constructed string
         */
        UString* finish();
    
    private:
        void clear();
        
        void resize();
};

} // namespace Text
} // namespace Compiler

#endif // BUFFER_HPP
