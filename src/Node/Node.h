#ifndef NODE_H
#define NODE_H

#include <string>
#include <memory>

class Node {
public:
    Node(int id, const std::string &name, double x, double y);
    virtual ~Node() = default;

    int getId() const;
    double getX() const;
    double getY() const;
    std::string getName() const;

    virtual void updatePosition() = 0;
    virtual bool connect(std::shared_ptr<Node> other) = 0;

protected:
    int id;
    std::string name;
    double x, y;
};

#endif //NODE_H
