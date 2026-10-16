#include "HandCursorFilter.h"

#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>

#include <algorithm>

namespace {

bool isSpinBoxArrow(QQuickItem *item) {
    QQuickItem *parent = item->parentItem();

    if (!parent || !parent->inherits("QQuickSpinBox")) {
        return false;
    }

    for (const char *name : {"up", "down"}) {
        const QObject *button = parent->property(name).value<QObject *>();

        if (button && button->property("indicator").value<QQuickItem *>() == item) {
            return true;
        }
    }

    return false;
}

// The topmost visible item at a scene position that takes mouse or hover events, children before parents.
// Decorations over a control (labels, outlines, backgrounds) take neither: the control is under the mouse.
QQuickItem *itemAt(QQuickItem *item, const QPointF &scenePosition) {
    // A drop area covers a whole page but neither draws nor takes clicks.
    if (!item->isVisible() || item->opacity() <= 0 || item->inherits("QQuickDropArea")) {
        return nullptr;
    }

    const QPointF localPosition = item->mapFromScene(scenePosition);

    if (item->clip() && !item->contains(localPosition)) {
        return nullptr;
    }

    QList<QQuickItem *> children = item->childItems();
    std::stable_sort(children.begin(), children.end(),
                     [](const QQuickItem *left, const QQuickItem *right) { return left->z() < right->z(); });

    for (auto child = children.crbegin(); child != children.crend(); ++child) {
        if (QQuickItem *found = itemAt(*child, scenePosition)) {
            return found;
        }
    }

    // The popup layer covers the whole window while any popup (even a tool tip) is shown; outside the popups
    // the items below it are under the mouse. A modal popup puts its dimmer into the layer.
    if (item->inherits("QQuickOverlay")) {
        return nullptr;
    }

    const bool takesInput =
        item->acceptedMouseButtons() != Qt::NoButton || item->acceptHoverEvents() || isSpinBoxArrow(item);

    return takesInput && item->contains(localPosition) ? item : nullptr;
}

} // namespace

bool HandCursorFilter::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseMove) {
        if (auto *window = qobject_cast<QQuickWindow *>(watched)) {
            updateCursor(window, static_cast<QMouseEvent *>(event)->scenePosition());
        }
    }

    return QObject::eventFilter(watched, event);
}

void HandCursorFilter::updateCursor(QQuickWindow *window, const QPointF &scenePosition) {
    // The content item's parent also holds the overlay with popups and dialogs.
    QQuickItem *root = window->contentItem();

    while (root->parentItem()) {
        root = root->parentItem();
    }

    QQuickItem *hovered = itemAt(root, scenePosition);
    QQuickItem *clickable = hovered ? clickableItem(hovered) : nullptr;

    if (!clickable) {
        return;
    }

    if (clickable->isEnabled()) {
        if (clickable->cursor().shape() != Qt::PointingHandCursor) {
            clickable->setCursor(Qt::PointingHandCursor);
        }
    } else if (clickable->cursor().shape() == Qt::PointingHandCursor) {
        clickable->unsetCursor();
    }
}

QQuickItem *HandCursorFilter::clickableItem(QQuickItem *item) {
    for (; item; item = item->parentItem()) {
        if (item->inherits("QQuickTextInput") || item->inherits("QQuickTextEdit")) {
            // Text fields keep their I-beam (an editable spin box, the language filter).
            return nullptr;
        }

        if (item->inherits("QQuickAbstractButton") || item->inherits("QQuickComboBox") || isSpinBoxArrow(item)) {
            return item;
        }
    }

    return nullptr;
}
