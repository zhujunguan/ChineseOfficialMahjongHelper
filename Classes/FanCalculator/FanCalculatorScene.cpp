﻿#include "FanCalculatorScene.h"
#include "../mahjong-algorithm/stringify.h"
#include "../mahjong-algorithm/fan_calculator.h"
#include "../widget/TilePickWidget.h"
#include "../widget/ExtraInfoWidget.h"
#include "../widget/AlertView.h"
#include "../widget/TilesKeyboard.h"

USING_NS_CC;

Scene *FanCalculatorScene::createScene() {
    auto scene = Scene::create();
    auto layer = FanCalculatorScene::create();
    scene->addChild(layer);
    return scene;
}

bool FanCalculatorScene::init() {
    if (UNLIKELY(!BaseLayer::initWithTitle("国标麻将算番器"))) {
        return false;
    }

    Size visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 选牌面板
    TilePickWidget *tilePicker = TilePickWidget::create();
    const Size &widgetSize = tilePicker->getContentSize();
    this->addChild(tilePicker);
    tilePicker->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - 35 - widgetSize.height * 0.5f));
    _tilePicker = tilePicker;

    // 其他信息的相关控件
    ExtraInfoWidget *extraInfo = ExtraInfoWidget::create();
    const Size &extraSize = extraInfo->getContentSize();
    this->addChild(extraInfo);
    extraInfo->setPosition(Vec2(origin.x + visibleSize.width * 0.5f,
        origin.y + visibleSize.height - 35 - widgetSize.height - 5 - extraSize.height * 0.5f));
    _extraInfo = extraInfo;

    // 直接输入
    ui::Button *button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(55.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("直接输入");
    extraInfo->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 40, 100.0f));
    button->addClickEventListener([this](Ref *) { showInputAlert(nullptr); });

    // 番算按钮
    button = ui::Button::create("source_material/btn_square_highlighted.png", "source_material/btn_square_selected.png");
    button->setScale9Enabled(true);
    button->setContentSize(Size(45.0f, 20.0f));
    button->setTitleFontSize(12);
    button->setTitleText("算番");
    extraInfo->addChild(button);
    button->setPosition(Vec2(visibleSize.width - 35, 10.0f));
    button->addClickEventListener([this](Ref *) { calculate(); });

    // 番种显示的Node
    Size areaSize(visibleSize.width, visibleSize.height - 35 - widgetSize.height - 5 - extraSize.height - 10);
    _fanAreaNode = Node::create();
    _fanAreaNode->setContentSize(areaSize);
    _fanAreaNode->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
    _fanAreaNode->setIgnoreAnchorPointForPosition(false);
    this->addChild(_fanAreaNode);
    _fanAreaNode->setPosition(Vec2(origin.x + visibleSize.width * 0.5f, origin.y + areaSize.height * 0.5f + 5));

    tilePicker->setFixedPacksChangedCallback([tilePicker, extraInfo]() {
        extraInfo->refreshByKong(tilePicker->isFixedPacksContainsKong());
    });

    tilePicker->setWinTileChangedCallback([tilePicker, extraInfo]() {
        ExtraInfoWidget::RefreshByWinTile rt;
        rt.getWinTile = std::bind(&TilePickWidget::getServingTile, tilePicker);
        rt.isStandingTilesContainsServingTile = std::bind(&TilePickWidget::isStandingTilesContainsServingTile, tilePicker);
        rt.countServingTileInFixedPacks = std::bind(&TilePickWidget::countServingTileInFixedPacks, tilePicker);
        rt.isFixedPacksContainsKong = std::bind(&TilePickWidget::isFixedPacksContainsKong, tilePicker);
        extraInfo->refreshByWinTile(rt);
    });

    return true;
}

cocos2d::Node *createFanResultNode(const long (&fan_table)[mahjong::FAN_TABLE_SIZE], int fontSize, float resultAreaWidth) {
    // 有n个番种，每行排2个
    long n = mahjong::FAN_TABLE_SIZE - std::count(std::begin(fan_table), std::end(fan_table), 0);
    long rows = (n >> 1) + (n & 1);  // 需要这么多行

    // 排列
    Node *node = Node::create();
    const int lineHeight = fontSize + 2;
    long resultAreaHeight = lineHeight * rows;  // 每行间隔2像素
    resultAreaHeight += (5 + lineHeight);  // 总计
    node->setContentSize(Size(resultAreaWidth, resultAreaHeight));

    long fan = 0;
    for (int i = 0, j = 0; i < n; ++i) {
        while (fan_table[++j] == 0) continue;

        int f = mahjong::fan_value_table[j];
        long n = fan_table[j];
        fan += f * n;
        std::string str = (n == 1) ? StringUtils::format("%s %d番\n", mahjong::fan_name[j], f)
            : StringUtils::format("%s %d番x%ld\n", mahjong::fan_name[j], f, fan_table[j]);

        // 创建label，每行排2个
        Label *fanName = Label::createWithSystemFont(str, "Arial", fontSize);
        fanName->setColor(Color3B(0x60, 0x60, 0x60));
        node->addChild(fanName);
        fanName->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
        div_t ret = div(i, 2);
        fanName->setPosition(Vec2(ret.rem == 0 ? 10.0f : resultAreaWidth * 0.5f, resultAreaHeight - lineHeight * (ret.quot + 1)));
    }

    Label *fanTotal = Label::createWithSystemFont(StringUtils::format("总计：%ld番", fan), "Arial", fontSize);
    fanTotal->setColor(Color3B::BLACK);
    node->addChild(fanTotal);
    fanTotal->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
    fanTotal->setPosition(Vec2(10.0f, lineHeight * 0.5f));

    return node;
}

void FanCalculatorScene::showInputAlert(const char *prevInput) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    const float width = visibleSize.width * 0.8f - 10;

    ui::Widget *rootWidget = ui::Widget::create();

    Label *label = Label::createWithSystemFont("使用说明：\n"
        "1." INPUT_GUIDE_STRING_1 "\n"
        "2." INPUT_GUIDE_STRING_2 "\n"
        "3." INPUT_GUIDE_STRING_3 "\n"
        "输入范例1：[EEEE][CCCC][FFFF][PPPP]NN\n"
        "输入范例2：1112345678999s9s\n"
        "输入范例3：WWWW 444s 45m678pFF6m\n", "Arial", 10);
    label->setColor(Color3B::BLACK);
    label->setDimensions(width, 0);
    rootWidget->addChild(label);

    // 输入手牌
    ui::EditBox *editBox = ui::EditBox::create(Size(width - 10, 20.0f), ui::Scale9Sprite::create("source_material/btn_square_normal.png"));
    editBox->setInputFlag(ui::EditBox::InputFlag::SENSITIVE);
    editBox->setInputMode(ui::EditBox::InputMode::SINGLE_LINE);
    editBox->setFontColor(Color4B::BLACK);
    editBox->setFontSize(12);
    editBox->setPlaceholderFontColor(Color4B::GRAY);
    editBox->setPlaceHolder("输入手牌");
    if (prevInput != nullptr) {
        editBox->setText(prevInput);
    }
    TilesKeyboard::hookEditBox(editBox);
    rootWidget->addChild(editBox);

    const Size &labelSize = label->getContentSize();
    rootWidget->setContentSize(Size(width, labelSize.height + 30));
    editBox->setPosition(Vec2(width * 0.5f, 10));
    label->setPosition(Vec2(width * 0.5f, labelSize.height * 0.5f + 30));

    AlertView::showWithNode("直接输入", rootWidget, [this, editBox]() {
        const char *input = editBox->getText();
        const char *errorStr = TilesKeyboard::parseInput(input,
            std::bind(&TilePickWidget::setData, _tilePicker, std::placeholders::_1, std::placeholders::_2));
        if (errorStr != nullptr) {
            const std::string str = input;
            AlertView::showWithMessage("直接输入牌", errorStr, 12, [this, str]() {
                showInputAlert(str.c_str());
            }, nullptr);
        }
    }, nullptr);
}

void FanCalculatorScene::calculate() {
    _fanAreaNode->removeAllChildren();

    const Size &fanAreaSize = _fanAreaNode->getContentSize();
    Vec2 pos(fanAreaSize.width * 0.5f, fanAreaSize.height * 0.5f);

    int flowerCnt = _extraInfo->getFlowerCount();
    if (flowerCnt > 8) {
        AlertView::showWithMessage("算番", "花牌数的范围为0~8", 12, nullptr, nullptr);
        return;
    }

    mahjong::calculate_param_t param;
    _tilePicker->getData(&param.hand_tiles, &param.win_tile);
    if (param.win_tile == 0) {
        AlertView::showWithMessage("算番", "牌张数错误", 12, nullptr, nullptr);
        return;
    }

    std::sort(param.hand_tiles.standing_tiles, param.hand_tiles.standing_tiles + param.hand_tiles.tile_count);

    param.flower_count = flowerCnt;
    long fan_table[mahjong::FAN_TABLE_SIZE] = { 0 };

    // 获取绝张、杠开、抢杠、海底信息
    mahjong::win_flag_t win_flag = _extraInfo->getWinFlag();

    // 获取圈风门风
    mahjong::wind_t prevalent_wind = _extraInfo->getPrevalentWind();
    mahjong::wind_t seat_wind = _extraInfo->getSeatWind();

    // 算番
    param.ext_cond.win_flag = win_flag;
    param.ext_cond.prevalent_wind = prevalent_wind;
    param.ext_cond.seat_wind = seat_wind;
    int fan = calculate_fan(&param, fan_table);

    if (fan == ERROR_NOT_WIN) {
        Label *errorLabel = Label::createWithSystemFont("诈和", "Arial", 14);
        errorLabel->setColor(Color3B::BLACK);
        _fanAreaNode->addChild(errorLabel);
        errorLabel->setPosition(pos);
        return;
    }
    if (fan == ERROR_WRONG_TILES_COUNT) {
        AlertView::showWithMessage("算番", "牌张数错误", 12, nullptr, nullptr);
        return;
    }
    if (fan == ERROR_TILE_COUNT_GREATER_THAN_4) {
        AlertView::showWithMessage("算番", "同一种牌最多只能使用4枚", 12, nullptr, nullptr);
        return;
    }

    Node *innerNode = createFanResultNode(fan_table, 14, fanAreaSize.width);

    // 超出高度就使用ScrollView
    if (innerNode->getContentSize().height <= fanAreaSize.height) {
        _fanAreaNode->addChild(innerNode);
        innerNode->setAnchorPoint(Vec2(0.5f, 0.5f));
        innerNode->setPosition(pos);
    }
    else {
        ui::ScrollView *scrollView = ui::ScrollView::create();
        scrollView->setDirection(ui::ScrollView::Direction::VERTICAL);
        scrollView->setScrollBarPositionFromCorner(Vec2(10, 10));
        scrollView->setContentSize(fanAreaSize);
        scrollView->setInnerContainerSize(innerNode->getContentSize());
        scrollView->addChild(innerNode);

        _fanAreaNode->addChild(scrollView);
        scrollView->setAnchorPoint(Vec2(0.5f, 0.5f));
        scrollView->setPosition(pos);
    }
}
