/*
 * This file is part of buteo-sync-plugin-caldav package
 *
 * Copyright (C) 2013 Jolla Ltd. and/or its subsidiary(-ies).
 *
 * Contributors: Mani Chandrasekar <maninc@gmail.com>
 *               Stephan Rave <mail@stephanrave.de>
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

#include "report.h"
#include "reader.h"
#include "settings.h"

#include <QNetworkAccessManager>
#include <QBuffer>
#include <QDebug>
#include <QStringList>

#include <LogMacros.h>

static const QString DateTimeFormat = QStringLiteral("yyyyMMddTHHmmss");
static const QString DateTimeFormatUTC = DateTimeFormat + QStringLiteral("Z");

static QString dateTimeToString(const QDateTime &dt)
{
    if (dt.timeSpec() == Qt::UTC) {
        return QLocale::c().toString(dt, DateTimeFormatUTC);
    } else {
        return QLocale::c().toString(dt, DateTimeFormat);
    }
}

static QByteArray timeRangeFilterXml(const QDateTime &fromDateTime, const QDateTime &toDateTime)
{
    QByteArray xml;
    if (fromDateTime.isValid() || toDateTime.isValid()) {
        xml = "<c:comp-filter name=\"VEVENT\"> <c:time-range ";
        if (fromDateTime.isValid()) {
            xml += "start=\"" + dateTimeToString(fromDateTime) + "\" ";
        }
        if (toDateTime.isValid()) {
            xml += "end=\"" + dateTimeToString(toDateTime) + "\" ";
        }
        xml += " /></c:comp-filter>";

    }
    return xml;
}

Report::Report(QNetworkAccessManager *manager, Settings *settings, QObject *parent)
    : Request(manager, settings, "REPORT", parent)
{
    FUNCTION_CALL_TRACE;
}

void Report::getAllEvents(const QString &remoteCalendarPath, const QDateTime &fromDateTime, const QDateTime &toDateTime)
{
    FUNCTION_CALL_TRACE;
    sendCalendarQuery(remoteCalendarPath, fromDateTime, toDateTime, true);
}

void Report::getAllETags(const QString &remoteCalendarPath, const QDateTime &fromDateTime, const QDateTime &toDateTime)
{
    FUNCTION_CALL_TRACE;
    sendCalendarQuery(remoteCalendarPath, fromDateTime, toDateTime, false);
}

void Report::sendCalendarQuery(const QString &remoteCalendarPath,
                               const QDateTime &fromDateTime,
                               const QDateTime &toDateTime,
                               bool getCalendarData)
{
    FUNCTION_CALL_TRACE;
    QByteArray requestData = \
            "<c:calendar-query xmlns:d=\"DAV:\" xmlns:c=\"urn:ietf:params:xml:ns:caldav\">" \
                "<d:prop>" \
                    "<d:getetag />";
    if (getCalendarData) {
        requestData += \
                    "<c:calendar-data />";
    }
    requestData += \
                "</d:prop>"
                "<c:filter>" \
                    "<c:comp-filter name=\"VCALENDAR\">";
    if (fromDateTime.isValid() || toDateTime.isValid()) {
        requestData.append(timeRangeFilterXml(fromDateTime, toDateTime));
    }
    requestData += \
                    "</c:comp-filter>" \
                "</c:filter>" \
            "</c:calendar-query>";
    sendRequest(remoteCalendarPath, requestData);
}

void Report::multiGetEvents(const QString &remoteCalendarPath, const QStringList &eventHrefList)
{
    FUNCTION_CALL_TRACE;
    sendMultiQuery(remoteCalendarPath, eventHrefList, true);
 }

void Report::multiGetEtags(const QString &remoteCalendarPath, const QStringList &eventHrefList)
{
    FUNCTION_CALL_TRACE;
    sendMultiQuery(remoteCalendarPath, eventHrefList, false);
}

void Report::sendMultiQuery(const QString &remoteCalendarPath, const QStringList &eventHrefList, bool getCalendarData)
{
    FUNCTION_CALL_TRACE;
    if (eventHrefList.isEmpty()) {
        return;
    }

    QByteArray requestData = "<c:calendar-multiget xmlns:d=\"DAV:\" xmlns:c=\"urn:ietf:params:xml:ns:caldav\">" \
                             "<d:prop><d:getetag />";
    if (getCalendarData) {
        requestData += "<c:calendar-data />";
    }
    requestData += "</d:prop>";
    Q_FOREACH (const QString &eventHref , eventHrefList) {
        requestData.append("<d:href>");
        requestData.append(eventHref.toUtf8());
        requestData.append("</d:href>");
    }
    requestData.append("</c:calendar-multiget>");

    sendRequest(remoteCalendarPath, requestData);
}

void Report::sendRequest(const QString &remoteCalendarPath, const QByteArray &requestData)
{
    FUNCTION_CALL_TRACE;
    mRemoteCalendarPath = remoteCalendarPath;

    QNetworkRequest request;
    prepareRequest(&request, remoteCalendarPath);
    request.setRawHeader("Depth", "1");
    request.setRawHeader("Prefer", "return-minimal");
    request.setHeader(QNetworkRequest::ContentLengthHeader, requestData.length());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml; charset=utf-8");
    QBuffer *buffer = new QBuffer(this);
    buffer->setData(requestData);
    // TODO: when Qt5.8 is available, remove the use of buffer, and pass requestData directly.
    QNetworkReply *reply = mNAManager->sendCustomRequest(request, REQUEST_TYPE.toLatin1(), buffer);
    debugRequest(request, buffer->buffer());
    connect(reply, SIGNAL(finished()), this, SLOT(processResponse()));
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)),
            this, SLOT(slotSslErrors(QList<QSslError>)));
}

void Report::processResponse()
{
    FUNCTION_CALL_TRACE;

    LOG_DEBUG("Process REPORT response for server path" << mRemoteCalendarPath);

    if (wasDeleted()) {
        LOG_DEBUG("REPORT request was aborted");
        return;
    }

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        finishedWithInternalError();
        return;
    }
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        finishedWithReplyResult(reply->error());
        return;
    }
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (statusCode.isValid()) {
        int status = statusCode.toInt();
        if (status > 299) {
            finishedWithError(Buteo::SyncResults::INTERNAL_ERROR,
                              QString("Got error status response for REPORT: %1").arg(status));
            return;
        }
    }

    QByteArray data = reply->readAll();
    debugReply(*reply, data);

    if (!data.isNull() && !data.isEmpty()) {
        Reader reader;
        reader.read(data);
        if (reader.hasError()) {
            finishedWithError(Buteo::SyncResults::INTERNAL_ERROR, QString("Malformed response body for REPORT"));
        } else {
            mReceivedResources = reader.results();
            finishedWithSuccess();
        }
    } else {
        finishedWithError(Buteo::SyncResults::INTERNAL_ERROR, QString("Empty response body for REPORT"));
    }
}

const QList<Reader::CalendarResource>& Report::receivedCalendarResources() const
{
    return mReceivedResources;
}
