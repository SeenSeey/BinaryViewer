#pragma once

class QAction;
class QAbstractButton;
class QToolButton;

namespace UiEffects
{
void addPressFeedback(QAbstractButton* button);
void addNavigationMotion(QToolButton* button, int direction, QAction* action);
} // namespace UiEffects
