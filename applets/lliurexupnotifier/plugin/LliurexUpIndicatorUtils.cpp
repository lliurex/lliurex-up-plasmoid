/*
 * Copyright (C) 2023 Juan Ramon Pelegrina <juapesai@hotmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "LliurexUpIndicatorUtils.h"

#include <QFile>
#include <QDateTime>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDebug>
#include <QtDBus/QtDBus>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusConnection>
#include <chrono>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <n4d.hpp>
#include <variant.hpp>
#include <thread>

using namespace edupals;
using namespace std;

LliurexUpIndicatorUtils::LliurexUpIndicatorUtils(QObject *parent)
    : QObject(parent)
       
{
    user=qgetenv("USER");
    PKGCACHE.setFileName("/var/cache/apt/pkgcache.bin");
    AUTO_UPDATE_TOKEN.setFileName("/var/run/lliurex-up-auto-upgrade.token");
    AUTO_UPDATE_RUN_TOKEN.setFileName("/var/run/lliurex-up-auto-upgrade.lock");

}

void LliurexUpIndicatorUtils::cleanCache(){

    QFile CURRENT_VERSION_TOKEN;
    QDir cacheDir("/home/"+user+"/.cache/plasmashell/qmlcache");
    QString currentVersion="";
    bool clear=false;

    CURRENT_VERSION_TOKEN.setFileName("/home/"+user+"/.config/lliurex-up-indicator.conf");
    QString installedVersion=getInstalledVersion();

    if (!CURRENT_VERSION_TOKEN.exists()){
        if (CURRENT_VERSION_TOKEN.open(QIODevice::WriteOnly)){
            QTextStream data(&CURRENT_VERSION_TOKEN);
            data<<installedVersion;
            CURRENT_VERSION_TOKEN.close();
            clear=true;
        }
    }else{
        if (CURRENT_VERSION_TOKEN.open(QIODevice::ReadOnly)){
            QTextStream content(&CURRENT_VERSION_TOKEN);
            currentVersion=content.readLine();
            CURRENT_VERSION_TOKEN.close();
        }

        if (currentVersion!=installedVersion){
            if (CURRENT_VERSION_TOKEN.open(QIODevice::WriteOnly)){
                QTextStream data(&CURRENT_VERSION_TOKEN);
                data<<installedVersion;
                CURRENT_VERSION_TOKEN.close();
                clear=true;
            }
        }
    } 
    if (clear){
        if (cacheDir.exists()){
            cacheDir.removeRecursively();
        }
    }   

}

QString LliurexUpIndicatorUtils::getInstalledVersion(){

    QFile INSTALLED_VERSION_TOKEN;
    QString installedVersion="";
    
    INSTALLED_VERSION_TOKEN.setFileName("/var/lib/lliurex-up-indicator/version");

    if (INSTALLED_VERSION_TOKEN.exists()){
        if (INSTALLED_VERSION_TOKEN.open(QIODevice::ReadOnly)){
            QTextStream content(&INSTALLED_VERSION_TOKEN);
            installedVersion=content.readLine();
            INSTALLED_VERSION_TOKEN.close();
        }
    }
    return installedVersion;

}    

QStringList LliurexUpIndicatorUtils::getFlavours(){

    QProcess process;
    QStringList flavours;
    QString cmd="lliurex-version -v";
    process.start("/bin/sh",QStringList()<<"-c"<<cmd);
    process.waitForFinished(-1);
    QString stdout=process.readAllStandardOutput();
    QString stderr=process.readAllStandardError();
    flavours=stdout.split('\n');

    return flavours;

}

QStringList LliurexUpIndicatorUtils::getUserGroups(){
    

    int j, ngroups=32;
    gid_t groups[32];
    struct passwd *pw;
    struct group *gr;
    QStringList userGroups;
    isStudent=false;


    QByteArray uname = user.toLocal8Bit();
    const char *username = uname.data();
    pw=getpwnam(username);
    getgrouplist(username, pw->pw_gid, groups, &ngroups);
    for (j = 0; j < ngroups; j++) {
        gr = getgrgid(groups[j]);
        if (gr != NULL){
            if ((strcmp(gr->gr_name,"adm")==0)||(strcmp(gr->gr_name,"admins")==0)||(strcmp(gr->gr_name,"teachers")==0)){
                userGroups.append(gr->gr_name);
            }else if (strcmp(gr->gr_name,"students")==0){
                isStudent=true;
            } 
        }  
    }
    return userGroups;
}    

static bool simulateUpgrade(){

    try{
        
        QDBusInterface iface("org.debian.apt",  // from list on left
                         "/org/debian/apt", // from first line of screenshot
                         "org.debian.apt",  // from above Methods
                         QDBusConnection::systemBus());

   
        QDBusMessage result=iface.call("UpgradeSystem",true);
        QString transaction=result.arguments()[0].toString();
        
        QDBusInterface ifaceS("org.debian.apt",  // from list on left
                        transaction, // from first line of screenshot
                        "org.debian.apt.transaction",  // from above Methods
                         QDBusConnection::systemBus());

         
   
        ifaceS.call("Simulate");
      
        QDBusInterface ifaceR("org.debian.apt",  // from list on left
                        transaction, // from first line of screenshot
                        "org.freedesktop.DBus.Properties",  // from above Methods
                         QDBusConnection::systemBus());

        

        QDBusMessage reply = ifaceR.call("Get", "org.debian.apt.transaction", "Dependencies");
         
        const QDBusArgument& dbusArgs=reply.arguments()[0].value<QDBusVariant>().variant().value<QDBusArgument>();
         
        QVariantList pkgList;

        dbusArgs.beginStructure();

        int cont=0;

        while(!dbusArgs.atEnd()) {
            cont=cont+1;
            if (cont<6){
                dbusArgs.beginArray();

                while(!dbusArgs.atEnd()) {
                    QString s;
                    dbusArgs>>s;
                    pkgList.push_back(s);
                } 
            }      

            dbusArgs.endArray();
                           
        }       
        
        dbusArgs.endStructure();

        if (pkgList.size()>0){
            return true;
        }else{
            return false;
        } 
        
  
    }catch(...){

        return false;
    }      

}

bool LliurexUpIndicatorUtils::runUpdateCache(){
    
    if (!cacheUpdated){

        if (!QDBusConnection::sessionBus().isConnected()) {
            fprintf(stderr, "Cannot connect to the D-Bus session bus.\n"
                    "To start it, run:\n"
                    "\teval `dbus-launch --auto-syntax`\n");
            return false;
        }
        
          
        try{
            QDBusInterface iface("org.debian.apt",  // from list on left
                             "/org/debian/apt", // from first line of screenshot
                             "org.debian.apt",  // from above Methods
                             QDBusConnection::systemBus());


            
            QDBusMessage resultU=iface.call("UpdateCache");
            QString transactionU=resultU.arguments()[0].toString();
            
            QDBusInterface ifaceR("org.debian.apt",  // from list on left
                            transactionU, // from first line of screenshot
                            "org.debian.apt.transaction", // from above Methods
                             QDBusConnection::systemBus());


           

            ifaceR.asyncCall("Run");

            QDBusInterface ifaceS("org.debian.apt",  // from list on left
                            transactionU, // from first line of screenshot
                            "org.freedesktop.DBus.Properties",  // from above Methods
                             QDBusConnection::systemBus());

         
            std::this_thread::sleep_for(std::chrono::seconds(10));

            int match=0;
            int cont=0;
            int timeout=300;

            while (match==0){
                std::this_thread::sleep_for(std::chrono::seconds(1));
                QDBusMessage replyS = ifaceS.call("Get", "org.debian.apt.transaction", "ExitState");
                QString state = replyS.arguments()[0].value<QDBusVariant>().variant().toString();
                if (state != "exit-unfinished"){
                    match=1;
                }else{
                    cont=cont+1;
                    if (cont>timeout){
                        return false;
                    }

                }

            }

        }catch(...){
            return false;   
        }
    }
    cacheUpdated=false;
    return simulateUpgrade();
}

bool LliurexUpIndicatorUtils::checkRemote(){

    int cont=0;

    QStringList remotePts;
    QProcess process;
    QString cmd="who | grep -v \"(:0\"";

    process.start("/bin/sh", QStringList()<< "-c" 
                       << cmd);
    process.waitForFinished(-1);
    QString stdout=process.readAllStandardOutput();
    QString stderr=process.readAllStandardError();
    QStringList remoteUsers=stdout.split("\n");
    remoteUsers.takeLast();

    if (remoteUsers.size()>0){
        QRegularExpression re("pts/\\d+");
        QRegularExpressionMatch match = re.match(remoteUsers[0]);
        if (match.hasMatch()) {
            QString matched = match.captured(0);
            remotePts.push_back(matched);
        }  
    }      

    QProcess process2;
    QString cmd2="ps -ef | grep 'lliurex-upgrade' | grep -v 'grep'";
    process2.start("/bin/sh", QStringList()<< "-c" 
                       << cmd2);
    process2.waitForFinished(-1);
    QString stdout2=process2.readAllStandardOutput();
    QString stderr2=process2.readAllStandardError();
    QStringList remoteProcess=stdout2.split("\n");
    remoteProcess.takeLast();

    for(int i=0 ; i < remoteProcess.length() ; i++){
       if (remoteProcess[i].contains("?")){
           cont=cont+1;
       } // -> True

       for (int j=0; j<remotePts.length();j++){
            if (remoteProcess[i].contains(remotePts[j])){
                cont=cont+1;
            }
       }
    }

    if (cont>0){
        return true;

    }else{
        return false;
    }

}  

bool LliurexUpIndicatorUtils::isCacheUpdated(){

    QDateTime currentTime=QDateTime::currentDateTime();
    QDateTime lastModification=QFileInfo(PKGCACHE).lastModified();    

    qint64 millisecondsDiff = lastModification.msecsTo(currentTime);

    if (millisecondsDiff<APT_FRECUENCY){
        cacheUpdated=true;
    }
    return cacheUpdated;

}

bool LliurexUpIndicatorUtils::isConnectionWithServer(){

    try{
        n4d::Client client;
        client=n4d::Client("https://server:9779");
        variant::Variant test=client.call("MirrorManager","is_alive");
        return true;
               
    }catch(...){
        return false;
    }
     
}

bool LliurexUpIndicatorUtils::canStopAutoUpdate(){

    try{
        n4d::Client localClient;
        localClient=n4d::Client("https://localhost:9779");
        variant::Variant result=localClient.call("LliurexUpManager","can_cancel_auto_upgrade");
        return result;
    }catch(...){
        return false;
    }

}

bool LliurexUpIndicatorUtils::isAutoUpdateReady(){

    if (AUTO_UPDATE_TOKEN.exists()){
        return true;
    }else{
        return false;
    }
   
}

bool LliurexUpIndicatorUtils::isAutoUpdateRun(){
    
    if (AUTO_UPDATE_RUN_TOKEN.exists()){
        return true;
    }else{
        return false;
    }
}

QString LliurexUpIndicatorUtils::getAutoUpdateTime(){

    QString result;
    if (LliurexUpIndicatorUtils::isAutoUpdateReady()){
        if (AUTO_UPDATE_TOKEN.open(QIODevice::ReadOnly)){
            QTextStream content(&AUTO_UPDATE_TOKEN);
            result=content.readLine();
            AUTO_UPDATE_TOKEN.close();
        }

    }
    return result;
}

void LliurexUpIndicatorUtils::stopAutoUpdate(){

    try{
        if (!AUTO_UPDATE_RUN_TOKEN.exists()){
            n4d::Client localClient;
            localClient=n4d::Client("https://localhost:9779");
            localClient.call("LliurexUpManager","stop_auto_update_service");
        }
    }catch(...){
        
    }
    
}

QString LliurexUpIndicatorUtils::getAutoUpdatePause(){

    QString dateToUpdate="";

    try{
        n4d::Client localClient;
        localClient=n4d::Client("https://localhost:9779");
        variant::Variant result=localClient.call("LliurexUpManager","read_current_config");
       
        int weeksOfPause=result["data"]["weeksOfPause"].get_int32();

        if (weeksOfPause>0){
            QString tmpDate=QString::fromStdString(result["data"]["dateToUpdate"].get_string());
            QDate date1=QDate::fromString(tmpDate,"yyyy-MM-dd");
            dateToUpdate=date1.toString("dd/MM/yyyy");

        }
    }catch (...){
       
    }

    return dateToUpdate; 

}
