/****************************************************************************************
 * Copyright (c) 2003-2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2005-2013 Mark Kretschmann <kretschmann@kde.org>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#pragma once

#include "AnalyzerBase.h"

#include <QImage>
#include <QPixmap>
#include <QSharedPointer>
#include <QSize>

class QPalette;

class SpectrogramAnalyzer : public AnalyzerBase
{
public:
    SpectrogramAnalyzer(QWidget *);
    
    void transform(QVector<float> &s) override;
    
public slots:
    void connectSignals() override;

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent * event) override;
    void analyze(const QVector<float> &, uint32_t srate) override;
    
protected slots:
    void display(const QImage&);
    
private:

    std::atomic<int> m_xPos;
    std::atomic<int> m_currentWidth;
    QVector<float> m_scope, m_logScope; //so we don't create a vector every frame

    QImage m_spectrogram;
    QImage m_toBeDrawn;
};

