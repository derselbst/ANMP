#ifndef ANALYZERAPPLET_H
#define ANALYZERAPPLET_H

#include <QMainWindow>

namespace Ui {
class AnalyzerApplet;
}
class AnalyzerBase;
class Player;

class AnalyzerApplet : public QMainWindow
{
    Q_OBJECT

    enum AnalyzerType { Block, Ascii};

public:
    explicit AnalyzerApplet(Player* player, QWidget *parent = 0);
    ~AnalyzerApplet();

private:
    Ui::AnalyzerApplet *ui = nullptr;
    AnalyzerBase* analyzerWidget = nullptr;

    Player* player;

    void setCurrentAnalyzer( enum AnalyzerType& type );
    void newGeometry();
    void resizeEvent(QResizeEvent* event) override;

};

#endif // ANALYZERAPPLET_H
