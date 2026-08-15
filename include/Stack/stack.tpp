#pragma once
#include <stdexcept>

template<typename T>
Stack<T>::Stack() {
    head = nullptr;
}

template<typename T>
Stack<T>::Stack(T data) {
    head = new Node(data);
}

template<typename T>
void Stack<T>::push(T value) {
    Node *tmp = head;
    head = new Node(value);
    head->next = tmp;
}

template<typename T>
void Stack<T>::pop() {
    if (empty()) throw std::logic_error("Stack is empty");
    Node *tmp = head->next;
    delete head;
    head = tmp;
}

template<typename T>
T Stack<T>::top() const {
    if (empty()) throw std::logic_error("Stack is empty");
    return head->data;
}

template<typename T>
bool Stack<T>::empty() const {
    return head == nullptr;
}

template<typename T>
Stack<T>::~Stack() {
    Node *current = head;
    while (current != nullptr) {
        Node *next = current->next;
        delete current;
        current = next;
    }
}

