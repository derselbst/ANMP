
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
: AnalyzerBase(parent), m_spectrogram(1,1,QImage::Format_RGB32)
{
    m_spectrogram.fill(Qt::black);
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
    m_toBeDrawn = img;
    this->update();
}

void SpectrogramAnalyzer::resizeEvent(QResizeEvent * event)
{
    m_currentHeight = event->size().height();
    m_currentWidth = event->size().width();

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
    auto currentH = m_currentHeight.load(),
    currentW = m_currentWidth.load();

    if (currentH == 0 || s.size() == 0)
    {
        return;
    }

    m_scope.resize(currentH);
    m_logScope.resize(currentH);

    m_spectrogram = m_spectrogram.scaled(currentW, currentH);

    interpolate(s, m_scope);

    const int xStepWidth = static_cast<int>(pow(2.0f, 44100.f/srate + 1));

    QPainter painter(&m_spectrogram);

    double w = 1 / sqrt(srate);
    int yTargetOld = m_scope.size();
    auto scopeSize = yTargetOld/2;
    for (int y = 1; y < scopeSize; y++)
    {
        int a = 255*sqrt(w * sqrt(m_scope[y + 0] * m_scope[y + 0] + m_scope[y + scopeSize] * m_scope[y + scopeSize]));
        int b = /*(nb_display_channels == 2 ) ? sqrt(w * hypot(data[1][2 * y + 0], data[1][2 * y + 1]))
                                            :*/ a;
        a = std::min(a, 255);
        b = std::min(b, 255);

        int aPrev = 255*sqrt(w * sqrt(m_scope[y - 1] * m_scope[y - 1] + m_scope[y-1 + scopeSize] * m_scope[y-1 + scopeSize]));
        aPrev = std::min(aPrev, 255);

        int yTarget = this->getYForFrequency(srate/2.0 * (y)/scopeSize, 1.0/scopeSize * srate/2.0, srate/2.0, true);

        for(auto yy=yTarget; yy<yTargetOld; yy++)
        {
            double fraction = (yy-yTarget) * 1.0 / (yTargetOld-yTarget);
            int r = (aPrev - a) * fraction + a;
            painter.fillRect(m_xPos, yy, xStepWidth, 1, QColor(qRgb(r, r, (r+r) >> 1)));
        }

        yTargetOld = yTarget;
    }

    m_xPos += xStepWidth;
    if (m_xPos >= currentW)
    {
        m_xPos = 0;
    }

    this->display(m_spectrogram);
}


double SpectrogramAnalyzer::getYForFrequency(double frequency, double minf, double maxf, bool logarithmic)
{
    int h = this->height();

    if (logarithmic) {

        static double lastminf = 0.0, lastmaxf = 0.0;
        static double logminf = 0.0, logmaxf = 0.0;

        if (lastminf != minf) {
            lastminf = (minf == 0.0 ? 1.0 : minf);
            logminf = std::log10(minf);
        }
        if (lastmaxf != maxf) {
            lastmaxf = (maxf < lastminf ? lastminf : maxf);
            logmaxf = std::log10(maxf);
        }

        if (logminf == logmaxf)
        {
            return 0;
        }
        return h - (h * (std::log10(frequency) - logminf)) / (logmaxf - logminf);

    } else {

        if (minf == maxf)
        {
            return 0;
        }
        return h - (h * (frequency - minf)) / (maxf - minf);
    }
}
