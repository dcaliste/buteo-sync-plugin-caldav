/*
 * This file is part of buteo-sync-plugin-caldav package
 *
 * Copyright (C) 2013 Jolla Ltd. and/or its subsidiary(-ies).
 *
 * Contributors: Mani Chandrasekar <maninc@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QUrl>

class Settings
{
public:
    Settings();

    QString authToken() const;
    void setAuthToken(const QString &token);

    void setUsername(const QString &username);
    QString username() const;

    void setPassword(const QString &password);
    QString password() const;

    void setIgnoreSSLErrors(bool ignore);
    bool ignoreSSLErrors() const;

    void setAccountId(quint32 accountId);
    quint32 accountId() const;

    void setServerAddress(const QString &serverAddress);
    QString serverAddress() const;

    void setUserPrincipal(const QString &href);
    QString userPrincipal() const;

    void setUserMailtoHref(const QString &href);
    QString userMailtoHref() const;

private:
    QString mUserPrincipal;
    QString mUserMailtoHref;
    QString mServerAddress;
    QString mOAuthToken;
    QString mUsername;
    QString mPassword;
    quint32 mAccountId;
    bool mIgnoreSSLErrors;
};

#endif // SETTINGS_H
