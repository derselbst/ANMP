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

#include <atomic>
#include <QImage>
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
    
    double getYForFrequency(double frequency, double minf, double maxf, bool logarithmic);
    
protected slots:
    void display(const QImage&);
    
private:

    int m_xPos{0};
    std::atomic<int> m_currentWidth{0};
    std::atomic<int> m_currentHeight{0};
    QVector<float> m_scope, m_logScope; //so we don't create a vector every frame

    QImage m_spectrogram;
    QImage m_toBeDrawn;
};

