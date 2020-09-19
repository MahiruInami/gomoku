#ifndef MCTSNODE_H
#define MCTSNODE_H


class MCTSNode
{
public:
    static long NODES_CREATED;
    static short MAX_DEPTH;

    MCTSNode();

    bool isLeaf() const { return _head == nullptr; }

    unsigned getPlayouts() const { return _playouts; }
    void setPlayouts(unsigned value) { _playouts = value; }
    void addPlayout() { _playouts++; }

    unsigned getRealPlayouts() const { return _realPlayouts; }
    void addRealPlayouts(unsigned value) { _realPlayouts += value; }

    float getScore() const { return _score; }
    void setScore(float value) { _score = value; }
    void addScore(float value) { _score += value; }

    bool isTerminal() const { return _isTerminal; }
    void setTerminal() { _isTerminal = true; }

    MCTSNode* getParent() const { return _parent; }
    void setParent(MCTSNode* parent) { _parent = parent; }

    MCTSNode* getChildHead() const { return _head; }
    MCTSNode* getNextNode() const { return _next; }

    void addChild(MCTSNode* child);

    short getChildrenCount() const { return _childrenCount; }

    void setUserData(unsigned data) { _userData = data; }
    unsigned getUserData() const { return _userData; }

    short __x = 0;
    short __y = 0;
    short __color = 0;
    short __depth = 0;
private:
    MCTSNode* _head = nullptr;
    MCTSNode* _next = nullptr;

    MCTSNode* _parent = nullptr;

    unsigned _playouts = 0;
    unsigned _realPlayouts = 0;
    short _childrenCount = 0;
    float _score = 0.f;

    bool _isTerminal = false;

    unsigned _userData = 0;
};

#endif // MCTSNODE_H
