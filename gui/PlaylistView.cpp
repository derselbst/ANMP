#include "PlaylistView.h"
#include "IPlaylist.h"
#include "Playlist.h"
#include "PlaylistModel.h"
#include "songinspector.h"
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMenu>
#include <QResizeEvent>
#include <QtGlobal>

PlaylistView::PlaylistView(QWidget *parent)
: QTableView(parent)
{
    this->setAcceptDrops(true);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setDragEnabled(true);
    this->setDragDropOverwriteMode(false);
    this->setDragDropMode(QAbstractItemView::DragDrop);
    this->setDefaultDropAction(Qt::MoveAction);
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->setShowGrid(false);
}

void PlaylistView::setModel(QAbstractItemModel *model)
{
    if (model != nullptr)
    {
        this->colSizes.resize(model->columnCount());
    }

    this->QTableView::setModel(model);
}

// sort ascendingly
bool sortQModelIndexList(QModelIndex i, QModelIndex j)
{
    return i.row() < j.row();
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
    PlaylistModel *playlistModel = dynamic_cast<PlaylistModel *>(this->model());
    if (playlistModel == nullptr)
    {
        return;
    }

    QItemSelection indexList = this->selectionModel()->selection();
    std::sort(indexList.begin(), indexList.end(), steps > 0 ? sortQItemSelectionDesc : sortQItemSelectionAsc);

    for (QItemSelection::const_iterator i = indexList.cbegin(); i != indexList.cend(); ++i)
    {
        int top = i->top();
        int btm = i->bottom();
        int count = abs(top - btm);

        int srcRow = min(top, btm);
        int destRow = srcRow + steps;

        QModelIndex newIdx = steps > 0 ? playlistModel->index(destRow + count, 0) : playlistModel->index(srcRow - 1, 0);
        QModelIndex oldIdx = steps > 0 ? playlistModel->index(srcRow, 0) : playlistModel->index(srcRow + count, 0);
        if (newIdx.isValid())
        {
            this->selectionModel()->select(newIdx, QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);
            this->selectionModel()->select(oldIdx, QItemSelectionModel::SelectionFlag::Deselect | QItemSelectionModel::SelectionFlag::Rows);


            playlistModel->moveRows(playlistModel->index(0, 0),
                                    srcRow,
                                    count,
                                    playlistModel->index(playlistModel->rowCount(QModelIndex()) - 1,
                                                         playlistModel->columnCount(QModelIndex()) - 1),
                                    destRow);
        }
    }
}

void PlaylistView::keyPressEvent(QKeyEvent *event)
{
    PlaylistModel *playlistModel = dynamic_cast<PlaylistModel *>(this->model());
    if (playlistModel == nullptr)
    {
        return;
    }

    int key = event->key();
    switch (key)
    {
        case Qt::Key_Delete:
        {
            QItemSelection indexList = this->selectionModel()->selection();

            int lastCount = 0;
            for (QItemSelection::const_iterator i = indexList.cbegin(); i != indexList.cend(); ++i)
            {
                int btm = i->bottom();
                int top = i->top();

                if (btm < 0 || top < 0)
                {
                    break;
                }

                lastCount = abs(top - btm) + 1;
                playlistModel->removeRows(min(btm, top), lastCount);
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QModelIndex i = this->currentIndex();
    if (i.isValid())
    {
        event->accept();
        QMenu menu;
        menu.addAction(QIcon::fromTheme("help-contents"), "Details", this, &PlaylistView::showInspector);
        menu.exec(event->globalPos());
    }
    else
#endif
    {
        QTableView::contextMenuEvent(event);
    }
}

void PlaylistView::showInspector()
{
    PlaylistModel *playlistModel = dynamic_cast<PlaylistModel *>(this->model());
    if (playlistModel == nullptr)
    {
        return;
    }

    QModelIndex i = this->currentIndex();
    if (i.isValid())
    {
        int row = i.row();
        const Song *s = playlistModel->getPlaylist()->getSong(row);

        SongInspector insp(s, this);
        insp.exec();
    }
}

int PlaylistView::sizeHintForColumn(int column) const
{
    if (column < this->colSizes.size() && this->colSizes[column] > 0)
    {
        return this->colSizes[column];
    }

    // should not happen
    return this->QTableView::sizeHintForColumn(column);
}

// try to stretch all columns to the full width, with respect to their content
void PlaylistView::resizeEvent(QResizeEvent *evt)
{
    int newWidth = evt->size().width();
    unsigned int colCount = this->colSizes.size();

    // assign each column width equally
    std::vector<unsigned int> freeCols, fullCols;
    for (unsigned int i = 0; i < colCount; i++)
    {
        int w = newWidth / colCount;

        // keep track how much free space there is avail for column and how much more space all columns need
        int free = w - this->QTableView::sizeHintForColumn(i);
        if (free >= 0) // this is a free column indeed
        {
            freeCols.push_back(i);
        }
        else
        {
            fullCols.push_back(i);
        }

        this->colSizes[i] = w;
    }

    // attempt to give full columns more width
    for (unsigned int i = 0; i < fullCols.size(); i++)
    {
        unsigned int fullColIdx = fullCols[i];
        int needed = this->QTableView::sizeHintForColumn(fullColIdx) - this->colSizes[fullColIdx];

        // go through free columns and steal width
        for (unsigned int j = 0; j < freeCols.size() && needed > 0; j++)
        {
            unsigned int freeColIdx = freeCols[j];
            int free = this->colSizes[freeColIdx] - this->QTableView::sizeHintForColumn(freeColIdx);
            free /= fullCols.size();

            free = min(free, needed);

            this->colSizes[fullColIdx] += free;
            this->colSizes[freeColIdx] -= free;
            needed -= free;
        }
    }

    int totalWidth = 0;
    for (unsigned int i = 0; i < colCount; i++)
    {
        totalWidth += max(this->colSizes[i], this->columnWidth(i));
    }

    int diff = totalWidth - newWidth;
    // are we still too wide?
    // make free columns even smaller
    for (unsigned int j = freeCols.size(); j > 0 && diff > 0; j--)
    {
        unsigned int freeColIdx = freeCols[j - 1];
        int free = max(0, this->colSizes[freeColIdx] - this->QTableView::sizeHintForColumn(freeColIdx));
        free = min(diff, free);

        this->colSizes[freeColIdx] -= free;
        diff -= free;
    }

    this->QTableView::resizeEvent(evt);
}
