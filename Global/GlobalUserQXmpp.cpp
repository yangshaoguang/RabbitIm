#include "GlobalUserQXmpp.h"
#include "Global.h"
#include "UserInfo/UserInfoXmpp.h"
#include "Client/ClientXmpp.h"
#include "qxmpp/QXmppVCardIq.h"
#include "qxmpp/QXmppVCardManager.h"
#include "qxmpp/QXmppUtils.h"
#include <QDir>

CGlobalUserQXmpp::CGlobalUserQXmpp(QObject *parent) 
    : CGlobalUser(parent)
{
}

QSharedPointer<CUserInfo> CGlobalUserQXmpp::NewUserInfo()
{
    QSharedPointer<CUserInfo> user(new CUserInfoXmpp);
    return user;
}

/**
 * @brief 新增加一个空的好友对象，并把此好友对象插入到好友列表中  
 *
 * @param szId：新增加好的ID  
 * @return QSharedPointer<CUserInfo>:成功,返回好友对象.失败,返回空  
 */
QSharedPointer<CUserInfo> CGlobalUserQXmpp::AddUserInfoRoster(const QString &szId)
{
    QSharedPointer<CUserInfo> user =this->GetUserInfoRoster(szId);
    if(!user.isNull())
    {
        LOG_MODEL_ERROR("CGlobalUserQXmpp", "AddUserInfoRoster:roster is exist:%s", szId.toStdString().c_str());
        return user;
    }

    user = NewUserInfo();
    ((CUserInfoXmpp*)user.data())->SetId(szId);
    m_UserInfoRoster.insert(szId, user);
    return user;
}

int CGlobalUserQXmpp::UpdateUserInfoLocale(const QXmppVCardIq &vCard, QString jid)
{
    m_bModify = true;
    if(m_UserInforLocale.isNull())
    {
        m_UserInforLocale = NewUserInfo();
    }

    ((CUserInfoXmpp*)m_UserInforLocale.data())->UpdateUserInfo(vCard, jid);
    return 0;
}

int CGlobalUserQXmpp::UpdateUserInfoRoster(const QXmppRosterIq::Item &rosterItem)
{
    int nRet = 0;
    m_bModify = true;
    QString jid = rosterItem.bareJid();
    QSharedPointer<CUserInfo> roster = GetUserInfoRoster(jid);
    if(roster.isNull())
    {
        LOG_MODEL_ERROR("CGlobalUserQXmpp", "There are not the roster:%s", jid.toStdString().c_str());
        return -1;
    }
    nRet = ((CUserInfoXmpp*)roster.data())->UpdateUserInfo(rosterItem);
    return nRet;
}

int CGlobalUserQXmpp::UpdateUserInfoRoster(const QXmppVCardIq &vCard, QString jid)
{
    int nRet = 0;
    m_bModify = true;
    QString szBareJid = QXmppUtils::jidToBareJid(jid);
    QSharedPointer<CUserInfo> roster = GetUserInfoRoster(szBareJid);
    if(roster.isNull())
    {
        LOG_MODEL_ERROR("CGlobalUserQXmpp", "There are not the roster:%s", jid.toStdString().c_str());
        return -1;
    }
    nRet = ((CUserInfoXmpp*)roster.data())->UpdateUserInfo(vCard, jid);
    return nRet;
}

QString CGlobalUserQXmpp::GetLocaleFile(const QString &szLocaleJid)
{
    return CGlobal::Instance()->GetDirUserData(szLocaleJid) 
            + QDir::separator() + "XmppLocaleInfo.dat";
}

QString CGlobalUserQXmpp::GetRosterFile(const QString &szLocaleJid)
{
    return CGlobal::Instance()->GetDirUserData(szLocaleJid) 
            + QDir::separator() + "XmppRosterInfo.dat";
}

int CGlobalUserQXmpp::LoadLocaleFromFile(const QString &szId)
{
    int nRet = 0;
    QString szFile = GetLocaleFile(szId);

    QFile in(szFile);
    if(!in.open(QFile::ReadOnly))
    {
        LOG_MODEL_WARNING("CGlobalUser", "Don't open file:%s", szFile.toStdString().c_str());
        return -1;
    }

    try{
        QDataStream s(&in);
        
        //版本号  
        int nVersion = 0;
        s >> nVersion;
        //本地用户信息  
        LOG_MODEL_DEBUG("CFrmUserList", "Version:%d", nVersion);
        if(m_UserInforLocale.isNull())
         {
            m_UserInforLocale = NewUserInfo();
        }
        s >> (CUserInfoXmpp&)*m_UserInforLocale;        
    }
    catch(...)
    {
        LOG_MODEL_ERROR("CGlobalUser", "CFrmUserList::LoadUserList exception");
        nRet = -1;
    }

    in.close();
    return nRet;
}

int CGlobalUserQXmpp::SaveLocaleToFile()
{
    int nRet = 0;
    QString szFile = GetLocaleFile(GetUserInfoLocale()->GetId());

    QFile out(szFile);
    if(!out.open(QFile::WriteOnly))
    {
        LOG_MODEL_WARNING("CGlobalUser", "Don't open file:%s", szFile.toStdString().c_str());
        return -1;
    }

    try
    {
        QDataStream s(&out);
        //版本号  
        int nVersion = 1;
        s << nVersion;
        //本地用户信息  
        s << (CUserInfoXmpp&)*m_UserInforLocale;     
    }
    catch(...)
    {
        LOG_MODEL_ERROR("CGlobalUser", "CFrmUserList::SaveUserList exception");
        return -1;
    }

    out.close();
    return nRet;
}

int CGlobalUserQXmpp::LoadRosterFromFile(QString szId)
{
    int nRet = 0;
    QString szFile = GetRosterFile(szId);
    QFile in(szFile);
    if(!in.open(QFile::ReadOnly))
    {
        LOG_MODEL_WARNING("CGlobalUser", "Don't open file:%s", szFile.toStdString().c_str());
        return -1;
    }

    try{
        QDataStream s(&in);

        //版本号  
        int nVersion = 0;
        s >> nVersion;
        //本地用户信息  
        LOG_MODEL_DEBUG("CFrmUserList", "Version:%d", nVersion);
        int nSize =0;
        s >> nSize;
        while(nSize--)
        {
            QString jid;
            QSharedPointer<CUserInfo> roster = NewUserInfo();
            s >> jid >> (CUserInfoXmpp&)*roster;
            m_UserInfoRoster.insert(jid, roster);
        }
    }
    catch(...)
    {
        LOG_MODEL_ERROR("CGlobalUser", "CFrmUserList::LoadUserList exception");
        nRet = -1;
    }

    in.close();
    return nRet;
}

int CGlobalUserQXmpp::SaveRosterToFile()
{
    int nRet = 0;
    QString szFile = GetRosterFile(GetUserInfoLocale()->GetId());

    QFile out(szFile);
    if(!out.open(QFile::WriteOnly))
    {
        LOG_MODEL_WARNING("CGlobalUser", "Don't open file:%s", szFile.toStdString().c_str());
        return -1;
    }

    try
    {
        QDataStream s(&out);
        //版本号  
        int nVersion = 1;
        s << nVersion;
        s << m_UserInfoRoster.size();
        QMap<QString, QSharedPointer<CUserInfo> >::iterator it;
        for(it = m_UserInfoRoster.begin(); it != m_UserInfoRoster.end(); it++)
        {
            s << it.key() << (CUserInfoXmpp&)*(it.value());
        }
    }
    catch(...)
    {
        LOG_MODEL_ERROR("CGlobalUser", "CFrmUserList::SaveUserList exception");
        return -1;
    }

    out.close();
    return nRet;
}