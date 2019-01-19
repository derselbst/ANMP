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
    m_fht->spectrum(&s.front());
}

// fill the buffer that will be fourier transformed
template<typename T>
void AnalyzerBase::prepareScope(const Song *s, frame_t playhead, QVector<float> &scope)
{
    const unsigned int nVoices = s->Format.Voices;
    const T *pcmBuf = static_cast<T *>(s->data) + (playhead * s->Format.Channels()) % s->count;

    unsigned int frame;
    for (frame = 0; (frame < gConfig.FramesToRender) && ((playhead + frame) < s->getFrames()); frame++)
    {
        /* init the frame'th element */
        float sampleItem = 0;

        for (unsigned int v = 0; v < nVoices; v++)
        {
            const uint16_t vchan = s->Format.VoiceChannels[v];
            if (!s->Format.VoiceIsMuted[v])
            {
                for (unsigned int m = 0; m < vchan; m++)
                {
                    sampleItem += float(pcmBuf[m]);
                }
            }

            // advance the pointer to point to next bunch of voice items
            pcmBuf += vchan;
        }

        //         /* Average between the channels */
        //         sampleItem /= s->Format.Channels();

        if (!std::is_floating_point<T>())
        {
            /* normalize the signal by dividing through the maximum value of the type PCMBUF points to */
            sampleItem /= std::numeric_limits<T>::max();
        }

        /* further attenuation */
        sampleItem /= 20;
        
        scope[frame] = sampleItem;
    }
    
    // workaround: the fft buffer may not be completely filled yet (e.g. end of song reached or seeked back to beginning).
    // we cannot FFT less frames than with what we've initialized FHT.
    // thus to avoid FFTing uninitialized PCM, fill up the rest of the buffer with zeros.
    for(;frame < gConfig.FramesToRender; frame++)
    {
        scope[frame]=0;
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
    analyze(m_fftData, s->Format.SampleRate);
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
    if(fps == 0)
    {
        m_renderTimer->stop();
        return;
    }
    
    m_renderTimer->setInterval(1000 / fps);
}


#include "AnalyzerBase.moc"
