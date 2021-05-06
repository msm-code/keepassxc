/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TagModel.h"

#include <QFont>

#include "core/Database.h"
#include "core/DatabaseIcons.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "keeshare/KeeShare.h"
#include <QDebug>

TagModel::TagModel(Group* g, QObject* parent)
    : QAbstractListModel(parent)
{
    setGroup(g);
}

void TagModel::setGroup(Group* g)
{

    m_group = g;
    auto entries = m_group->entries();
    beginResetModel();
    QSet<QString> s;
    for (auto entry : entries) {
        for (auto tag : entry->tags().split(";")) {
            if (tag != "") {
                s.insert(tag);
            }
        }
    }
    tagList = s.toList();
    endResetModel();
}

int TagModel::rowCount(const QModelIndex& parent) const
{
    (void)parent; // avoid unused warnings
    return tagList.size();
}

QVariant TagModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= tagList.size())
        return QVariant();

    if (role == Qt::DisplayRole)
        return tagList.at(index.row());
    else
        return QVariant();
}

QVariant TagModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
        return QString("Column %1").arg(section);
    else
        return QString("Row %1").arg(section);
}

QStringList& TagModel::tags()
{
    return tagList;
}