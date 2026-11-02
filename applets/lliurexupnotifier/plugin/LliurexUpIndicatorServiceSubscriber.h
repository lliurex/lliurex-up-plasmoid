#ifndef PLASMA_LLIUREX_UP_INDICATOR_SERVICE_SUBSCRIBER_H
#define PLASMA_LLIUREX_UP_INDICATOR_SERVICE_SUBSCRIBER_H

#include <QObject>
#include <QFile>
#include <QDir>
#include <QDBusInterface>
#include <QStringList>
#include <QVariantMap>


class LliurexUpIndicatorServiceSubscriber : public QObject
{
    Q_OBJECT


public:

   explicit LliurexUpIndicatorServiceSubscriber(QObject *parent=nullptr);

   QString m_unitName="lliurex-auto-upgrade.service";
  
   void initSubscription();

signals:

    void createSubscriptionFinished(bool startOk);
    void serviceStateChanged(bool autoUpgradeRun);

private:

    QDBusInterface *managerInterface;

    bool autoUpgradeRun=false;
    
    bool m_isSubscribed=false;
    bool m_isInitializing=false;
    bool m_isSubscribing=false;
    bool m_interfaceCreated=false;

    QString disableAutoUpgrade="/etc/lliurex-auto-upgrade/disabled";
    QString lastState;

    bool createInterface();
    void createSubscription();
    
private slots:

    void onPropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
 
};

#endif // PLASMA_LLIUREX_UP_INDICATOR_SERVICE_SUBSCRIBER_H
