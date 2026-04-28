
#include "SpectrogramAnalyzer.h"

#include "PaletteHandler.h"
#include "fht.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <QAction>
#include <QActionGroup>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QResizeEvent>


SpectrogramAnalyzer::SpectrogramAnalyzer(QWidget *parent)
: AnalyzerBase(parent), m_spectrogram(1,1,QImage::Format_RGB32)
{
    m_spectrogram.fill(Qt::black);
    setObjectName("SpectrogramAnalyzer");
    this->setAttribute(Qt::WA_OpaquePaintEvent);
    
    setFps(0);
}

SpectrogramAnalyzer::~SpectrogramAnalyzer()
{
    delete m_windowFht;
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
    const int newWidth = event->size().width();
    const int oldWidth = m_currentWidth.load();

    // Keep xPos proportional so the drawing cursor doesn't jump or go out of bounds
    if (oldWidth > 0 && newWidth != oldWidth)
    {
        m_xPos = static_cast<int>(static_cast<float>(m_xPos) * newWidth / oldWidth + 0.5f);
    }

    m_currentHeight = event->size().height();
    m_currentWidth = newWidth;

    AnalyzerBase::resizeEvent(event);
}


void SpectrogramAnalyzer::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.drawImage(this->rect(), m_toBeDrawn);

    event->accept();
}

void SpectrogramAnalyzer::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    // --- Frequency Scale ---
    QMenu *scaleMenu = menu.addMenu(tr("Frequency Scale"));
    QActionGroup *scaleGroup = new QActionGroup(scaleMenu);
    scaleGroup->setExclusive(true);

    QAction *logAction = scaleMenu->addAction(tr("Logarithmic"));
    logAction->setCheckable(true);
    logAction->setChecked(m_freqScale.load() == Logarithmic);
    scaleGroup->addAction(logAction);

    QAction *melAction = scaleMenu->addAction(tr("Mel"));
    melAction->setCheckable(true);
    melAction->setChecked(m_freqScale.load() == Mel);
    scaleGroup->addAction(melAction);

    connect(logAction, &QAction::triggered, this, [this]() {
        m_freqScale.store(Logarithmic);
    });
    connect(melAction, &QAction::triggered, this, [this]() {
        m_freqScale.store(Mel);
    });

    // --- Window Size ---
    QMenu *windowMenu = menu.addMenu(tr("Window Size"));
    QActionGroup *windowGroup = new QActionGroup(windowMenu);
    windowGroup->setExclusive(true);

    const int currentWindowSize = m_windowSize.load();
    for (int size : {512, 1024, 2048, 4096, 8192, 16384})
    {
        QAction *act = windowMenu->addAction(QString::number(size));
        act->setCheckable(true);
        act->setChecked(currentWindowSize == size);
        windowGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, size]() {
            m_windowSize.store(size);
        });
    }

    // --- Scrolling mode ---
    menu.addSeparator();
    QAction *scrollAction = menu.addAction(tr("Scrolling"));
    scrollAction->setCheckable(true);
    scrollAction->setChecked(m_scrolling.load());
    connect(scrollAction, &QAction::triggered, this, [this](bool checked) {
        m_scrolling.store(checked);
    });

    // --- Speed ---
    QMenu *speedMenu = menu.addMenu(tr("Speed"));
    QActionGroup *speedGroup = new QActionGroup(speedMenu);
    speedGroup->setExclusive(true);

    const int currentSpeed = m_speed.load();
    const struct { const char *label; Speed value; } speedItems[] = {
        { QT_TR_NOOP("Slow"),   Slow   },
        { QT_TR_NOOP("Normal"), Normal },
        { QT_TR_NOOP("Fast"),   Fast   },
    };
    for (const auto &item : speedItems)
    {
        QAction *act = speedMenu->addAction(tr(item.label));
        act->setCheckable(true);
        act->setChecked(currentSpeed == static_cast<int>(item.value));
        speedGroup->addAction(act);
        const Speed sv = item.value;
        connect(act, &QAction::triggered, this, [this, sv]() {
            m_speed.store(static_cast<int>(sv));
        });
    }

    menu.exec(event->globalPos());
}

void SpectrogramAnalyzer::clearSpectrogram()
{
    const auto currentH = m_currentHeight.load();
    const auto currentW = m_currentWidth.load();
    if (currentH > 0 && currentW > 0)
    {
        m_spectrogram = QImage(currentW, currentH, QImage::Format_RGB32);
        m_spectrogram.fill(Qt::black);
    }
    m_xPos = 0;
}

void SpectrogramAnalyzer::transform(QVector<float> &s)
{
    if (s.size() == 0)
    {
        return;
    }

    const int wsize = m_windowSize.load();

    // Rebuild local FHT when window size changes
    if (!m_windowFht || m_windowFht->size() != wsize)
    {
        delete m_windowFht;
        m_windowFht = new FHT(static_cast<int>(std::log2(wsize)));
    }

    m_windowBuf.resize(wsize);
    const int copySize = std::min(static_cast<int>(s.size()), wsize);
    std::copy(s.begin(), s.begin() + copySize, m_windowBuf.begin());
    std::fill(m_windowBuf.begin() + copySize, m_windowBuf.end(), 0.0f);

    m_windowFht->spectrum(m_windowBuf.data());
}

void SpectrogramAnalyzer::analyze(const QVector<float> &/*s*/, uint32_t srate)
{
    auto currentH = m_currentHeight.load(),
    currentW = m_currentWidth.load();

    if (currentH == 0 || m_windowBuf.size() == 0)
    {
        return;
    }

    m_scope.resize(currentH);

    if (m_spectrogram.width() != currentW || m_spectrogram.height() != currentH)
    {
        m_spectrogram = m_spectrogram.scaled(currentW, currentH);
    }

    interpolate(m_windowBuf, m_scope);

    const int xStepWidth = [&]() {
        const int base = static_cast<int>(pow(2.0f, 44100.f / srate + 1));
        switch (static_cast<Speed>(m_speed.load()))
        {
            case Slow:   return std::max(1, base / 2); // minimum 1 pixel per frame
            case Fast:   return base * 2;
            default:     return base;
        }
    }();
    const bool scrolling = m_scrolling.load();

    // In scrolling mode, shift the existing image left before drawing the new column
    if (scrolling && xStepWidth > 0 && xStepWidth < currentW)
    {
        for (int row = 0; row < currentH; row++)
        {
            auto *rowData = reinterpret_cast<uint32_t *>(m_spectrogram.scanLine(row));
            std::memmove(rowData, rowData + xStepWidth,
                         static_cast<std::size_t>(currentW - xStepWidth) * sizeof(uint32_t));
            for (int x = currentW - xStepWidth; x < currentW; x++)
            {
                rowData[x] = 0xFF000000u; // opaque black
            }
        }
        m_xPos = currentW - xStepWidth;
    }

    QPainter painter(&m_spectrogram);

    const FrequencyScale scale = static_cast<FrequencyScale>(m_freqScale.load());
    const double w = 1.0 / std::sqrt(static_cast<double>(srate));
    int yTargetOld = m_scope.size();
    const int scopeSize = yTargetOld / 2;

    for (int y = 1; y < scopeSize; y++)
    {
        int a = static_cast<int>(255 * std::sqrt(w * std::sqrt(
            m_scope[y] * m_scope[y] + m_scope[y + scopeSize] * m_scope[y + scopeSize])));
        a = std::min(a, 255);

        int aPrev = static_cast<int>(255 * std::sqrt(w * std::sqrt(
            m_scope[y - 1] * m_scope[y - 1] +
            m_scope[y - 1 + scopeSize] * m_scope[y - 1 + scopeSize])));
        aPrev = std::min(aPrev, 255);

        int yTarget = static_cast<int>(this->getYForFrequency(
            srate / 2.0 * y / scopeSize,
            1.0 / scopeSize * srate / 2.0,
            srate / 2.0,
            scale));

        for (int yy = yTarget; yy < yTargetOld; yy++)
        {
            const double fraction = (yy - yTarget) * 1.0 / (yTargetOld - yTarget);
            const int r = static_cast<int>((aPrev - a) * fraction + a);
            painter.fillRect(m_xPos, yy, xStepWidth, 1, QColor(qRgb(r, r, (r + r) >> 1)));
        }

        yTargetOld = yTarget;
    }

    if (!scrolling)
    {
        m_xPos += xStepWidth;
        if (m_xPos >= currentW)
        {
            m_xPos = 0;
        }
    }

    this->display(m_spectrogram);
}


double SpectrogramAnalyzer::getYForFrequency(double frequency, double minf, double maxf, FrequencyScale scale)
{
    const int h = this->height();

    if (scale == Logarithmic)
    {
        const double safeMinf = (minf <= 0.0 ? 1.0 : minf);
        const double safeMaxf = (maxf < safeMinf ? safeMinf : maxf);
        const double logminf = std::log10(safeMinf);
        const double logmaxf = std::log10(safeMaxf);

        if (logminf == logmaxf)
        {
            return 0;
        }
        return h - (h * (std::log10(frequency) - logminf)) / (logmaxf - logminf);
    }
    else // Mel
    {
        auto toMel = [](double f) { return 2595.0 * std::log10(1.0 + f / 700.0); };
        const double melMin = toMel(minf > 0.0 ? minf : 1.0);
        const double melMax = toMel(maxf);

        if (melMin == melMax)
        {
            return 0;
        }
        return h - (h * (toMel(frequency) - melMin)) / (melMax - melMin);
    }
}
