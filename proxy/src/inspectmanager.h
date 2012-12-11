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

#ifndef INSPECTMANAGER_H
#define INSPECTMANAGER_H

#include <QObject>

class InspectRequestPacket;
class InspectRequest;

class InspectManager : public QObject
{
	Q_OBJECT

public:
	InspectManager(QObject *parent = 0);
	~InspectManager();

	bool setSpec(const QString &spec);

	InspectRequest *createRequest();

private:
	class Private;
	Private *d;

	friend class InspectRequest;
	void write(const InspectRequestPacket &packet);
	void unlink(InspectRequest *req);
};

#endif
