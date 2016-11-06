#ifndef MODELS_DB_CONNECTIONPARAMETERSFORM_H
#define MODELS_DB_CONNECTIONPARAMETERSFORM_H

#include <QObject>
#include <QString>

#include "db/connection_parameters.h"

namespace meow {
namespace models {
namespace forms {

class ConnectionParametersForm : public QObject
{
    Q_OBJECT

public:
    ConnectionParametersForm(
        const meow::db::ConnectionParameters & connectionParams = meow::db::ConnectionParameters()
    );
    virtual ~ConnectionParametersForm() {}

    db::NetworkType networkType() const { return _connectionParams.networkType(); }
    QString sessionName() const { return _connectionParams.sessionName(); }
    QString hostName() const { return _connectionParams.hostName(); }
    QString userName() const { return _connectionParams.userName(); }
    QString password() const { return _connectionParams.password(); }
    bool isLoginPrompt() const { return _connectionParams.isLoginPrompt(); }
    qint16 port() const { return _connectionParams.port(); }
    int index() const;
    const meow::db::ConnectionParameters & connectionParams() const { return _connectionParams; }

    void setNetworkType(db::NetworkType networkType);
    void setSessionName(const QString &sessionName);
    void setHostName(const QString &hostName);
    void setUserName(const QString &userName);
    void setPassword(const QString &password);
    void setLoginPrompt(bool loginPrompt);
    void setPort(qint16 port);

    bool isEqualTo(const meow::db::ConnectionParameters & connectionParams);

    Q_SIGNAL void changed();

private:
    meow::db::ConnectionParameters _connectionParams; // store a copy, so we can edit freely, not affecting real data
};

} // namespace forms
} // namespace models
} // namespace meow

#endif // MODELS_DB_CONNECTIONPARAMETERSFORM_H