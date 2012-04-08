/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011 - 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012, Jeff Mitchell <jeffe@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "FadingPixmap.h"

#include "utils/logger.h"

#include <QTimer>
#include <QBuffer>
#include <QPainter>

#define ANIMATION_TIME 1000

QWeakPointer< TomahawkUtils::SharedTimeLine > FadingPixmap::s_stlInstance = QWeakPointer< TomahawkUtils::SharedTimeLine >();

QWeakPointer< TomahawkUtils::SharedTimeLine >
FadingPixmap::stlInstance()
{
    if ( s_stlInstance.isNull() )
        s_stlInstance = QWeakPointer< TomahawkUtils::SharedTimeLine> ( new TomahawkUtils::SharedTimeLine() );

    return s_stlInstance;
}


FadingPixmap::FadingPixmap( QWidget* parent )
    : QLabel( parent )
    , m_oldPixmap( QPixmap() )
    , m_fadePct( 100 )
    , m_startFrame( 0 )
{
//    setCursor( Qt::PointingHandCursor );
}


FadingPixmap::~FadingPixmap()
{
}


void
FadingPixmap::onAnimationStep( int frame )
{
    m_fadePct = (float)( frame - m_startFrame ) / 10.0;
    if ( m_fadePct > 100.0 )
        m_fadePct = 100.0;
    
    repaint();

    if ( m_fadePct == 100.0 )
        QTimer::singleShot( 0, this, SLOT( onAnimationFinished() ) );
}


void
FadingPixmap::onAnimationFinished()
{
    m_oldPixmap = QPixmap();
    repaint();

    disconnect( stlInstance().data(), SIGNAL( frameChanged( int ) ), this, SLOT( onAnimationStep( int ) ) );
    
    if ( m_pixmapQueue.count() )
        setPixmap( m_pixmapQueue.takeFirst() );
}


void
FadingPixmap::setPixmap( const QPixmap& pixmap, bool clearQueue )
{
    QByteArray ba;
    QBuffer buffer( &ba );
    buffer.open( QIODevice::WriteOnly );
    pixmap.save( &buffer, "PNG" );
    QString newImageMd5 = TomahawkUtils::md5( buffer.data() );
    if ( m_oldImageMd5 == newImageMd5 )
        return;

    m_oldImageMd5 = newImageMd5;

    if ( !m_oldPixmap.isNull() )
    {
        if ( clearQueue )
            m_pixmapQueue.clear();

        m_pixmapQueue << pixmap;
        return;
    }

    m_oldPixmap = m_pixmap;
    m_pixmap = pixmap;

    stlInstance().data()->setUpdateInterval( 20 );
    m_startFrame = stlInstance().data()->currentFrame();
    m_fadePct = 0;
    connect( stlInstance().data(), SIGNAL( frameChanged( int ) ), this, SLOT( onAnimationStep( int ) ) );
}


void
FadingPixmap::mouseReleaseEvent( QMouseEvent* event )
{
    QFrame::mouseReleaseEvent( event );

    emit clicked();
}


void
FadingPixmap::paintEvent( QPaintEvent* event )
{
    Q_UNUSED( event );
    
    QPainter p( this );
    QRect r = contentsRect();

    p.save();
    p.setRenderHint( QPainter::Antialiasing );

    p.setOpacity( float( 100.0 - m_fadePct ) / 100.0 );
    p.drawPixmap( r, m_oldPixmap );

    p.setOpacity( float( m_fadePct ) / 100.0 );
    p.drawPixmap( r, m_pixmap );

    p.restore();
}
