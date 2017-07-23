﻿#ifndef _ALERT_LAYER_H_
#define _ALERT_LAYER_H_

#include "cocos2d.h"

class AlertView : public cocos2d::Layer {
public:
    bool initWithTitle(const std::string &title, cocos2d::Node *node, float maxWidth, const std::function<void()> &confirmCallback, const std::function<void()> &cancelCallback);

    static AlertView *showWithNode(const std::string &title, cocos2d::Node *node, const std::function<void ()> &confirmCallback, const std::function<void ()> &cancelCallback) {
        const float maxWidth = cocos2d::Director::getInstance()->getVisibleSize().width * 0.8f - 10;
        return showWithNode(title, node, maxWidth, confirmCallback, cancelCallback);
    }

    static AlertView *showWithNode(const std::string &title, cocos2d::Node *node, float maxWidth, const std::function<void ()> &confirmCallback, const std::function<void ()> &cancelCallback);
    static AlertView *showWithMessage(const std::string &title, const std::string &message, float fontSize, const std::function<void ()> &confirmCallback, const std::function<void ()> &cancelCallback);

    inline void close() { this->removeFromParent(); }

private:
    void onCancelButton(cocos2d::Ref *sender);
    void onConfirmButton(cocos2d::Ref *sender);

    std::function<void ()> _confirmCallback;
    std::function<void ()> _cancelCallback;
};

#endif
