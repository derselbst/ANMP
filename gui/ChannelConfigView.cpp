#include "ChannelConfigView.h"
#include "mainwindow.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QStandardItemModel>

ChannelConfigView::ChannelConfigView(QWidget *parent)
: QTableView(parent)
{
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

void ChannelConfigView::SetContextMenu(QMenu *menu)
{
    this->contextMenu = menu;
}

void ChannelConfigView::contextMenuEvent(QContextMenuEvent *event)
{
    // open the context menu slightly below where user clicked, else he might accidently trigger first menu element
    QPoint offset(0, 7);
    QPoint p = event->globalPos();
    p += offset;
    this->contextMenu->exec(p);
}
