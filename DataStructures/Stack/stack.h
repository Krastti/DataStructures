#ifndef DATASTRUCTURES_STACK_H
#define DATASTRUCTURES_STACK_H

template <typename T>
class Stack {
private:
    struct Node {
        T data;
        Node *next;
        explicit Node(T data, Node *prev=nullptr) : data(data), next(prev) {}
    };

    Node *head;
public:
    Stack();
    explicit Stack(T data);

    void push(T value);
    void pop();
    [[nodiscard]] T top() const;
    [[nodiscard]] bool empty() const;

    ~Stack();
};

#include "stack.tpp"

#endif //DATASTRUCTURES_STACK_H
