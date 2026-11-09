#include "controller/UndoManager.h"

#include "utils/Exceptions.h"

namespace scapms::controller {

void UndoManager::record(std::string description, std::function<void()> revert) {
    actions_.push(UndoAction{std::move(description), std::move(revert)});
}

std::string UndoManager::undoLast() {
    if (actions_.empty()) throw utils::ClinicException("There is no administrative operation to undo.");
    UndoAction action = std::move(actions_.top());
    actions_.pop();
    action.revert();   // may throw; the action is discarded so a broken entry cannot block the stack
    return action.description;
}

std::string UndoManager::peekDescription() const {
    return actions_.empty() ? "" : actions_.top().description;
}

std::vector<std::string> UndoManager::history() const {
    std::vector<std::string> descriptions;
    std::stack<UndoAction> copy = actions_;   // a stack can only be read from the top
    while (!copy.empty()) {
        descriptions.push_back(copy.top().description);
        copy.pop();
    }
    return descriptions;
}

void UndoManager::clear() {
    actions_ = std::stack<UndoAction>();
}

} // namespace scapms::controller
