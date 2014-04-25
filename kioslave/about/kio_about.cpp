/*  This file is part of the KDE libraries

    Copyright (c) 2002 John Firebaugh <jfirebaugh@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kio_about.h"

using namespace KIO;

AboutProtocol::AboutProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
    : SlaveBase("about", pool_socket, app_socket)
{
}

AboutProtocol::~AboutProtocol()
{
}

void AboutProtocol::get( const QUrl& )
{
    QByteArray output;
    
    QTextStream os( &output, QIODevice::WriteOnly );
    os.setCodec( "ISO-8859-1" ); // In fact ASCII

    os << "<html><head><title>about:blank</title></head><body></body></html>";
    os.flush();

    mimeType("text/html");
    data( output );
    finished();
}

void AboutProtocol::mimetype( const QUrl& )
{
    mimeType("text/html");
    finished();
}

extern "C" Q_DECL_EXPORT int kdemain( int argc, char **argv )
{

        if (argc != 4)
        {
            fprintf(stderr, "Usage: kio_about protocol domain-socket1 domain-socket2\n");
            exit(-1);
        }

        AboutProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        return 0;
    }

