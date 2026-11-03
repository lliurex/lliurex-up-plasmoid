#include "LliurexUpIndicatorServiceSubscriber.h"

#include <QCoreApplication>
#include <QFile>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusInterface>
#include <QDBusReply>

LliurexUpIndicatorServiceSubscriber::LliurexUpIndicatorServiceSubscriber(QObject *parent)
    : QObject(parent)
    , managerInterface(nullptr)
    , autoUpgradeRun(false)
    , m_isSubscribed(false)
    , m_isInitializing(false)
    , m_isSubscribing(false)
    , m_interfaceCreated(false)
       
{

}

void LliurexUpIndicatorServiceSubscriber::initSubscription(){

    if (m_interfaceCreated && managerInterface && managerInterface->isValid()){
        return;
    }

    if (m_isInitializing){
        return;
    }

    m_isInitializing=true;

    if (QFile::exists(disableAutoUpgrade)){
        m_isInitializing=false;
        emit createSubscriptionFinished(false);
        return;
    }

    bool startOk = false;
    
    try {
        startOk = createInterface();
    } catch (...) {

    }

    m_interfaceCreated = startOk; 
    m_isInitializing=false;

    if (!startOk){
        emit createSubscriptionFinished(false);
    }else{
        createSubscription();
    }

}

bool LliurexUpIndicatorServiceSubscriber::createInterface(){

    if (managerInterface && managerInterface->isValid()) {
        return true;
    }

    if (!QDBusConnection::systemBus().isConnected()) {
        return false;
    }

    if (managerInterface){
        delete managerInterface;
        managerInterface=nullptr;
    }
    
    managerInterface=new QDBusInterface("org.freedesktop.systemd1",
                                        "/org/freedesktop/systemd1",
                                        "org.freedesktop.systemd1.Manager",
                                        QDBusConnection::systemBus(),qApp);
       
    return managerInterface->isValid();   

}

void LliurexUpIndicatorServiceSubscriber::createSubscription(){

    if (!managerInterface || !managerInterface->isValid()){
        emit createSubscriptionFinished(autoUpgradeRun);
        return;
    }

    if (m_isSubscribed){
        emit createSubscriptionFinished(autoUpgradeRun);
        return;
    }

    if (m_isSubscribing){
        return;
    }

    m_isSubscribing=true;

    QDBusPendingCall unitCall = managerInterface->asyncCall("GetUnit", m_unitName);
    QDBusPendingCallWatcher *unitWatcher = new QDBusPendingCallWatcher(unitCall, this);
        
    connect(unitWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *uSelf) {
            
        if (!uSelf) return;

        QDBusPendingReply<QDBusObjectPath> unitReply = *uSelf;
        uSelf->deleteLater();

        if (unitReply.isError()) {
            m_isSubscribed=false;
            m_isSubscribing=false;
            emit createSubscriptionFinished(false);
            return;
        }

        QString path = unitReply.value().path();

        bool connected=QDBusConnection::systemBus().connect(
            "org.freedesktop.systemd1",
            path,
            "org.freedesktop.DBus.Properties",
            "PropertiesChanged",
            this,
            SLOT(onPropertiesChanged(const QString&, const QVariantMap&, const QStringList&))
        );

        if (connected){ 
            m_isSubscribed=true;
            QDBusInterface unitInterface("org.freedesktop.systemd1",
                                         path,
                                         "org.freedesktop.DBus.Properties",
                                         QDBusConnection::systemBus());

            if (unitInterface.isValid()) {
                QDBusMessage reply = unitInterface.call("Get",
                                                        "org.freedesktop.systemd1.Service",
                                                        "StatusText");

                if (reply.errorMessage().isEmpty()) {
                    if (reply.arguments().isEmpty()){
                        QVariant firstArgument = reply.arguments().at(0).value<QDBusVariant>().variant();
                        QString initialStatusText = firstArgument.toString();

                        if (!initialStatusText.isEmpty()) {
                            QVariantMap simulatedProperties;
                            simulatedProperties.insert("StatusText", initialStatusText);
                            this->onPropertiesChanged("org.freedesktop.systemd1.Unit", simulatedProperties, QStringList());
                        }
                    }
                }
            }
            m_isSubscribing=false;
            emit createSubscriptionFinished(true);
        }else{
            m_isSubscribing=false;
            m_isSubscribed=false;
            emit createSubscriptionFinished(false);
        }
    });

}

void LliurexUpIndicatorServiceSubscriber::onPropertiesChanged(const QString &interfaceName, const QVariantMap& changedProperties, const QStringList &invalidatedProperties)
{
    
    Q_UNUSED(interfaceName);
    Q_UNUSED(invalidatedProperties);

    static const QStringList activeStates={
        "before installing","Installing packages","upgrade is downloading",
        "upgrade is installing"
    };

    if (changedProperties.contains("StatusText")) {
        QString newState = changedProperties["StatusText"].toString();
        autoUpgradeRun=false;
        if (newState!=lastState){
            lastState=newState;
                
            for (const QString &state:activeStates){
                if (newState.contains(state)){
                    autoUpgradeRun=true;
                    break;
                }
            }
            emit serviceStateChanged(autoUpgradeRun);
        }
    }
}
