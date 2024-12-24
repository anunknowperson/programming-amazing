#include "scene/Node.h"

#include <utility>

#include "core/Logging.h"

Node::Node(std::shared_ptr<Node> parent, std::string name)
    : _parent(parent), _name(std::move(name)) {
    if (parent != nullptr) {
        parent->_child.push_back(shared_from_this());
    }
}

Node::~Node() {
    _child.clear();
    auto parent = _parent.lock();
    if (parent) {
        parent->remove_child(shared_from_this());
    }
}

std::shared_ptr<Node> Node::get_parent() const {
    return _parent.lock();
}

std::vector<std::shared_ptr<Node>> Node::get_children() const {
    return _child;
}

std::shared_ptr<Node> Node::add_child() {
    Node child(*this);
    auto child_ptr = std::make_shared<Node>(child);
    _child.push_back(child_ptr);
    return child_ptr;
}

void Node::remove_child(std::shared_ptr<Node> del_child) {
    auto child_iter = _child.begin();
    auto del_child_iter = _child.end();
    while (child_iter != _child.end()) {
        if (*child_iter == del_child) {
            del_child_iter = child_iter;
            break;
        }
        ++child_iter;
    }
    if (del_child_iter == _child.end()) {
        LOGW("Not possible to delete a non-existent child");
    } else {
        _child.erase(del_child_iter);
    }
}

std::string Node::get_name() const {
    return _name;
}

void Node::change_name(const std::string& new_name) {
    _name = new_name;
}