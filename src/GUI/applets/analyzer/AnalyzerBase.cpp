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


AnalyzerBase::AnalyzerBase( QWidget *parent )
    : QGLWidget( parent )
    , m_fht( new FHT( log2( Config::FramesToRender ) ) )
    , m_renderTimer( new QTimer( this ) )
{
    setFps( 60 ); // Default unless changed by subclass

    connect( m_renderTimer, SIGNAL( timeout() ), this, SLOT( updateGL() ) );

    //initialize openGL context before managing GL calls
    makeCurrent();

    connectSignals();
}


AnalyzerBase::~AnalyzerBase()
{
    delete m_fht;
}

void
AnalyzerBase::connectSignals()
{
    if( m_renderTimer->isActive() )
        return;
    
    m_renderTimer->start();
}

void
AnalyzerBase::disconnectSignals()
{
    m_renderTimer->stop();
}

// TODO this seems to be quite usefull
// void
// AnalyzerBase::currentDesktopChanged()
// {
//     // Optimization for X11/Linux desktops:
//     // Don't update the analyzer if Amarok is not on the active virtual desktop.
// 
//     if( The::mainWindow()->isOnCurrentDesktop() )
//         connectSignals();
//     else
//         disconnectSignals();
// }


void AnalyzerBase::transform( QVector<float> &scope ) //virtual
{
    //this is a standard transformation that should give
    //an FFT scope that has bands for pretty analyzers

    float *front = static_cast<float*>( &scope.front() );

    float* f = new float[ m_fht->size() ];
    m_fht->copy( &f[0], front );
    m_fht->logSpectrum( front, &f[0] );
    m_fht->scale( front, 1.0 / 20 );

    scope.resize( m_fht->size() / 2 ); //second half of values are rubbish
    delete [] f;
}

void AnalyzerBase::processData( const Song* s, frame_t playhead )
{
  if(s==nullptr)
  {
    return;
  }
  
  QVector<float> scope(m_fht->size());
  if(s->Format.SampleFormat != int16)
  {
    int16_t* pcmBuf = static_cast<int16_t*>(s->data) + playhead * s->Format.Channels;
  
    for( unsigned int x = 0,i = 0; x < Config::FramesToRender*s->Format.Channels && playhead + i < s->getFrames(); x+=s->Format.Channels,i++ )
    {
        if( s->Format.Channels == 1 )  // Mono
        {
            scope[i] = double( pcmBuf[x] );
        }
        else    // Anything > Mono is treated as Stereo
        {
            scope[i] = double( pcmBuf[x] + pcmBuf[x+1] ) / ( 2 * ( 1 << 15 ) ); // Average between the channels
        }
        
        // attenuate the signal
        scope[i] /= 10;
    }
  }
  else
  {
        int32_t* pcmBuf = static_cast<int32_t*>(s->data) + playhead * s->Format.Channels;
  
    for( unsigned int x = 0,i = 0; x < Config::FramesToRender*s->Format.Channels && playhead + i < s->getFrames(); x+=s->Format.Channels,i++ )
    {
        if( s->Format.Channels == 1 )  // Mono
        {
            scope[i] = double( pcmBuf[x] >> 15 );
        }
        else    // Anything > Mono is treated as Stereo
        {
            scope[i] = double( (pcmBuf[x] + pcmBuf[x+1]) >> 15 ) / ( 2 * ( 1 << 15 ) ); // Average between the channels
        }
        
        // attenuate the signal
        scope[i] /= 10;
    }
  }
    transform( scope );
    analyze( scope );
}

void AnalyzerBase::interpolate( const QVector<float> &inVec, QVector<float> &outVec ) const
{
    double pos = 0.0;
    const double step = ( double )inVec.size() / outVec.size();

    for( int i = 0; i < outVec.size(); ++i, pos += step )
    {
        const double error = pos - std::floor( pos );
        const unsigned long offset = ( unsigned long )pos;

        long indexLeft = offset + 0;

        if( indexLeft >= inVec.size() )
            indexLeft = inVec.size() - 1;

        long indexRight = offset + 1;

        if( indexRight >= inVec.size() )
            indexRight = inVec.size() - 1;

        outVec[i] = inVec[indexLeft ] * ( 1.0 - error ) +
                    inVec[indexRight] * error;
    }
}

void AnalyzerBase::setFps( int fps )
{
    m_renderTimer->setInterval( 1000 / fps );
}


#include "AnalyzerBase.moc"
