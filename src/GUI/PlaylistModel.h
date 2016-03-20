#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QModelIndex>
#include <QAbstractTableModel>
#include <QColor>

#include "Playlist.h"

class Song;


class PlaylistModel : public QAbstractTableModel, public Playlist
{
  Q_OBJECT
  
public:
    PlaylistModel(QObject *parent = 0);
    
    // *** MODEL READ SUPPORT ***
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    
    
    // *** MODEL WRITE SUPPORT ***
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
    
    
    

    void add (Song* song) override;

    void remove (int i) override;

    void clear() override;

    Song* setCurrentSong (unsigned int id) override;

/*
    Song* getSong(unsigned int id) override;

    Song* current () override;*/

private:
  QColor calculateRowColor(int row) const;

};


#endif
