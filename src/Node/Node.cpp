#include "Node.h"

Node::Node(int id, const std::string &name, double x, double y)
    : id(id), name(name), x(x), y(y) {}

int Node::getId() const { return id; }

std::string Node::getName() const { return name; }

double Node::getX() const { return x; }

double Node::getY() const { return y; }