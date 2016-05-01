/****************************************************************************************
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
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

#ifndef ANALYZERBASE_H
#define ANALYZERBASE_H

#ifdef __FreeBSD__
#include <sys/types.h>
#endif

#include "fht.h"     //stack allocated

#include <QGLWidget>

#include <vector>    //included for convenience


#include "types.h"

class Song;


class AnalyzerBase : public QGLWidget
{
    Q_OBJECT
public:

    ~AnalyzerBase();

protected:
    AnalyzerBase( QWidget* );

    void interpolate( const QVector<float>&, QVector<float>& ) const;

    virtual void transform( QVector<float>& );
    virtual void analyze( const QVector<float>& ) = 0;

    void setFps( int fps );

    FHT    *m_fht;
    QTimer *m_renderTimer;

public slots:
    void connectSignals();
    void disconnectSignals();
//     void currentDesktopChanged();
    void processData( const Song* thescope, frame_t playhead );
};


#endif
