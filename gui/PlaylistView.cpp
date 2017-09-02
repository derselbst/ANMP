#include "PlaylistView.h"
#include "IPlaylist.h"
#include "PlaylistModel.h"
#include "songinspector.h"
#include <QMenu>
#include <QKeyEvent>
#include <QItemSelectionModel>

PlaylistView::PlaylistView(QWidget * parent)
    : QTableView(parent)
{
}

// sort ascendingly
bool sortQModelIndexList(QModelIndex i, QModelIndex j)
{
    return i.row()<j.row();
}

// sort descendingly
bool sortQItemSelectionDesc(QItemSelectionRange i, QItemSelectionRange j)
{
    return i.bottom() > j.bottom();
}

bool sortQItemSelectionAsc(QItemSelectionRange i, QItemSelectionRange j)
{
    return i.bottom() < j.bottom();
}


void PlaylistView::moveItems(int steps)
{
    PlaylistModel* playlistModel = dynamic_cast<PlaylistModel*>(this->model());
    if(playlistModel==nullptr)
    {
        return;
    }

    QItemSelection indexList = this->selectionModel()->selection();
    std::sort(indexList.begin(), indexList.end(), steps>0 ? sortQItemSelectionDesc : sortQItemSelectionAsc);

    for(QItemSelection::const_iterator i=indexList.cbegin(); i!=indexList.cend(); ++i)
    {
        int top = i->top();
        int btm = i->bottom();
        int count = abs(top-btm);

        int srcRow = min(top, btm);
        int destRow = srcRow + steps;

        QModelIndex newIdx = steps > 0 ? playlistModel->index(destRow+count,0) : playlistModel->index(srcRow-1,0);
        QModelIndex oldIdx = steps > 0 ? playlistModel->index(srcRow,0) : playlistModel->index(srcRow+count,0);
        if(newIdx.isValid())
        {
            this->selectionModel()->select(newIdx, QItemSelectionModel::SelectionFlag::Select|QItemSelectionModel::SelectionFlag::Rows);
            this->selectionModel()->select(oldIdx, QItemSelectionModel::SelectionFlag::Deselect | QItemSelectionModel::SelectionFlag::Rows);


            playlistModel->moveRows(playlistModel->index(0,0),
                                    srcRow,
                                    count,
                                    playlistModel->index(playlistModel->rowCount(QModelIndex())-1,
                                            playlistModel->columnCount(QModelIndex())-1),
                                    destRow);
        }
    }
}

void PlaylistView::keyPressEvent(QKeyEvent * event)
{
    PlaylistModel* playlistModel = dynamic_cast<PlaylistModel*>(this->model());
    if(playlistModel==nullptr)
    {
        return;
    }

    int key = event->key();
    switch(key)
    {
    case Qt::Key_Delete:
    {
        QItemSelection indexList = this->selectionModel()->selection();

        int lastCount = 0;
        for(QItemSelection::const_iterator i=indexList.cbegin(); i!=indexList.cend(); ++i)
        {
            int btm = i->bottom();
            int top = i->top();

            if(btm <0 || top <0)
            {
                break;
            }

            lastCount = abs(top-btm)+1;
            playlistModel->removeRows(min(btm,top), lastCount);
        }
    }
    break;

    case Qt::Key_W:
        this->moveItems(-1);
        break;

    case Qt::Key_S:
        this->moveItems(1);
        break;

    default:
        QTableView::keyPressEvent(event);
        break;
    }
}


void PlaylistView::contextMenuEvent(QContextMenuEvent *event)
{
    QModelIndex i = this->currentIndex();
    if(i.isValid())
    {
        event->accept();
        QMenu menu;
        menu.addAction(QIcon::fromTheme("help-contents"), "Details", this, &PlaylistView::showInspector);
        menu.exec(event->globalPos());
    }
    else
    {
        QTableView::contextMenuEvent(event);
    }
}

void PlaylistView::showInspector()
{
    PlaylistModel* playlistModel = dynamic_cast<PlaylistModel*>(this->model());
    if(playlistModel==nullptr)
    {
        return;
    }

    QModelIndex i = this->currentIndex();
    if(i.isValid())
    {
        int row = i.row();
        const Song* s = playlistModel->getSong(row);

        SongInspector insp (s, this);
        insp.exec();
    }
}
