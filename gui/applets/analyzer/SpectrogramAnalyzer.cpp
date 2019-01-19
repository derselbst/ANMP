
#include "SpectrogramAnalyzer.h"

#include "PaletteHandler.h"

#include <cmath>

#include <QPainter>
#include <QResizeEvent>


static inline uint myMax(uint v1, uint v2)
{
    return v1 > v2 ? v1 : v2;
}

SpectrogramAnalyzer::SpectrogramAnalyzer(QWidget *parent)
: AnalyzerBase(parent), m_spectrogram(0,0,QImage::Format_Invalid)
{
    setObjectName("SpectrogramAnalyzer");
    this->setAttribute(Qt::WA_OpaquePaintEvent);
    
    setFps(0);
}


void SpectrogramAnalyzer::connectSignals()
{
    this->disconnectSignals();
}

void SpectrogramAnalyzer::display(const QImage& img)
{
    m_toBeDrawn = img.mirrored();
    this->update();
}

void SpectrogramAnalyzer::resizeEvent(QResizeEvent * event)
{
    int h = event->size().height(), w = event->size().width();
    
    m_scope.resize(h);
    m_logScope.resize(h);
    m_xPos = 0;
    
    m_spectrogram = QImage(w,h,QImage::Format_RGB32);
    m_spectrogram.fill(Qt::black);
    m_currentWidth = w;
    
    event->accept();
}


void SpectrogramAnalyzer::paintEvent(QPaintEvent* event)
{    
    QPainter painter(this);
    painter.drawImage(this->rect(), m_toBeDrawn);
    
    event->accept();
}

void SpectrogramAnalyzer::transform(QVector<float> &s)
{
    if (m_logScope.size() == 0 || s.size() == 0)
    {
        return;
    }

    m_fht->spectrum(/*&m_logScope.front(), */&s.front());
}

void SpectrogramAnalyzer::analyze(const QVector<float> &s, uint32_t srate)
{
    if (m_scope.size() == 0 || s.size() == 0)
    {
        return;
    }

    interpolate(s, m_scope);
    
    double w = 1 / sqrt(srate);
    auto scopeSize = m_scope.size();
    for (int y = 0; y < scopeSize/2; y++)
    {
        int a = 255*sqrt(w * /*sqrt*/(m_scope[y + 0] * m_scope[y + 0] /*+ m_scope[y + 1] * m_scope[y + 1]*/));
        int b = /*(nb_display_channels == 2 ) ? sqrt(w * hypot(data[1][2 * y + 0], data[1][2 * y + 1]))
                                            :*/ a;
        a = std::min(a, 255);
        b = std::min(b, 255);

        for (int i = 0; i < 2; i++)
        {
            m_spectrogram.setPixel(m_xPos, 2 * y + i, qRgb(a, b, (a + b) >> 1));
            m_spectrogram.setPixel(m_xPos+1, 2 * y + i, qRgb(a, b, (a + b) >> 1));
        }
    }
    ++m_xPos;
    if (++m_xPos >= m_currentWidth)
    {
        m_xPos = 0;
    }
    
    this->display(m_spectrogram);
}
