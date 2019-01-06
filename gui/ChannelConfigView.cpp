#include "ChannelConfigView.h"
#include "ChannelConfigModel.h"
#include <anmp.hpp>

#include <QAction>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QStandardItemModel>
#include <QHeaderView>

ChannelConfigView::ChannelConfigView(QWidget *parent)
: QTableView(parent),
  contextMenu(new QMenu(this)),
  actionSelect_Muted_Voices(new QAction(this)),
  actionSelect_Unmuted_Voices(new QAction(this)),
  actionMute_Selected_Voices(new QAction(this)),
  actionUnmute_Selected_Voices(new QAction(this)),
  actionMute_All_Voices(new QAction(this)),
  actionUnmute_All_Voices(new QAction(this)),
  actionToggle_All_Voices(new QAction(this)),
  actionSolo_Selected_Voices(new QAction(this))
{
    this->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setAcceptDrops(false);
    this->setDragEnabled(false);

    this->actionSelect_Muted_Voices->setObjectName(QStringLiteral("actionSelect_Muted_Voices"));
    this->actionSelect_Muted_Voices->setText("&Select Muted Voices");

    this->actionSelect_Unmuted_Voices->setObjectName(QStringLiteral("actionSelect_Unmuted_Voices"));
    this->actionSelect_Unmuted_Voices->setText("S&elect Unmuted Voices");

    this->actionMute_Selected_Voices->setObjectName(QStringLiteral("actionMute_Selected_Voices"));
    this->actionMute_Selected_Voices->setText("&Mute Selected Voices [Del]");

    this->actionUnmute_Selected_Voices->setObjectName(QStringLiteral("actionUnmute_Selected_Voices"));
    this->actionUnmute_Selected_Voices->setText("&Unmute Selected Voices [Ins]");

    this->actionMute_All_Voices->setObjectName(QStringLiteral("actionMute_All_Voices"));
    this->actionMute_All_Voices->setText("Mute &All Voices");

    this->actionUnmute_All_Voices->setObjectName(QStringLiteral("actionUnmute_All_Voices"));
    this->actionUnmute_All_Voices->setText("Unmute All &Voices");

    this->actionToggle_All_Voices->setObjectName(QStringLiteral("actionToggle_All_Voices"));
    this->actionToggle_All_Voices->setText("&Toogle All Voices");

    this->actionSolo_Selected_Voices->setObjectName(QStringLiteral("actionSolo_Selected_Voices"));
    this->actionSolo_Selected_Voices->setText("S&olo Selected Voices [Shift + Ins]");

    QMenu *m = this->contextMenu;
    m->addAction(this->actionSelect_Muted_Voices);
    m->addAction(this->actionSelect_Unmuted_Voices);
    m->addSeparator();
    m->addAction(this->actionMute_Selected_Voices);
    m->addAction(this->actionUnmute_Selected_Voices);
    m->addAction(this->actionSolo_Selected_Voices);
    m->addSeparator();
    m->addAction(this->actionMute_All_Voices);
    m->addAction(this->actionUnmute_All_Voices);
    m->addAction(this->actionToggle_All_Voices);

    connect(this->actionSelect_Muted_Voices, &QAction::triggered, this, [this] { this->Select(Qt::Unchecked); });
    connect(this->actionSelect_Unmuted_Voices, &QAction::triggered, this, [this] { this->Select(Qt::Checked); });
    connect(this->actionMute_Selected_Voices, &QAction::triggered, this, &ChannelConfigView::MuteSelectedVoices);
    connect(this->actionUnmute_Selected_Voices, &QAction::triggered, this, &ChannelConfigView::UnmuteSelectedVoices);
    connect(this->actionSolo_Selected_Voices, &QAction::triggered, this, &ChannelConfigView::SoloSelectedVoices);
    connect(this->actionMute_All_Voices, &QAction::triggered, this, &ChannelConfigView::MuteAllVoices);
    connect(this->actionUnmute_All_Voices, &QAction::triggered, this, &ChannelConfigView::UnmuteAllVoices);
    connect(this->actionToggle_All_Voices, &QAction::triggered, this, &ChannelConfigView::ToggleAllVoices);
}

void ChannelConfigView::Select(Qt::CheckState state)
{
    QStandardItemModel *model = dynamic_cast<QStandardItemModel *>(this->model());
    if (model == nullptr)
    {
        return;
    }

    QItemSelectionModel *sel = this->selectionModel();
    sel->clearSelection();
    for (int i = 0; i < model->rowCount(); i++)
    {
        QStandardItem *item = model->item(i, 0);
        if (item->checkState() == state)
        {
            sel->select(model->index(i, 0), QItemSelectionModel::Select);
        }
    }
}

void ChannelConfigView::contextMenuEvent(QContextMenuEvent *event)
{
    if(contextMenu == nullptr)
    {
        return;
    }

    // open the context menu slightly below where user clicked, else he might accidently trigger first menu element
    QPoint offset(0, 7);
    QPoint p = event->globalPos();
    p += offset;
    this->contextMenu->exec(p);
}

void ChannelConfigView::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Qt::Key_Delete: // mute selection
            this->MuteSelectedVoices();
            break;
            
        case Qt::Key_Insert: // unmute / solo selection
            if(event->modifiers() & Qt::ShiftModifier)
            {
                this->SoloSelectedVoices();
            }
            else
            {
                this->UnmuteSelectedVoices();
            }
            break;
            
        case Qt::Key_Return:
        case Qt::Key_Enter: // toggle selection
            this->ToggleSelectedVoices();
            break;
            
        default:
            QTableView::keyPressEvent(event);
            break;
    }
}

void ChannelConfigView::DoChannelMuting(bool (*pred)(bool voiceIsMuted, bool voiceIsSelected))
{
    ChannelConfigModel *cmodel = dynamic_cast<ChannelConfigModel*>(this->model());
    const SongFormat *f;
    if (cmodel == nullptr || (f = cmodel->getCurrentSongFormat()) == nullptr)
    {
        return;
    }

    QModelIndex root = cmodel->invisibleRootItem()->index();
    QItemSelectionModel *sel = this->selectionModel();
    QModelIndex curIdx = sel->currentIndex();
    QModelIndexList selRows = sel->selectedRows();

    for (int i = 0; i < f->Voices; i++)
    {
        f->VoiceIsMuted[i] = (*pred)(f->VoiceIsMuted[i], sel->isRowSelected(i, root));
    }
    cmodel->updateChannelConfig(f);

    // restore selection, focus and currentIndex
    sel->clearSelection();
    for (QModelIndex i : selRows)
    {
        sel->select(i, QItemSelectionModel::Select);
    }
    sel->setCurrentIndex(curIdx, QItemSelectionModel::NoUpdate);

    this->setFocus();
}

void ChannelConfigView::ToggleSelectedVoices()
{
    this->DoChannelMuting([](bool voiceMuted, bool voiceSelected) { return voiceMuted != voiceSelected; });
}

void ChannelConfigView::MuteSelectedVoices()
{
    this->DoChannelMuting([](bool voiceMuted, bool voiceSelected) { return voiceMuted || voiceSelected; });
}

void ChannelConfigView::UnmuteSelectedVoices()
{
    this->DoChannelMuting([](bool voiceMuted, bool voiceSelected) { return voiceMuted && !voiceSelected; });
}

void ChannelConfigView::SoloSelectedVoices()
{
    this->DoChannelMuting([](bool, bool voiceSelected) { return !voiceSelected; });
}

void ChannelConfigView::MuteAllVoices()
{
    this->DoChannelMuting([](bool, bool) { return true; });
}

void ChannelConfigView::UnmuteAllVoices()
{
    this->DoChannelMuting([](bool, bool) { return false; });
}

void ChannelConfigView::ToggleAllVoices()
{
    this->DoChannelMuting([](bool voiceMuted, bool) { return !voiceMuted; });
}

