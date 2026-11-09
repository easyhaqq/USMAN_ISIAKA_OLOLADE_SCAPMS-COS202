#ifndef SCAPMS_CONTROLLER_UNDOMANAGER_H
#define SCAPMS_CONTROLLER_UNDOMANAGER_H

#include <functional>
#include <stack>
#include <string>
#include <vector>

namespace scapms::controller {

// One reversible administrative operation: a description for the GUI and a
// function that puts the data back exactly as it was before the operation.
struct UndoAction {
    std::string description;
    std::function<void()> revert;
};

// Undo Last Administrative Operation, implemented with std::stack (LIFO):
// the most recent operation is always the first one reversed.
class UndoManager {
public:
    void record(std::string description, std::function<void()> revert);
    std::string undoLast();                       // returns the description of what was undone
    bool canUndo() const { return !actions_.empty(); }
    std::size_t size() const { return actions_.size(); }
    std::string peekDescription() const;
    std::vector<std::string> history() const;     // most recent first
    void clear();

private:
    std::stack<UndoAction> actions_;
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_UNDOMANAGER_H
