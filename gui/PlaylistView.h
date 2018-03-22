#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QTableView>
#include <vector>

class QKeyEvent;
class QResizeEvent;

class PlaylistView : public QTableView
{
    Q_OBJECT

    public:
    PlaylistView(QWidget *parent = 0);
    void setModel(QAbstractItemModel *model) override;

    protected:
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

    int sizeHintForColumn(int column) const override;
    void resizeEvent(QResizeEvent *evt) override;

    private:
    std::vector<int> colSizes;
    void moveItems(int steps);

    protected slots:
    void showInspector();
};


#endif
