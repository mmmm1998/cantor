/*
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301, USA.

    ---
    Copyright (C) 2012 Martin Kuettler <martin.kuettler@gmail.com>
 */

#include "worksheetimageitem.h"
#include "worksheet.h"

#include <QMovie>
#include <QImage>
#include <QGraphicsSceneContextMenuEvent>
#include <QUrl>
#include <QMenu>
#include <QDebug>

WorksheetImageItem::WorksheetImageItem(QGraphicsObject* parent)
    : QGraphicsObject(parent)
{
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    connect(this, SIGNAL(menuCreated(QMenu*,QPointF)), parent,
            SLOT(populateMenu(QMenu*,QPointF)), Qt::DirectConnection);
}

WorksheetImageItem::~WorksheetImageItem()
{
    if (worksheet())
        worksheet()->removeRequestedWidth(this);
}

int WorksheetImageItem::type() const
{
    return Type;
}

bool WorksheetImageItem::imageIsValid()
{
    return !m_pixmap.isNull();
}

qreal WorksheetImageItem::setGeometry(qreal x, qreal y, qreal w, bool centered)
{
    if (width() <= w && centered) {
        setPos(x + w/2 - width()/2, y);
    } else {
        setPos(x, y);
    }
    worksheet()->setRequestedWidth(this, scenePos().x() + width());

    return height();
}

qreal WorksheetImageItem::height() const
{
    return m_size.height();
}

qreal WorksheetImageItem::width() const
{
    return m_size.width();
}

QSizeF WorksheetImageItem::size()
{
    return m_size;
}

void WorksheetImageItem::setSize(QSizeF size)
{
    m_size = size;

    qreal width = scenePos().x() + size.width();
    worksheet()->setRequestedWidth(this, width);
}

QSize WorksheetImageItem::imageSize()
{
    return m_pixmap.size();
}

QRectF WorksheetImageItem::boundingRect() const
{
    return QRectF(QPointF(0, 0), m_size);
}

#include <QStyleOptionGraphicsItem>

void WorksheetImageItem::paint(QPainter *painter,
                               const QStyleOptionGraphicsItem *option,
                               QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->drawPixmap(QRectF(QPointF(0,0), m_size), m_pixmap,
                        m_pixmap.rect());
    if (hasFocus())
    {
        painter->setPen(Qt::DashLine);
        painter->drawRect(0, 0, width(), height());
    }
}

void WorksheetImageItem::setEps(const QUrl& url)
{
    const QImage img = worksheet()->renderer()->renderToImage(url, Cantor::Renderer::EPS, &m_size);
    m_pixmap = QPixmap::fromImage(img.convertToFormat(QImage::Format_ARGB32));
}

void WorksheetImageItem::setImage(QImage img)
{
    m_pixmap = QPixmap::fromImage(img);
    setSize(m_pixmap.size());
}

void WorksheetImageItem::setImage(QImage img, QSize displaySize)
{
    m_pixmap = QPixmap::fromImage(img);
    setSize(displaySize);
}

void WorksheetImageItem::setPixmap(QPixmap pixmap)
{
    m_pixmap = pixmap;
}

QPixmap WorksheetImageItem::pixmap() const
{
    return m_pixmap;
}

void WorksheetImageItem::populateMenu(QMenu* menu, QPointF pos)
{
    emit menuCreated(menu, mapToParent(pos));
}

void WorksheetImageItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu *menu = worksheet()->createContextMenu();
    populateMenu(menu, event->pos());

    menu->popup(event->screenPos());
}

Worksheet* WorksheetImageItem::worksheet()
{
    return qobject_cast<Worksheet*>(scene());
}



