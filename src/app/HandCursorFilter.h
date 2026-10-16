#pragma once

#include <QObject>

class QQuickItem;
class QQuickWindow;

// Gives clickable controls a pointing hand cursor: buttons of any kind (including dialog, menu and popup
// delegates that QML cannot reach), combo boxes and spin box arrows. Installed on the application, it
// looks at the item under the mouse on every move, before the window resolves the cursor.
class HandCursorFilter : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    static void updateCursor(QQuickWindow *window, const QPointF &scenePosition);
    static QQuickItem *clickableItem(QQuickItem *item);
};
