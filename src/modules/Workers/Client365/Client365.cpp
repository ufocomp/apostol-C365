/*++

Programm name:

  Apostol

Module Name:

  Client365.cpp

Notices:

  Apostol Web Service (Client365)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

//----------------------------------------------------------------------------------------------------------------------

#include "Apostol.hpp"
#include "Client365.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Client365 {

        //--------------------------------------------------------------------------------------------------------------

        //-- CClient365 ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CClient365::CClient365(CModuleProcess *AProcess) : CApostolModule(AProcess, "client365") {
            m_Headers.Add("Authorization");

            CClient365::InitMethods();

            UpdateCacheList();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::InitMethods() {
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            m_pMethods->AddObject(_T("GET")    , (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoGet(Connection); }));
            m_pMethods->AddObject(_T("POST")   , (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoPost(Connection); }));
            m_pMethods->AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoOptions(Connection); }));
            m_pMethods->AddObject(_T("HEAD")   , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("PUT")    , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("DELETE") , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("TRACE")  , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("PATCH")  , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
#else
            m_pMethods->AddObject(_T("GET")    , (CObject *) new CMethodHandler(true, std::bind(&CAppServer::DoGet, this, _1)));
            m_pMethods->AddObject(_T("POST")   , (CObject *) new CMethodHandler(true, std::bind(&CAppServer::DoPost, this, _1)));
            m_pMethods->AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true, std::bind(&CAppServer::DoOptions, this, _1)));
            m_pMethods->AddObject(_T("HEAD")   , (CObject *) new CMethodHandler(false, std::bind(&CAppServer::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("PUT")    , (CObject *) new CMethodHandler(false, std::bind(&CAppServer::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("DELETE") , (CObject *) new CMethodHandler(false, std::bind(&CAppServer::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("TRACE")  , (CObject *) new CMethodHandler(false, std::bind(&CAppServer::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("PATCH")  , (CObject *) new CMethodHandler(false, std::bind(&CAppServer::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, std::bind(&CAppServer::MethodNotAllowed, this, _1)));
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::QueryToJson(CPQPollQuery *Query, CString& Json, CString &Session) {

            CPQResult *Login;
            CPQResult *Run;

            clock_t start = clock();

            if ((Query->ResultCount() > 0) && (Query->ResultCount() <= 2)) {

                CString LoginResult;

                Login = Query->Results(0);
                if (Login->ExecStatus() == PGRES_TUPLES_OK) {

                    LoginResult = Login->GetValue(0, 1);

                    if (LoginResult.IsEmpty()) {
                        throw Delphi::Exception::EDBError(_T("Login Result Is Empty!"));
                    }

                    Session = Login->GetValue(0, 0);

                    if (Query->ResultCount() == 2) {
                        Run = Query->Results(1);

                        if (Run->ExecStatus() == PGRES_TUPLES_OK) {
                            if (LoginResult == "t") {

                                Json = "{\"result\": ";

                                if (Run->nTuples() > 0) {

                                    Json += "[";
                                    for (int Row = 0; Row < Run->nTuples(); ++Row) {
                                        for (int Col = 0; Col < Run->nFields(); ++Col) {
                                            if (Row != 0)
                                                Json += ", ";
                                            if (Run->GetIsNull(Row, Col)) {
                                                Json += "null";
                                            } else {
                                                Json += Run->GetValue(Row, Col);
                                            }
                                        }
                                    }
                                    Json += "]";

                                } else {
                                    Json += "{}";
                                }

                                Json += "}";

                            } else {
                                Log()->Error(APP_LOG_EMERG, 0, Login->GetValue(0, 2));
                                Postgres::PQResultToJson(Login, Json);
                            }
                        } else {
                            throw Delphi::Exception::EDBError(Run->GetErrorMessage());
                        }
                    } else {
                        Log()->Error(APP_LOG_EMERG, 0, Login->GetValue(0, 2));
                        Postgres::PQResultToJson(Login, Json);
                    }
                } else {
                    throw Delphi::Exception::EDBError(Login->GetErrorMessage());
                }
            } else {
                throw Delphi::Exception::EDBError(_T("Invalid record count!"));
            }

            log_debug1(APP_LOG_DEBUG_CORE, Log(), 0, _T("Query parse runtime: %.2f ms."), (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::DoPostgresQueryExecuted(CPQPollQuery *APollQuery) {
            auto LConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->PollConnection());

            CString Session;

            if (LConnection != nullptr) {

                auto LReply = LConnection->Reply();

                CReply::CStatusType LStatus = CReply::internal_server_error;

                try {
                    QueryToJson(APollQuery, LReply->Content, Session);

                    if (!Session.IsEmpty()) {
                        LReply->AddHeader(_T("Authenticate"), _T("SESSION=") + Session);
                    }

                    if (!LReply->CacheFile.IsEmpty()) {
                        clock_t start = clock();
                        LReply->Content.SaveToFile(LReply->CacheFile.c_str());
                        log_debug1(APP_LOG_DEBUG_CORE, Log(), 0, _T("Save cache runtime: %.2f ms."), (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));
                    }

                    LStatus = CReply::ok;

                } catch (Delphi::Exception::Exception &E) {
                    ExceptionToJson(LStatus, E, LReply->Content);
                    Log()->Error(APP_LOG_EMERG, 0, E.what());
                }

                LConnection->SendReply(LStatus, nullptr, true);

            } else {

                auto LJob = m_pJobs->FindJobByQuery(APollQuery);
                if (LJob == nullptr) {
                    Log()->Error(APP_LOG_EMERG, 0, _T("Job not found by Query."));
                    return;
                }

                try {
                    QueryToJson(APollQuery, LJob->Reply().Content, Session);
                } catch (Delphi::Exception::Exception &E) {
                    ExceptionToJson(500, E, LJob->Reply().Content);
                    Log()->Error(APP_LOG_EMERG, 0, E.what());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) {
            auto LConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->PollConnection());

            if (LConnection != nullptr) {
                auto LReply = LConnection->Reply();

                CReply::CStatusType LStatus = CReply::internal_server_error;

                ExceptionToJson(LStatus, *AException, LReply->Content);

                LConnection->SendReply(LStatus);
            }

            Log()->Error(APP_LOG_EMERG, 0, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::UpdateCacheList() {
            CString LFile;
            LFile = Config()->Prefix() + _T("client365.cache");
            if (FileExists(LFile.c_str())) {
                m_CacheList.LoadFromFile(LFile.c_str());
                for (int i = m_CacheList.Count() - 1; i >= 0; --i) {
                    LFile = Config()->CachePrefix() + m_CacheList[i].SubString(1) + __T("/");
                    if (!ForceDirectories(LFile.c_str())) {
                        m_CacheList.Delete(i);
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CClient365::CheckCache(const CString &FileName) {
            if (!FileExists(FileName.c_str()))
                return false;

            time_t age = FileAge(FileName.c_str());
            if (age == -1)
                return false;

            time_t now = time(nullptr);

            return (now - age) <= 1 * 60; // 1 min
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::DoGet(CHTTPServerConnection *AConnection) {

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            LReply->ContentType = CReply::json;

            CStringList LRouts;
            SplitColumns(LRequest->Location.pathname, LRouts, '/');

            if (LRouts.Count() < 3) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            const auto& LService = LRouts[0].Lower();
            const auto& LVersion = LRouts[1].Lower();
            const auto& LCommand = LRouts[2].Lower();

            if (LVersion == "v1") {
                m_Version = 1;
            } else if (LVersion == "v2") {
                m_Version = 2;
            }

            if (LService != "api" || (m_Version == -1)) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            try {
                if (LCommand == "ping") {

                    AConnection->SendStockReply(CReply::ok);

                } else if (LCommand == "time") {

                    LReply->Content << "{\"serverTime\": " << LongToString(MsEpoch()) << "}";

                    AConnection->SendReply(CReply::ok);

                } else if (m_Version == 2) {

                    if (LRouts.Count() != 3) {
                        AConnection->SendStockReply(CReply::bad_request);
                        return;
                    }

                    const auto& Identity = LRouts[2];

                    if (Identity.Length() != APOSTOL_MODULE_UID_LENGTH) {
                        AConnection->SendStockReply(CReply::bad_request);
                        return;
                    }

                    auto LJob = m_pJobs->FindJobById(Identity);

                    if (LJob == nullptr) {
                        AConnection->SendStockReply(CReply::not_found);
                        return;
                    }

                    if (LJob->Reply().Content.IsEmpty()) {
                        AConnection->SendStockReply(CReply::no_content);
                        return;
                    }

                    LReply->Content = LJob->Reply().Content;

                    CReply::GetReply(LReply, CReply::ok);

                    LReply->Headers << LJob->Reply().Headers;

                    AConnection->SendReply();

                    delete LJob;

                } else {
                    AConnection->SendStockReply(CReply::not_found);
                }
            } catch (std::exception &e) {
                ExceptionToJson(CReply::bad_request, e, LReply->Content);

                AConnection->CloseConnection(true);
                AConnection->SendReply(CReply::ok);

                Log()->Error(APP_LOG_EMERG, 0, e.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::DoPost(CHTTPServerConnection *AConnection) {

            auto LRequest = AConnection->Request();

            CString LCacheFile;

            CStringList LRouts;
            SplitColumns(LRequest->Location.pathname, LRouts, '/');

            if (LRouts.Count() < 2) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            if (LRouts[1] == _T("v1")) {
                m_Version = 1;
            } else if (LRouts[1] == _T("v2")) {
                m_Version = 2;
            }

            if (LRouts[0] != _T("api") || (m_Version == -1)) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            const CString &LContentType = LRequest->Headers.Values(_T("content-type"));
            if (!LContentType.IsEmpty() && LRequest->ContentLength == 0) {
                AConnection->SendStockReply(CReply::no_content);
                return;
            }

            CString LPath;
            for (int I = 2; I < LRouts.Count(); ++I) {
                LPath.Append('/');
                LPath.Append(LRouts[I].Lower());
            }

            CStringList LSQL;

            if (LRouts[3] == _T("login")) {

                if (LRequest->Content.IsEmpty()) {
                    AConnection->SendStockReply(CReply::no_content);
                    return;
                }

                CJSON Login;

                Login << LRequest->Content;

                CString Host(Login["host"].AsString());
                if (Host.IsEmpty()) {
                    Host = AConnection->Socket()->Binding()->PeerIP();
                }

                const CString &Session = Login["session"].AsString();
                if (Session.IsEmpty()) {
                    LSQL.Add("SELECT * FROM apis.login('");

                    const CString &UserName = Login["username"].AsString();
                    const CString &Password = Login["password"].AsString();

                    if (UserName.IsEmpty() && Password.IsEmpty()) {
                        AConnection->SendStockReply(CReply::bad_request);
                        return;
                    }

                    LSQL.Last() += UserName;
                    LSQL.Last() += "', '" + Password;
                } else {
                    LSQL.Add("SELECT * FROM apis.slogin('");
                    LSQL.Last() += Session;
                }

                LSQL.Last() += "', '" + Host;

                const CString &Department = Login["department"].AsString();
                if (!Department.IsEmpty())
                    LSQL.Last() += "', '" + Department;

                const CString &WorkPlace = Login["workplace"].AsString();
                if (!WorkPlace.IsEmpty())
                    LSQL.Last() += "', '" + WorkPlace;

                LSQL.Last() += "');";

            } else {

                const CString &LAuthorization = LRequest->Headers.Values(_T("authorization"));
                const CString &LAuthenticate = LRequest->Headers.Values(_T("authenticate"));

                if (LAuthorization.IsEmpty() && LAuthenticate.IsEmpty()) {
                    AConnection->SendStockReply(CReply::unauthorized);
                    return;
                }

                CString LSession;

                if (!LAuthenticate.IsEmpty()) {
                    size_t Pos = LAuthenticate.Find('=');
                    if (Pos == CString::npos) {
                        AConnection->SendStockReply(CReply::bad_request);
                        return;
                    }

                    LSession = LAuthenticate.SubString(Pos + 1);
                }

                if (LSession.Length() != 40) {
                    AConnection->SendStockReply(CReply::bad_request);
                    return;
                }

                int Index = m_CacheList.IndexOf(LPath);
                if (Index != -1) {

                    LCacheFile = Config()->CachePrefix();
                    LCacheFile += LPath.SubString(1);
                    LCacheFile += _T("/");

                    if (!DirectoryExists(LCacheFile.c_str())) {
                        if (!ForceDirectories(LCacheFile.c_str())) {
                            AConnection->SendStockReply(CReply::internal_server_error);
                            return;
                        }
                    }

                    LCacheFile += LSession;
                    if (!LRequest->Content.IsEmpty()) {
                        std::hash<std::string> ContentHash;
                        std::string Content(LRequest->Content.c_str());
                        LCacheFile.Append('.');
                        LCacheFile << (size_t) ContentHash(Content);
                    }
                }

                if (!LCacheFile.IsEmpty()) {
                    auto LReply = AConnection->Reply();
                    LReply->CacheFile = LCacheFile;
                    if (CheckCache(LReply->CacheFile)) {
                        LReply->Content.LoadFromFile(LReply->CacheFile);
                        AConnection->SendReply(CReply::ok);
                        return;
                    }
                }

                LSQL.Add("SELECT * FROM apis.slogin('" + LSession + "');");
                LSQL.Add("SELECT * FROM api.run('" + LPath);

                if (LRequest->Content == "{}" || LRequest->Content == "[]")
                    LRequest->Content.Clear();

                if (!LRequest->Content.IsEmpty()) {
                    LSQL.Last() += "', '" + LRequest->Content;
                }

                LSQL.Last() += "');";
            }

            ExecSQL(LSQL, AConnection);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CClient365::Enabled() {
            if (m_ModuleStatus == msUnknown)
                m_ModuleStatus = Config()->IniFile().ReadBool("worker/Client365", "enable", true) ? msEnabled : msDisabled;
            return m_ModuleStatus == msEnabled;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CClient365::CheckConnection(CHTTPServerConnection *AConnection) {
            const auto& Location = AConnection->Request()->Location;
            return Location.pathname.SubString(0, 5) == _T("/api/");
        }
        //--------------------------------------------------------------------------------------------------------------
    }
}
}