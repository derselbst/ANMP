#ifndef ANALYZERAPPLET_H
#define ANALYZERAPPLET_H

#include <QMainWindow>
#include "types.h"

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
    
    void startGraphics();
    void stopGraphics();
    void setCurrentAnalyzer( enum AnalyzerType& type );
    

private:
    Ui::AnalyzerApplet *ui = nullptr;
    AnalyzerBase* analyzerWidget = nullptr;

    Player* player;

    void newGeometry();
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* e) override;
    
    static void redraw(void* ctx, frame_t pos);
    
};

#endif // ANALYZERAPPLET_H
