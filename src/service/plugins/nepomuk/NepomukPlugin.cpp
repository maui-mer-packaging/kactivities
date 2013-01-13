/*
 *   Copyright (C) 2011, 2012 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <config-features.h>

#include "NepomukPlugin.h"
#include "NepomukCommon.h"
#include "resourceslinkingadaptor.h"

#include "../../Event.h"

#include "kao.h"

#include <QDebug>
#include <QDBusConnection>

#include <Nepomuk2/Resource>
#include <Nepomuk2/ResourceManager>
#include <Nepomuk2/Variant>

#include <KDirNotify>

#include <utils/nullptr.h>
#include <utils/d_ptr_implementation.h>
#include <utils/val.h>

namespace Nepomuk = Nepomuk2;
using namespace KDE::Vocabulary;

class NepomukPlugin::Private {
public:
    Private()
      : nepomuk(nullptr),
        activities(nullptr),
        resourceScoring(nullptr),
        nepomukPresent(false)
    {
    }

    void syncActivities(const QStringList & activities = QStringList());

    template < typename Fn >
    void doWithActivity(const QString & _activity, Fn fn)
    {
        qDebug() << "Doing something sinister with nepomuk, is it here? " << nepomukPresent;

        if (!nepomukPresent) return;

        val currentActivity = Plugin::callOn <QString, Qt::DirectConnection> (activities, "CurrentActivity", "QString");
        val activity = _activity.isEmpty() ? currentActivity : _activity;

        qDebug() << "The current activity is: " << currentActivity
                 << "We need to set the linking for: " << activity;

        // JIC checking that the service hasn't returned an empty activity
        if (activity.isEmpty()) return;

        fn(activity);

        if (currentActivity == activity)
            org::kde::KDirNotify::emitFilesAdded("activities:/current");

        org::kde::KDirNotify::emitFilesAdded("activities:/" + activity);
    }

    Nepomuk::ResourceManager * nepomuk;
    QObject * activities;
    QObject * resourceScoring;
    bool nepomukPresent;

    static NepomukPlugin * s_instance;
    static QString protocol;
};

NepomukPlugin * NepomukPlugin::Private::s_instance = nullptr;
QString NepomukPlugin::Private::protocol = QLatin1String("activities:/");

NepomukPlugin::NepomukPlugin(QObject *parent, const QVariantList & args)
    : Plugin(parent)
{
    Q_UNUSED(args)
    Private::s_instance = this;

    setName("org.kde.ActivityManager.Nepomuk");

    new ResourcesLinkingAdaptor(this);

    QDBusConnection::sessionBus().registerObject("/ActivityManager/Resources/Linking", this);
    QDBusConnection::sessionBus().registerObject("/ActivityManager/Nepomuk", this);
}

NepomukPlugin::~NepomukPlugin()
{
    Private::s_instance = nullptr;
}

bool NepomukPlugin::init(const QHash < QString, QObject * > & modules)
{
    // Listening to changes in activities
    d->activities = modules["activities"];

    connect(d->activities, SIGNAL(ActivityAdded(QString)),
            this, SLOT(addActivity(QString)));

    connect(d->activities, SIGNAL(ActivityRemoved(QString)),
            this, SLOT(removeActivity(QString)));

    connect(d->activities, SIGNAL(ActivityNameChanged(QString, QString)),
            this, SLOT(setActivityName(QString, QString)));

    connect(d->activities, SIGNAL(ActivityIconChanged(QString, QString)),
            this, SLOT(setActivityIcon(QString, QString)));

    connect(d->activities, SIGNAL(CurrentActivityChanged(QString)),
            this, SLOT(setCurrentActivity(QString)));

    // Listening to resource score changes
    d->resourceScoring = modules["org.kde.ActivityManager.Resources.Scoring"];

    connect(d->resourceScoring, SIGNAL(resourceScoreUpdated(QString, QString, QString, double)),
            this, SLOT(resourceScoreUpdated(QString, QString, QString, double)));
    connect(d->resourceScoring, SIGNAL(recentStatsDeleted(QString, int, QString)),
            this, SLOT(deleteRecentStats(QString, int, QString)));
    connect(d->resourceScoring, SIGNAL(earlierStatsDeleted(QString, int)),
            this, SLOT(deleteEarlierStats(QString, int)));

    // Initializing nepomuk
    d->nepomuk = Nepomuk::ResourceManager::instance();
    d->nepomuk->init();

    connect(d->nepomuk, SIGNAL(nepomukSystemStarted()),
            this, SLOT(nepomukSystemStarted()));
    connect(d->nepomuk, SIGNAL(nepomukSystemStopped()),
            this, SLOT(nepomukSystemStopped()));

    d->nepomukPresent = d->nepomuk->initialized();

    d->syncActivities();

    return true;
}

NepomukPlugin * NepomukPlugin::self()
{
    return Private::s_instance;
}

void NepomukPlugin::setActivityName(const QString & activity, const QString & name)
{
    Q_ASSERT(!activity.isEmpty());
    Q_ASSERT(!name.isEmpty());

    if (d->nepomukPresent)
        activityResource(activity).setLabel(name);
}

void NepomukPlugin::setActivityIcon(const QString & activity, const QString & icon)
{
    Q_ASSERT(!activity.isEmpty());

    if (d->nepomukPresent && !icon.isEmpty())
        activityResource(activity).setSymbols(QStringList() << icon);
}

void NepomukPlugin::setCurrentActivity(const QString & activity)
{
    Q_UNUSED(activity);

    org::kde::KDirNotify::emitFilesAdded(Private::protocol + "current");
}

void NepomukPlugin::addActivity(const QString & activity)
{
    Q_ASSERT(!activity.isEmpty());

    d->syncActivities(QStringList() << activity);

    org::kde::KDirNotify::emitFilesAdded(Private::protocol);
    org::kde::KDirNotify::emitFilesAdded(Private::protocol + activity);
}

void NepomukPlugin::removeActivity(const QString & activity)
{
    Q_ASSERT(!activity.isEmpty());

    if (d->nepomukPresent)
        activityResource(activity).remove();

    org::kde::KDirNotify::emitFilesAdded(Private::protocol);
}


void NepomukPlugin::nepomukSystemStarted()
{
    if (d->nepomukPresent) return;
    d->nepomukPresent = true;
}

void NepomukPlugin::nepomukSystemStopped()
{
    d->nepomukPresent = false;
}

bool NepomukPlugin::isFeatureOperational(const QStringList & feature) const
{
    if (feature.size() && feature.first() == "linking")
        return d->nepomukPresent;

    return false;
}

bool NepomukPlugin::isFeatureEnabled(const QStringList & feature) const
{
    return false;
}

void NepomukPlugin::setFeatureEnabled(const QStringList & feature, bool value)
{
    Q_UNUSED(feature)
    Q_UNUSED(value)
}

QStringList NepomukPlugin::listFeatures(const QStringList & feature) const
{
    Q_UNUSED(feature)

    return QStringList() << "linking";
}

void NepomukPlugin::Private::syncActivities(const QStringList & activityIds)
{
    if (!nepomukPresent) return;

    // If we got an empty list, it means we should synchronize
    // all the activities known by the service
    foreach (val & activityId,
        (
            activityIds.isEmpty()
                ? Plugin::callOn <QStringList, Qt::DirectConnection> (activities, "ListActivities", "QStringList")
                : activityIds
        )
    ) {
        // Notifying KIO of the update
        org::kde::KDirNotify::emitFilesAdded("activities:/" + activityId);

        // Getting the activity info from the service
        val name = Plugin::callOnWithArgs <QString, Qt::DirectConnection>
                (activities, "ActivityName", "QString", Q_ARG(QString, activityId));
        val icon = Plugin::callOnWithArgs <QString, Qt::DirectConnection>
                (activities, "ActivityIcon", "QString", Q_ARG(QString, activityId));

        // Setting the nepomuk resource properties - id, name, icon
        auto resource = activityResource(activityId);
        resource.setProperty(KAO::activityIdentifier(), activityId);

        if (!name.isEmpty()) {
            resource.setLabel(name);
        }

        if (!icon.isEmpty()) {
            resource.setSymbols(QStringList() << icon);

        } else {
            // If there is no icon reported by the service, and we
            // have one in nepomuk, send it to the service
            val & symbols = resource.symbols();
            if (symbols.size() > 0) {
                Plugin::callOnWithArgs <QString, Qt::DirectConnection>
                    (activities, "SetActivityIcon", "QString", Q_ARG(QString, activityId), Q_ARG(QString, symbols.at(0)));
            }
        }
    }
}

void NepomukPlugin::resourceScoreUpdated(const QString & activity, const QString & client, const QString & resource, double score)
{
    if (!d->nepomukPresent) return;

    updateNepomukScore(activity, client, resource, score);
}

void NepomukPlugin::deleteRecentStats(const QString & activity, int count, const QString & what)
{
    // TODO: BLOCKER - implement deleting statistics from nepomuk
}

void NepomukPlugin::deleteEarlierStats(const QString & activity, int months)
{
    // TODO: BLOCKER - implement deleting statistics from nepomuk
}

void NepomukPlugin::LinkResourceToActivity(const QString & uri, const QString & activity)
{
    Q_ASSERT(!uri.isEmpty());

    d->doWithActivity(activity,
        [uri] (const QString & activity) {
            // linking the resource to the specified activity
            qDebug() << "Adding the triple " << activity << " isRelated " << uri;
            activityResource(activity).addIsRelated(Nepomuk::Resource(uri));
        });
}

void NepomukPlugin::UnlinkResourceFromActivity(const QString & uri, const QString & activity)
{
    Q_ASSERT(!uri.isEmpty());

    d->doWithActivity(activity,
        [uri] (const QString & activity) {
            // unlinking the resource from the specified activity
            qDebug() << "Removing the triple " << activity << " isRelated " << uri;
            activityResource(activity).removeProperty(NAO::isRelated(), Nepomuk::Resource(uri));
        });
}

bool NepomukPlugin::IsResourceLinkedToActivity(const QString & uri, const QString & _activity)
{
    Q_ASSERT(!uri.isEmpty());

    if (!d->nepomukPresent) return false;

    val activity = _activity.isEmpty()
            ? Plugin::callOn <QString, Qt::DirectConnection> (d->activities, "CurrentActivity", "QString")
            : _activity;

    static val _query = QString::fromLatin1(
            "select ?r where { "
                " ?a a kao:Activity . "
                " ?a nao:isRelated ?r . "
                " ?r nie:url %1 . "
                " ?a kao:activityIdentifier %2 "
            "}");

    val query = _query.arg(Soprano::Node::resourceToN3(uri))
                      .arg(Soprano::Node::literalToN3(activity));

    qDebug() << "So, this is the query that should get the linked resource " << query;

    Soprano::QueryResultIterator it
        = Nepomuk::ResourceManager::instance()->mainModel()->executeQuery(
                query, Soprano::Query::QueryLanguageSparql);

    val hasResults = it.next();
    it.close();

    return hasResults;
}

KAMD_EXPORT_PLUGIN(NepomukPlugin, "activitymanger_plugin_nepomuk")

