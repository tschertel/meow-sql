#include "connections_manager.h"
#include "connection.h"
#include "user_query/user_query.h"
#include "db/entity/table_entity.h"
#include "db/entity/database_entity.h"
#include "helpers/logger.h"

namespace meow {
namespace db {

ConnectionsManager::ConnectionsManager()
    :Entity(),
     _connections(),
     _activeEntity(),
     _activeSession(nullptr),
     _userQueries()
{

}

ConnectionsManager::~ConnectionsManager()
{
    qDeleteAll(_connections);
    qDeleteAll(_userQueries);
}

ConnectionPtr ConnectionsManager::openDBConnection(db::ConnectionParameters & params)
{
    ConnectionPtr connection = params.createConnection();

    connection->setActive(true);

    SessionEntity * newSession = new SessionEntity(connection, this);

    QObject::connect(newSession,
            &meow::db::SessionEntity::entityEdited,
            this,
            &meow::db::ConnectionsManager::onEntityEdited);

    QObject::connect(newSession,
            &meow::db::SessionEntity::entityInserted,
            this,
            &meow::db::ConnectionsManager::onEntityInserted);

    QObject::connect(newSession->connection(),
            &meow::db::Connection::databaseChanged,
            this,
            &meow::db::ConnectionsManager::activeDatabaseChanged);

    _connections.push_back(newSession);

    emit connectionOpened(newSession);

    return connection;
}

bool ConnectionsManager::closeActiveConnection()
{
    SessionEntity * session = activeSession();
    if (session) {
        emit beforeConnectionClosed(session);
        _connections.removeOne(session);
        setActiveSession(nullptr);
        delete session;
        return true;
    }
    return false;
}

int ConnectionsManager::childCount() // override
{
    return _connections.size();
}

SessionEntity * ConnectionsManager::child(int row) // override
{
    return _connections.value(row); // returns null if out of bounds
}

int ConnectionsManager::indexOf(SessionEntity * session) const
{
    return _connections.indexOf(session);
}

Connection * ConnectionsManager::activeConnection() const
{
    if (_activeSession) {
        return _activeSession->connection();
    }
    return nullptr;
}

UserQuery * ConnectionsManager::userQueryAt(size_t index)
{
    if (index + 1 > _userQueries.size()) {
        _userQueries.resize(index + 1, nullptr);
    }
    if (!_userQueries[index]) {
        _userQueries[index] = new UserQuery(this);
    }
    return _userQueries[index];
}

void ConnectionsManager::createEntity(Entity::Type type)
{
    Entity * currentEntity = _activeEntity.currentEntity();
    if (!currentEntity) return;

    DataBaseEntity * databaseEntity =
        static_cast<DataBaseEntity *>(
            db::findParentEntityOfType(currentEntity, Entity::Type::Database)
    );
    if (!databaseEntity) return;

    if (type == Entity::Type::Table) {
        TableEntity * table = new TableEntity("", databaseEntity);
        table->setIsNew(true);
        // TODO move this code into connection ?
        table->setEngine(table->connection()->defaultTableEngine());
        _activeEntity.setCurrentEntity(nullptr);
        emit creatingNewEntity(table);
        //emit activeEntityChanged(table);
    }
}

void ConnectionsManager::refreshActiveSession()
{
    if (_activeSession) {
        _activeSession->refreshAllEntities();
        emit activeSessionRefreshed();
        setActiveEntity(nullptr);
    }
}

bool ConnectionsManager::dropActiveEntity()
{
    Entity * activeEntity = _activeEntity.currentEntity();
    if (!activeEntity) return false;

    if (_activeSession) {
        if ( (int)activeEntity->type() >= (int)Entity::Type::Table) {
            EntityInDatabase * entityDbLvl
                = static_cast<EntityInDatabase *>(activeEntity);
            return _activeSession->dropEntityInDB(entityDbLvl);
        } else if (activeEntity->type() == Entity::Type::Database) {
            DataBaseEntity * database
                 = static_cast<DataBaseEntity *>(activeEntity);
            return _activeSession->dropDatabase(database);
        } else {
            Q_ASSERT(0);
        }
    }
    return false;
}

QString ConnectionsManager::activeEntityPath() const
{
    QStringList path;

    Entity * entity = _activeEntity.currentEntity();

    while (entity != nullptr) {
        path.prepend(entity->name());
        entity = entity->parent();
    }

    return path.join('/');
}

void ConnectionsManager::setActiveEntity(Entity * activeEntity, bool select)
{
    bool changed = _activeEntity.setCurrentEntity(activeEntity);
    if (changed) {

        if (activeEntity) {
            if (_activeEntity.connectionChanged()) {
                setActiveSession(db::sessionForEntity(activeEntity));
            }
        }

        Connection * connection = activeConnection();

        if (connection && activeEntity) {
            if (_activeEntity.databaseChanged()) {
                QString dbName = databaseName(activeEntity);
                if (dbName.length()) {
                    connection->setDatabase(dbName);
                }
            }

            if (activeEntity->type() == Entity::Type::Table) {
                TableEntity * table = static_cast<TableEntity *>(activeEntity);
                connection->parseTableStructure(table);
            }
        }
        emit activeEntityChanged(activeEntity, select);
    }
}

void ConnectionsManager::onEntityEdited(Entity * entity)
{
    emit entityEdited(entity);
}

void ConnectionsManager::onEntityInserted(Entity * entity)
{
    emit entityInserted(entity);
    setActiveEntity(entity, true);
}

void ConnectionsManager::setActiveSession(SessionEntity * session)
{
    if (_activeSession != session) {
        _activeSession = session;
        meowLogC(Log::Category::Info) << "Entering session: "
                 <<  (session ? session->name() : "");
        emit activeSessionChanged();
    }
}

} // namespace db
} // namespace meow
