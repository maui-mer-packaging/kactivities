/*
 * Copyright (c) 2010, 2011, 2012 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef ACTIVITIES_CONSUMER_H
#define ACTIVITIES_CONSUMER_H

#include <QObject>
#include <QWidget>
#include <QString>
#include <QStringList>

#include "info.h"

#include <kurl.h>
#include "kactivities_export.h"

namespace KActivities {

class ConsumerPrivate;

/**
 * Contextual information can be, from the user's point of view, divided
 * into three aspects - "who am I?", "where am I?" (what are my surroundings?)
 * and "what am I doing?".
 *
 * Activities deal with the last one - "what am I doing?". The current activity
 * refers to what the user is doing at the moment, while the other activities represent
 * things that he/she was doing before, and probably will be doing again.
 *
 * Activity is an abstract concept whose meaning can differ from one user to another.
 * Typical examples of activities are "developing a KDE project", "studying the
 * 19th century art", "composing music", "lazing on a Sunday afternoon" etc.
 *
 * Every activity can have applications, documents, or other types of resources
 * assigned to it.
 *
 * Consumer provides an entry-level API for supporting activities in an
 * application - to react to the changes to the current activity as well as
 * registering the resources with its windows.
 *
 * Resource can be anything that is identifiable by an URI (for example,
 * a local file or a web page)
 *
 * @since 4.5
 */
class KACTIVITIES_EXPORT Consumer: public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentActivity READ currentActivity)
    Q_PROPERTY(QStringList activities READ listActivities)

public:
    /**
     * Different states of the activities service
     */
    enum ServiceStatus {
        NotRunning,        ///< Service is not running
        BareFunctionality, ///< @deprecated Service is running without a sane backend.
        FullFunctionality, ///< @deprecated Service is running, and a backend is available
        Running
    };

    explicit Consumer(QObject * parent = 0 /*nullptr*/);

    ~Consumer();

    /**
     * @returns the id of the current activity
     */
    QString currentActivity() const;

    /**
     * @returns the list of activities filtered by state
     * @param state state of the activity
     */
    QStringList listActivities(Info::State state) const;

    /**
     * @returns the list of all existing activities
     */
    QStringList listActivities() const;

    /**
     * @returns status of the activities service
     */
    static ServiceStatus serviceStatus();

    /**
     * Links a resource to the activity
     * @param uri URI of the resource
     * @param activityId id of the activity to link to. If empty, the
     *    resource is linked to the current activity.
     */
    void linkResourceToActivity(const QUrl & uri, const QString & activityId = QString());

    /**
     * Unlinks a resource from the activity
     * @param uri URI of the resource
     * @param activityId id of the activity to unlink from. If empty, the
     *    resource is unlinked from the current activity.
     */
    void unlinkResourceFromActivity(const QUrl & uri, const QString & activityId = QString());

    /**
     * @returns whether the resource is linket to the specified activity
     * @param uri URI of the resource
     * @param activityId id of the activity. If empty, the current activity is used.
     */
    bool isResourceLinkedToActivity(const QUrl & uri, const QString & activityId = QString()) const;

Q_SIGNALS:
    /**
     * This signal is emitted when the global
     * activity is changed
     * @param id id of the new current activity
     */
    void currentActivityChanged(const QString & id);

    /**
     * This signal is emitted when the activity service
     * goes online or offline
     * @param status new status of the service
     */
    void serviceStatusChanged(KActivities::Consumer::ServiceStatus status);

    /**
     * This signal is emitted when a new activity is added
     * @param id id of the new activity
     */
    void activityAdded(const QString & id);

    /**
     * This signal is emitted when the activity
     * is removed
     * @param id id of the removed activity
     */
    void activityRemoved(const QString & id);

private:
    ConsumerPrivate * const d;
};

} // namespace KActivities

#endif // ACTIVITIES_CONSUMER_H
