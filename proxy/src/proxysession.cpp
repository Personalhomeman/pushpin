/*
 * Copyright (C) 2012 Fan Out Networks, Inc.
 *
 * This file is part of Pushpin.
 *
 * Pushpin is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Pushpin is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "proxysession.h"

#include <stdio.h>
#include <QUrl>
#include "acceptdata.h"
#include "m2request.h"
#include "m2response.h"
#include "zurlmanager.h"
#include "zurlrequest.h"
#include "domainmap.h"
#include "requestsession.h"

class ProxySession::Private : public QObject
{
	Q_OBJECT

public:
	ProxySession *q;
	ZurlManager *zurlManager;
	DomainMap *domainMap;
	M2Request *mr;
	ZurlRequest *zr;
	bool firstRead;

	Private(ProxySession *_q, ZurlManager *_zurlManager, DomainMap *_domainMap) :
		QObject(_q),
		q(_q),
		zurlManager(_zurlManager),
		domainMap(_domainMap)
	{
	}

public slots:
	void mr_readyRead()
	{
		QByteArray buf = mr->read();
		printf("got chunk: %d\n", buf.size());
		zr->writeBody(buf);
	}

	void mr_finished()
	{
		printf("finished\n");
		zr->endBody();
	}

	void zr_readyRead()
	{
		printf("zr_readyRead\n");
		M2Response *resp = mr->createResponse();
		if(firstRead)
		{
			//HttpHeaders headers;
			//QByteArray str = "body.size=" + QByteArray::number(req->actualContentLength()) + '\n';
			//delete req;
			//headers += HttpHeader("Content-Length", QByteArray::number(str.length()));
			//resp->write(200, "OK", headers, str);
			//delete resp;
			HttpHeaders headers = zr->responseHeaders();

			if(headers.get("Content-Type") == "application/x-fo-instruct")
			{
				AcceptData ad;
				ad.rids += mr->rid();
				ad.request.method = mr->method();
				ad.request.path = mr->path();

				ad.haveResponse = true;
				ad.response.code = zr->responseCode();
				ad.response.status = zr->responseStatus();
				ad.response.headers = headers;
				ad.response.body = zr->readResponseBody(); // FIXME: need to keep reading until end

				delete resp;
				delete mr;
				delete zr;

				emit q->finishedForAccept(ad);
				return;
			}
			else
			{
				if(!headers.contains("Content-Length"))
					headers += HttpHeader("Transfer-Encoding", "chunked");
				resp->write(zr->responseCode(), zr->responseStatus(), headers, zr->readResponseBody());
				resp->write(QByteArray());
			}
		}
		else
			resp->write(zr->readResponseBody());

		delete resp;

		if(zr->isFinished())
		{
			delete mr;
			delete zr;

			emit q->finishedByPassthrough();
		}
	}

	void zr_bytesWritten(int count)
	{
		Q_UNUSED(count);
		printf("zr_bytesWritten\n");
	}

	void zr_error()
	{
		printf("zr_error\n");
	}
};

ProxySession::ProxySession(ZurlManager *zurlManager, DomainMap *domainMap, QObject *parent) :
	QObject(parent)
{
	d = new Private(this, zurlManager, domainMap);
}

ProxySession::~ProxySession()
{
	delete d;
}

void ProxySession::add(RequestSession *rs)
{
	d->mr = rs->request();
	connect(d->mr, SIGNAL(readyRead()), d, SLOT(mr_readyRead()));
	connect(d->mr, SIGNAL(finished()), d, SLOT(mr_finished()));

	d->zr = d->zurlManager->createRequest();
	connect(d->zr, SIGNAL(readyRead()), d, SLOT(zr_readyRead()));
	connect(d->zr, SIGNAL(bytesWritten(int)), d, SLOT(zr_bytesWritten(int)));
	connect(d->zr, SIGNAL(error()), d, SLOT(zr_error()));
	QString host = QString::fromUtf8(d->mr->headers().get("host"));
	int at = host.indexOf(':');
	if(at != -1)
		host = host.mid(0, at);
	QList<DomainMap::Target> targets = d->domainMap->entry(host);
	printf("%s has %d routes\n", qPrintable(host), targets.count());
	QByteArray str = "http://" + targets[0].first.toUtf8() + ':' + QByteArray::number(targets[0].second) + d->mr->path();
	//QUrl url(QByteArray("http://localhost:80/static/chat.js") /*+ req->path()*/);
	QUrl url(str);
	d->firstRead = true;
	d->zr->start(d->mr->method(), url, d->mr->headers());
}

#include "proxysession.moc"
