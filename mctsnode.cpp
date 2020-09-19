#include "mctsnode.h"

long MCTSNode::NODES_CREATED = 0;
short MCTSNode::MAX_DEPTH = 0;

MCTSNode::MCTSNode()
{
    NODES_CREATED++;
}


void MCTSNode::addChild(MCTSNode* child) {
    child->_next = _head;
    child->_parent = this;
    _head = child;

    child->__depth = __depth + 1;
    MAX_DEPTH = child->__depth > MAX_DEPTH ? __depth + 1 : MAX_DEPTH;

    this->_childrenCount++;
}
