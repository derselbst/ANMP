#ifndef SONGINSPECTOR_H
#define SONGINSPECTOR_H

#include <QDialog>

class Song;
struct SongInfo;

namespace Ui {
class SongInspector;
}

class SongInspector : public QDialog
{
    Q_OBJECT

public:
    explicit SongInspector(QWidget *parent = 0);
    ~SongInspector();

    void FillView(const Song* s);

private:
    Ui::SongInspector *ui;

    void fillGeneral(const Song* s);
    void fillMetadata(const SongInfo& m);
};

#endif // SONGINSPECTOR_H
