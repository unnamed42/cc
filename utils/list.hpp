#ifndef LIST_HPP
#define LIST_HPP

namespace Compiler {
namespace Utils {

class List {
    private:
        using self = List;
        
        struct ListNode {
            ListNode *next;
            void     *data;
        };
    private:
        ListNode *head, *end;
    public:
        
};

}
}

#endif // LIST_HPP
