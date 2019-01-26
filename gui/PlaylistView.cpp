#include "PlaylistView.h"

#include "anmp.hpp"
#include "PlaylistModel.h"
#include "songinspector.h"

#include <QDesktopServices>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMenu>
#include <QResizeEvent>
#include <QtGlobal>

PlaylistView::PlaylistView(QWidget *parent)
: QTableView(parent), contextMenu(new QMenu(this))
{
    this->setAcceptDrops(true);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->setDragEnabled(true);
    this->setDragDropOverwriteMode(false);
    this->setDragDropMode(QAbstractItemView::DragDrop);
    this->setDefaultDropAction(Qt::MoveAction);
    this->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->setShowGrid(false);

    this->contextMenu->addAction(QIcon::fromTheme("help-contents"), "Details", this, &PlaylistView::showInspector);
    this->contextMenu->addAction(QIcon::fromTheme("system-file-manager"), "Open containing folder", this, &PlaylistView::openContainingFolder);
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
bool sortQItemSelectionDesc(const QItemSelectionRange& i, const QItemSelectionRange& j)
{
    return i.bottom() > j.bottom();
}

bool sortQItemSelectionAsc(const QItemSelectionRange& i, const QItemSelectionRange& j)
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
    
    QModelIndex bot = playlistModel->index(playlistModel->rowCount(QModelIndex()) - 1, 0);
    QModelIndex top = playlistModel->index(0, 0);

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

        case Qt::Key_Home:
        case Qt::Key_End:
            if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier)
            {
                QModelIndex cur = this->currentIndex();
                
                QItemSelectionRange aboveCur(top, cur);
                
                QItemSelection selection;
                selection.append(aboveCur);
                this->selectionModel()->select(selection,
                                               ((key == Qt::Key_Home) ? QItemSelectionModel::Select : QItemSelectionModel::Deselect) | QItemSelectionModel::Rows);
                selection.clear();
                
                QItemSelectionRange belowCur(cur, bot);
                selection.append(belowCur);
                this->selectionModel()->select(selection,
                                               ((key == Qt::Key_Home) ? QItemSelectionModel::Deselect : QItemSelectionModel::Select) | QItemSelectionModel::Rows);
                
                // make sure the current index is selected
                this->selectionModel()->select(this->currentIndex(),
                                               QItemSelectionModel::Select | QItemSelectionModel::Rows);
                
                break;
            }
            else if ((event->modifiers() & Qt::NoModifier) == Qt::NoModifier)
            {
                this->setCurrentIndex((key == Qt::Key_Home) ? top : bot);
                break;
            }
            else
            {
                [[fallthrough]];
            }

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
        this->contextMenu->exec(event->globalPos());
    }
    else
#endif
    {
        QTableView::contextMenuEvent(event);
    }
}

void PlaylistView::openContainingFolder()
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

        QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(::mydirname(s->Filename))));
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

        SongInspector *insp = new SongInspector(s, this);
        insp->setAttribute(Qt::WA_DeleteOnClose);
        insp->show();
    }
}

int PlaylistView::sizeHintForColumn(int column) const
{
    if (column >= 0 && static_cast<unsigned int>(column) < this->colSizes.size() && this->colSizes[column] > 0)
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
    std::vector<int> sizeHint;
    freeCols.reserve(colCount);
    fullCols.reserve(colCount);
    sizeHint.resize(colCount);    
    
    for (unsigned int i = 0; i < colCount; i++)
    {
        int w = newWidth / colCount;

        int hint = this->QTableView::sizeHintForColumn(i);
        // queue the size hint (calling it is expensive as the model get full)
        sizeHint[i] = hint;
        // keep track how much free space there is avail for column and how much more space all columns need
        int free = w - hint;
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
        int needed = sizeHint[fullColIdx] - this->colSizes[fullColIdx];

        // go through free columns and steal width
        for (unsigned int j = 0; j < freeCols.size() && needed > 0; j++)
        {
            unsigned int freeColIdx = freeCols[j];
            int free = this->colSizes[freeColIdx] - sizeHint[freeColIdx];
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
        int free = max(0, this->colSizes[freeColIdx] - sizeHint[freeColIdx]);
        free = min(diff, free);

        this->colSizes[freeColIdx] -= free;
        diff -= free;
    }

    this->QTableView::resizeEvent(evt);
}
