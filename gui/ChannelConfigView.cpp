#include "ChannelConfigView.h"
#include "mainwindow.h"

#include <QStandardItemModel>
#include <QContextMenuEvent>
#include <QAction>
#include <QMenu>

ChannelConfigView::ChannelConfigView(QWidget * parent)
    : QTableView(parent)
{
    this->createActions();
}

void ChannelConfigView::createActions()
{
    this->actSelMuted = new QAction("Select muted voices", this);
    connect(this->actSelMuted, &QAction::triggered, this, &ChannelConfigView::SelectMuted);
    
    this->actSelUnmuted = new QAction("Select unmuted voices", this);
    connect(this->actSelUnmuted, &QAction::triggered, this, &ChannelConfigView::SelectUnmuted);
}

void ChannelConfigView::Select(Qt::CheckState state)
{
    QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(this->model());
    if(model==nullptr)
    {
        return;
    }

    QItemSelectionModel* sel = this->selectionModel();
    sel->clearSelection();
    for(int i=0; i<model->rowCount(); i++)
    {
        QStandardItem* item = model->item(i,0);
        if(item->checkState() == state)
        {
            sel->select(model->index(i,0), QItemSelectionModel::Select);
        }
    }
}

void ChannelConfigView::SelectUnmuted()
{
    this->Select(Qt::Checked);
}

void ChannelConfigView::SelectMuted()
{
    this->Select(Qt::Unchecked);
}

void ChannelConfigView::contextMenuEvent(QContextMenuEvent *event)
{    
    QMenu menu(this);
    menu.addAction(actSelMuted);
    menu.addAction(actSelUnmuted);
    menu.exec(event->globalPos());

}
