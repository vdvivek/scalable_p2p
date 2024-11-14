#ifndef GROUNDNODE_H
#define GROUNDNODE_H

#include "Node.h"
#include <string>
#include <memory>

class GroundNode : public Node {
public:
    GroundNode(int id, const std::string &name, double x, double y);

    void updatePosition() override; // Ground nodes remain stationary, so no action needed
    bool connect(std::shared_ptr<Node> other) override; // Connection method implementation

    void sendData(const std::string &data, std::shared_ptr<Node> targetNode);
    void receiveData(const std::string &data);

private:
    // TODO: Additional methods for discovery and routing
};

#endif
