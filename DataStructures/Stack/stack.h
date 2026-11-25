#ifndef DATASTRUCTURES_STACK_H
#define DATASTRUCTURES_STACK_H

#include <type_traits>
#include <utility>

template <typename T>
class Stack {
private:
    struct Node {
        T data;
        Node *next;
        explicit Node(const T& data, Node* next = nullptr) : data(data), next(next) {}

        Node(T&& data, Node* next) requires std::is_move_constructible_v<T>
            : data(std::move(data)), next(next) {}
    };

    Node *head;
public:
    Stack();
    explicit Stack(T data);

    void push(const T& value);
    void push(T&& value);
    void pop();

    // Ссылка действительна до удаления элемента или уничтожения стека.
    [[nodiscard]] T& top();
    [[nodiscard]] const T& top() const;
    [[nodiscard]] bool empty() const;

    ~Stack();
};

#include "stack.tpp"

#endif //DATASTRUCTURES_STACK_H
