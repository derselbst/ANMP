#include "channelconfig.h"
#include "ui_channelconfig.h"

#include "anmp.hpp"

#include <QPushButton>

ChannelConfig::ChannelConfig(Player* player, QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::ChannelConfig),
    player(player)
{
    ui->setupUi(this);
    this->player->onCurrentSongChanged += make_pair(this, &ChannelConfig::callbackCurrentSongChanged);
}

ChannelConfig::~ChannelConfig()
{
    delete ui;
    this->player->onCurrentSongChanged -= this;
}

void ChannelConfig::callbackCurrentSongChanged(void * context)
{
    ChannelConfig* ctx = static_cast<ChannelConfig*>(context);
    QMetaObject::invokeMethod( ctx, "onSongChanged", Qt::QueuedConnection);
}

void ChannelConfig::onSongChanged()
{
    // clear layout
    QLayoutItem* child=this->ui->voiceList->takeAt(0);
    while(child != nullptr)
    {
        if(child->widget())
        {
            this->ui->voiceList->removeWidget(child->widget());
            delete child->widget();
        }
        this->ui->voiceList->removeItem(child);
        delete child;
        child=this->ui->voiceList->takeAt(0);
    }


    const Song* s = this->player->getCurrentSong();
    if(s == nullptr)
    {
        this->currentFormat = nullptr;
        return;
    }

    this->currentFormat = &s->Format;
    for(int i=0; i < this->currentFormat->Voices; i++)
    {
        QPushButton* btn = new QPushButton(QString::fromStdString(this->currentFormat->VoiceName[i]), this);
        btn->setCheckable(true);
        btn->setChecked(!this->currentFormat->VoiceIsMuted[i]);
        btn->setStyleSheet("QPushButton:checked { background-color: lime; } QPushButton { background-color: red; } QPushButton:selected { background : transparent; border: 2px solid blue; }");
        btn->setFocusPolicy(Qt::StrongFocus);

        connect(btn, &QPushButton::toggled, this,
                [this, i](bool isChecked)
                {
                    this->player->Mute(i, !isChecked);
                });

        this->ui->voiceList->addWidget(btn);
    }
}
