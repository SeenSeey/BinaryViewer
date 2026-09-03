#include "ui/UiEffects.h"

#include <QAbstractAnimation>
#include <QAbstractButton>
#include <QAction>
#include <QEasingCurve>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QPointer>
#include <QPropertyAnimation>
#include <QToolButton>

namespace
{
class PressFeedback final : public QObject
{
public:
    explicit PressFeedback(QAbstractButton* button)
        : QObject(button)
        , effect_(new QGraphicsOpacityEffect(button))
    {
        effect_->setOpacity(1.0);
        button->setGraphicsEffect(effect_);
        button->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        bool shouldAnimate = event->type() == QEvent::MouseButtonPress;
        if (event->type() == QEvent::KeyPress) {
            const auto* keyEvent = static_cast<QKeyEvent*>(event);
            shouldAnimate = !keyEvent->isAutoRepeat()
                && (keyEvent->key() == Qt::Key_Space
                    || keyEvent->key() == Qt::Key_Return
                    || keyEvent->key() == Qt::Key_Enter);
        }
        if (shouldAnimate) {
            animate();
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void animate()
    {
        if (animation_) {
            animation_->stop();
            animation_->deleteLater();
        }
        effect_->setOpacity(1.0);
        animation_ = new QPropertyAnimation(effect_, "opacity", this);
        animation_->setDuration(160);
        animation_->setStartValue(1.0);
        animation_->setKeyValueAt(0.35, 0.70);
        animation_->setEndValue(1.0);
        animation_->setEasingCurve(QEasingCurve::OutCubic);
        animation_->start(QAbstractAnimation::DeleteWhenStopped);
    }

    QGraphicsOpacityEffect* effect_;
    QPointer<QPropertyAnimation> animation_;
};

class NavigationMotion final : public QObject
{
public:
    NavigationMotion(QToolButton* button, const int direction)
        : QObject(button)
        , button_(button)
        , direction_(direction)
    {
    }

    void play()
    {
        if (animation_) {
            animation_->stop();
            button_->move(basePosition_);
            animation_->deleteLater();
        }

        basePosition_ = button_->pos();
        animation_ = new QPropertyAnimation(button_, "pos", this);
        animation_->setDuration(150);
        animation_->setStartValue(basePosition_);
        animation_->setKeyValueAt(0.45, basePosition_ + QPoint(direction_ * 4, 0));
        animation_->setEndValue(basePosition_);
        animation_->setEasingCurve(QEasingCurve::InOutQuad);
        connect(animation_, &QPropertyAnimation::finished, this, [this] {
            button_->move(basePosition_);
        });
        animation_->start(QAbstractAnimation::DeleteWhenStopped);
    }

private:
    QToolButton* button_;
    int direction_;
    QPoint basePosition_;
    QPointer<QPropertyAnimation> animation_;
};
} // namespace

void UiEffects::addPressFeedback(QAbstractButton* button)
{
    button->setCursor(Qt::PointingHandCursor);
    new PressFeedback(button);
}

void UiEffects::addNavigationMotion(QToolButton* button, const int direction, QAction* action)
{
    auto* motion = new NavigationMotion(button, direction);
    QObject::connect(action, &QAction::triggered, motion, [motion] {
        motion->play();
    });
}
