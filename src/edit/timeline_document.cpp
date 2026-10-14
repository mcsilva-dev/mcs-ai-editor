#include "edit/timeline_document.h"

#include <QUndoCommand>
#include <utility>

namespace mcs::edit {

// ponytail: snapshot da timeline inteira por passo (limite 200); trilhas compartilhadas
// (shared_ptr) se timelines muito grandes pesarem na memória.
class SnapshotCommand : public QUndoCommand {
  public:
    SnapshotCommand(TimelineDocument* document, const QString& text, core::Timeline before,
                    core::Timeline after)
        : QUndoCommand(text), document_(document), before_(std::move(before)),
          after_(std::move(after)) {}

    void undo() override { document_->setTimeline(before_); }
    void redo() override { document_->setTimeline(after_); }

  private:
    TimelineDocument* document_;
    core::Timeline before_;
    core::Timeline after_;
};

TimelineDocument::TimelineDocument(QObject* parent) : QObject(parent) {
    stack_.setUndoLimit(200);
}

const core::Timeline& TimelineDocument::timeline() const {
    return timeline_;
}

QString TimelineDocument::apply(const QString& actionName, const core::EditResult& result) {
    if (!result.timeline) {
        return result.error;
    }
    stack_.push(new SnapshotCommand(this, actionName, timeline_, *result.timeline));
    return {};
}

void TimelineDocument::reset(const core::Timeline& timeline) {
    stack_.clear();
    setTimeline(timeline);
    stack_.setClean();
}

QUndoStack* TimelineDocument::undoStack() {
    return &stack_;
}

bool TimelineDocument::isModified() const {
    return !stack_.isClean();
}

void TimelineDocument::markSaved() {
    stack_.setClean();
}

void TimelineDocument::setTimeline(const core::Timeline& timeline) {
    timeline_ = timeline;
    emit timelineChanged();
}

} // namespace mcs::edit
