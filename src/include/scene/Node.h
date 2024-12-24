#pragma once

#include <memory>
#include <string>
#include <vector>

/**
 * @class Node
 * @brief A class representing a node in a tree structure.
 *
 * This class is designed to represent objects on scene. Each node can have
 * multiple children but only one parent. Each node can be uniquely identified
 * by its name. This class uses shared pointers to manage the memory of the
 * nodes.
 */
class Node : public std::enable_shared_from_this<Node> {
public:
    /**
     * @brief Construct a new Node object.
     *
     * @param parent The parent of the node. Default is nullptr, indicating that
     * the node has no parent.
     * @param name The name of the node. Default is "without name".
     */
    explicit Node(std::shared_ptr<Node> parent = nullptr,
                  std::string name = "without name");

    /**
     * @brief Destroy the Node object.
     */
    ~Node();

    /**
     * @brief Get the parent of the node.
     *
     * @return A shared pointer to the parent node.
     */
    std::shared_ptr<Node> get_parent() const;

    /**
     * @brief Get the children of the node.
     *
     * @return A vector of shared pointers to the child nodes.
     */
    std::vector<std::shared_ptr<Node>> get_children() const;

    /**
     * @brief Add a child to the node.
     *
     * @return A shared pointer to the newly added child node.
     */
    std::shared_ptr<Node> add_child();

    /**
     * @brief Remove a child from the node.
     *
     * @param child The child node to be removed.
     */
    void remove_child(std::shared_ptr<Node> child);

    /**
     * @brief Get the name of the node.
     *
     * @return The name of the node.
     */
    std::string get_name() const;

    /**
     * @brief Change the name of the node.
     *
     * @param new_name The new name of the node.
     */
    void change_name(const std::string& new_name);

private:
    const std::weak_ptr<Node> _parent;
    std::vector<std::shared_ptr<Node>> _child;
    std::string _name;
};