/*
 *   Copyright (C) 2012, 2013, 2014 Ivan Cukic <ivan.cukic(at)kde.org>
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

// Self
#include "resourcemodel.h"

// Qt
#include <QByteArray>
#include <QDebug>
#include <QIcon>
#include <QModelIndex>
#include <QCoreApplication>
#include <QDBusInterface>
#include <QUuid>

// KDE
#include <klocalizedstring.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <ksharedconfig.h>
#include <kfileitem.h>

// STL and Boost
#include <mutex>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/filtered.hpp>

// Local
#include "utils/continue_with.h"
#include "utils/range.h"
#include "utils/continue_with.h"
#include "utils/dbusfuture_p.h"

#define ACTIVITY_COLUMN 0
#define AGENT_COLUMN    1
#define RESOURCE_COLUMN 2
#define UNKNOWN_COLUMN  3


using kamd::utils::continue_with;

namespace KActivities {
namespace Models {

class ResourceModel::LinkerService: public QDBusInterface {
private:
    LinkerService()
        : QDBusInterface("org.kde.ActivityManager",
                         "/ActivityManager/Resources/Linking",
                         "org.kde.ActivityManager.ResourcesLinking")
    {
    }

public:
    static std::shared_ptr<LinkerService> self()
    {
        static std::weak_ptr<LinkerService> s_instance;
        static std::mutex singleton;

        std::lock_guard<std::mutex> singleton_lock(singleton);

        auto result = s_instance.lock();

        if (s_instance.expired()) {
            result.reset(new LinkerService());
            s_instance = result;
        }

        return std::move(result);
    }

};

ResourceModel::ResourceModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_shownActivities(QStringLiteral(":current"))
    , m_shownAgents(QStringLiteral(":current"))
    , m_sorting{"/media/ivan/documents"}
    , m_linker(LinkerService::self())
    , m_config(KSharedConfig::openConfig("kactivitymanagerd-resourcelinkingrc")
        ->group("Order"))
{
    // TODO: What to do if the file does not exist?

    const QString databaseDir
        = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
          + QStringLiteral("/kactivitymanagerd/resources/");

    const QString databaseFile = databaseDir + QStringLiteral("database");

    m_database = QSqlDatabase::addDatabase(
        QStringLiteral("QSQLITE"),
        QStringLiteral("kactivities_db_resources"));

    m_database.setDatabaseName(databaseFile);

    m_database.open();

    m_databaseModel = new QSqlTableModel(parent, m_database);
    m_databaseModel->setTable("ResourceLink");
    m_databaseModel->select();

    setSourceModel(m_databaseModel);

    connect(&m_service, &KActivities::Consumer::currentActivityChanged,
            this, &ResourceModel::setCurrentActivity);

    connect(m_linker.get(), SIGNAL(ResourceLinkedToActivity(QString, QString, QString)),
            this, SLOT(resourceLinkedToActivity(QString, QString, QString)));
    connect(m_linker.get(), SIGNAL(ResourceUnlinkedFromActivity(QString, QString, QString)),
            this, SLOT(resourceUnlinkedFromActivity(QString, QString, QString)));

    setDynamicSortFilter(true);
    sort(0);
}

ResourceModel::~ResourceModel()
{
}

QVariant ResourceModel::dataForColumn(const QModelIndex &index, int column) const
{
    return m_databaseModel->data(index.sibling(index.row(), column),
                                 Qt::DisplayRole);
}

bool ResourceModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto leftResource = dataForColumn(left, RESOURCE_COLUMN).toString();
    const auto rightResource = dataForColumn(right, RESOURCE_COLUMN).toString();

    const bool hasLeft = m_sorting.contains(leftResource);
    const bool hasRight = m_sorting.contains(rightResource);

    return
        ( hasLeft && !hasRight) ? true :
        (!hasLeft &&  hasRight) ? false :
        ( hasLeft &&  hasRight) ? m_sorting.indexOf(leftResource) < m_sorting.indexOf(rightResource) :
        QString::compare(leftResource, rightResource, Qt::CaseInsensitive) < 0;
}

QHash<int, QByteArray> ResourceModel::roleNames() const
{
    return {
        { Qt::DisplayRole,    "display" },
        { Qt::DecorationRole, "decoration" },
        { ResourceRole,       "url" },
        { AgentRole,          "agent" },
        { ActivityRole,       "activity" }
    };
}

template <typename Validator>
inline QStringList validateList(
        const QString &values, Validator validator)
{
    using boost::adaptors::filtered;
    using kamd::utils::as_collection;

    auto result
        = as_collection<QStringList>(values.split(',') | filtered(validator));

    if (result.isEmpty()) {
        result.append(QStringLiteral(":current"));
    }

    return result;
}

void ResourceModel::setShownActivities(const QString &activities)
{
    m_shownActivities = validateList(activities, [&] (const QString &activity) {
        return
            activity == ":current" ||
            activity == ":any" ||
            activity == ":global" ||
            !QUuid(activity).isNull();
    });

    reloadData();
    emit shownActivitiesChanged();
}

void ResourceModel::setShownAgents(const QString &agents)
{
    m_shownAgents = validateList(agents, [&] (const QString &agent) {
        return
            agent == ":current" ||
            agent == ":any" ||
            agent == ":global" ||
            (!agent.isEmpty() && !agent.contains('\'') && !agent.contains('"'));
    });

    reloadData();
    emit shownAgentsChanged();
}

QString ResourceModel::shownActivities() const
{
    return m_shownActivities.join(',');
}

QString ResourceModel::shownAgents() const
{
    return m_shownAgents.join(',');
}

QString ResourceModel::activityToWhereClause(const QString &shownActivity) const
{
    return QStringLiteral(" OR usedActivity=") + (
        shownActivity == ":current" ? "'" + m_service.currentActivity() + "'" :
        shownActivity == ":any"     ? "usedActivity" :
        shownActivity == ":global"  ? "''" :
                                      "'" + shownActivity + "'"
    );
}

QString ResourceModel::agentToWhereClause(const QString &shownAgent) const
{
    return QStringLiteral(" OR initiatingAgent=") + (
        shownAgent == ":current" ? "'" + QCoreApplication::applicationName() + "'" :
        shownAgent == ":any"     ? "initiatingAgent" :
        shownAgent == ":global"  ? "''" :
                                   "'" + shownAgent + "'"
    );
}

void ResourceModel::reloadData()
{
    using boost::accumulate;
    using namespace kamd::utils;

    m_sorting = m_config.readEntry(m_shownAgents.first(), QStringList());
    qDebug() << "Order for reloading" << m_sorting;

    // Defining the transformation functions for generating the SQL WHERE clause
    // from the specified activity/agent. They also resolve the special values
    // like :current, :any and :global.

    auto activityToWhereClause = transformed(&ResourceModel::activityToWhereClause, this);
    auto agentToWhereClause = transformed(&ResourceModel::agentToWhereClause, this);

    // Generating the SQL WHERE part by concatenating the generated clauses.
    // The generated query will be in the form of '0 OR clause1 OR clause2 ...'

    const QString whereActivity =
        accumulate(m_shownActivities | activityToWhereClause, QStringLiteral("0"));

    const QString whereAgent =
        accumulate(m_shownAgents | agentToWhereClause, QStringLiteral("0"));

    m_databaseModel->setFilter('(' + whereActivity + ") AND (" + whereAgent + ')');
}

void ResourceModel::setCurrentActivity(const QString &activity)
{
    Q_UNUSED(activity)

    if (m_shownActivities.contains(":current")) {
        reloadData();
    }
}

QVariant ResourceModel::data(const QModelIndex &proxyIndex, int role) const
{
    auto index = mapToSource(proxyIndex);

    if (role == Qt::DisplayRole || role == Qt::DecorationRole) {
        auto url = dataForColumn(index, RESOURCE_COLUMN).toString();

        // TODO: Will probably need some more special handling -
        // for application:/ and a few more

        if (url.startsWith('/')) {
            url = QStringLiteral("file://") + url;
        }

        KFileItem file(url);

        return role == Qt::DisplayRole ? file.name() : file.iconName();
    }

    return dataForColumn(index,
            role == ResourceRole ? RESOURCE_COLUMN :
            role == AgentRole    ? AGENT_COLUMN :
            role == ActivityRole ? ACTIVITY_COLUMN :
                                   UNKNOWN_COLUMN
        );
}

void ResourceModel::linkResourceToActivity(const QString &resource,
                                           const QJSValue &callback)
{
    linkResourceToActivity(resource, m_shownActivities.first(), callback);
}

void ResourceModel::linkResourceToActivity(const QString &resource,
                                           const QString &activity,
                                           const QJSValue &callback)
{
    linkResourceToActivity(m_shownAgents.first(), resource, activity, callback);
}

void ResourceModel::linkResourceToActivity(const QString &agent,
                                           const QString &resource,
                                           const QString &activity,
                                           const QJSValue &callback)
{
    if (activity == ":any") {
        qDebug() << ":any is not a valid activity specification for linking";
        return;
    }

    kamd::utils::continue_with(
        DBusFuture::asyncCall<void>(m_linker.get(),
            QStringLiteral("LinkResourceToActivity"),
            agent, resource,
            activity == ":current" ? m_service.currentActivity() :
            activity == ":global"  ? "" : activity),
        callback);
}

void ResourceModel::unlinkResourceFromActivity(const QString &resource,
                                               const QJSValue &callback)
{
    unlinkResourceFromActivity(resource, m_shownActivities.first(), callback);
}

void ResourceModel::unlinkResourceFromActivity(const QString &resource,
                                               const QString &activity,
                                               const QJSValue &callback)
{
    unlinkResourceFromActivity(m_shownAgents.first(), resource, activity,
                               callback);
}

void ResourceModel::unlinkResourceFromActivity(const QString &agent,
                                               const QString &resource,
                                               const QString &activity,
                                               const QJSValue &callback)
{
    if (activity == ":any") {
        qDebug() << ":any is not a valid activity specification for linking";
        return;
    }

    kamd::utils::continue_with(
        DBusFuture::asyncCall<void>(m_linker.get(),
            QStringLiteral("UnlinkResourceFromActivity"),
            agent, resource,
            activity == ":current" ? m_service.currentActivity() :
            activity == ":global"  ? "" : activity),
        callback);
}

void ResourceModel::resourceLinkedToActivity(const QString &initiatingAgent,
                                             const QString &targettedResource,
                                             const QString &usedActivity)
{
    Q_UNUSED(targettedResource)

    auto matchingActivity = boost::find_if(m_shownActivities, [&] (const QString &shownActivity) {
        return
            // If the activity is not important
            shownActivity == ":any" ||
            // or we are listening for the changes for the current activity
            (shownActivity == ":current"
                 && usedActivity == m_service.currentActivity()) ||
            // or we want the globally linked resources
            (shownActivity == ":global" && usedActivity == "") ||
            // or we have a specific activity in mind
            shownActivity == usedActivity;
    });

    auto matchingAgent = boost::find_if(m_shownAgents, [&](const QString &shownAgent) {
        return
            // If the agent is not important
            shownAgent == ":any" ||
            // or we are listening for the changes for the current agent
            (shownAgent == ":current"
                && initiatingAgent == QCoreApplication::applicationName()) ||
            // or for links that are global, and not related to a specific agent
            (shownAgent == ":global" && initiatingAgent == "") ||
            // or we have a specific agent to listen for
            shownAgent == initiatingAgent;
    });

    if (matchingActivity != m_shownActivities.end()
        && matchingAgent != m_shownAgents.end()) {
        reloadData();
    }
}

void ResourceModel::resourceUnlinkedFromActivity(const QString &initiatingAgent,
                                                 const QString &targettedResource,
                                                 const QString &usedActivity)
{
    // These are the same at the moment
    resourceLinkedToActivity(initiatingAgent, targettedResource, usedActivity);
}

void ResourceModel::setOrder(const QStringList &resources)
{
    m_sorting = resources;
    m_config.writeEntry(m_shownAgents.first(), m_sorting);
    m_config.sync();
    qDebug() << "Order" << m_sorting;
    reloadData();
}

KConfigGroup ResourceModel::config() const
{
    return KSharedConfig::openConfig("kactivitymanagerd-resourcelinkingrc")
        ->group("Order");
}

int ResourceModel::count() const
{
    return QSortFilterProxyModel::rowCount();
}

QString ResourceModel::displayAt(int row) const
{
    return data(index(row, 0), Qt::DisplayRole).toString();
}

QString ResourceModel::resourceAt(int row) const
{
    return data(index(row, 0), ResourceRole).toString();
}


} // namespace Models
} // namespace KActivities

#include "resourcemodel.moc"