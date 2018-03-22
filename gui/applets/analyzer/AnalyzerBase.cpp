/****************************************************************************************
 * Copyright (c) 2003 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2009 Martin Sandsmark <martin.sandsmark@kde.org>                       *
 * Copyright (c) 2013 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "AnalyzerBase.h"
#include "Config.h"
#include "Song.h"

#include <cmath> // interpolate()

// #include <KWindowSystem>

#include <QTimer>


// INSTRUCTIONS
// 1. reimplement analyze()
// 2. if you want to manipulate the scope, reimplement transform()


AnalyzerBase::AnalyzerBase(QWidget *parent)
: QGLWidget(parent), m_fht(new FHT(log2(gConfig.FramesToRender))), m_fftData(m_fht->size()), m_renderTimer(new QTimer(this))
{
    setFps(60); // Default unless changed by subclass

    connect(m_renderTimer, &QTimer::timeout, this, &QGLWidget::updateGL);

    //initialize openGL context before managing GL calls
    makeCurrent();

    connectSignals();
}


AnalyzerBase::~AnalyzerBase()
{
    delete m_fht;
}

void AnalyzerBase::connectSignals()
{
    if (m_renderTimer->isActive())
    {
        return;
    }

    m_renderTimer->start();
}

void AnalyzerBase::disconnectSignals()
{
    m_renderTimer->stop();
}

void AnalyzerBase::transform(QVector<float> &s)
{
    float *front = static_cast<float *>(&s.front());

    m_fht->spectrum(front);
}

// fill the buffer that will be fourier transformed
template<typename T>
void AnalyzerBase::prepareScope(const Song *s, frame_t playhead, QVector<float> &scope)
{
    const unsigned int nVoices = s->Format.Voices;
    const T *pcmBuf = static_cast<T *>(s->data) + playhead * s->Format.Channels();

    for (unsigned int frame = 0; (frame < gConfig.FramesToRender) && ((playhead + frame) < s->getFrames()); frame++)
    {
        /* init the frame'th element */
        scope[frame] = 0;

        for (unsigned int v = 0; v < nVoices; v++)
        {
            const uint16_t vchan = s->Format.VoiceChannels[v];
            if (!s->Format.VoiceIsMuted[v])
            {
                for (unsigned int m = 0; m < vchan; m++)
                {
                    scope[frame] += float(pcmBuf[m]);
                }
            }

            // advance the pointer to point to next bunch of voice items
            pcmBuf += vchan;
        }

        //         /* Average between the channels */
        //         scope[frame] /= s->Format.Channels();

        if (!std::is_floating_point<T>())
        {
            /* normalize the signal by dividing through the maximum value of the type PCMBUF points to */
            scope[frame] /= std::numeric_limits<T>::max();
        }

        /* further attenuation */
        scope[frame] /= 20;
    }
}

void AnalyzerBase::processData(const Song *s, frame_t playhead)
{
    if (s == nullptr)
    {
        return;
    }

    if (s->Format.SampleFormat == SampleFormat_t::int16)
    {
        this->prepareScope<int16_t>(s, playhead, m_fftData);
    }
    else if (s->Format.SampleFormat == SampleFormat_t::int32)
    {
        this->prepareScope<int32_t>(s, playhead, m_fftData);
    }
    else if (s->Format.SampleFormat == SampleFormat_t::float32)
    {
        this->prepareScope<float>(s, playhead, m_fftData);
    }

    transform(m_fftData);
    analyze(m_fftData);
}

void AnalyzerBase::interpolate(const QVector<float> &inVec, QVector<float> &outVec)
{
    double pos = 0.0;
    const double step = (double)(inVec.size() / 2) / outVec.size(); // upper half of is only aliased duplication

    for (int i = 0; i < outVec.size(); ++i, pos += step)
    {
        const double error = pos - std::floor(pos);
        const unsigned long offset = (unsigned long)pos;

        long indexLeft = offset + 0;

        if (indexLeft >= inVec.size())
        {
            indexLeft = inVec.size() - 1;
        }

        long indexRight = offset + 1;

        if (indexRight >= inVec.size())
        {
            indexRight = inVec.size() - 1;
        }

        outVec[i] = inVec[indexLeft] * (1.0 - error) +
                    inVec[indexRight] * error;
    }
}

void AnalyzerBase::setFps(int fps)
{
    m_renderTimer->setInterval(1000 / fps);
}


#include "AnalyzerBase.moc"
