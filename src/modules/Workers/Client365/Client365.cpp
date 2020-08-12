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
            m_pMethods->AddObject(_T("GET")    , (CObject *) new CMethodHandler(true, std::bind(&CClient365::DoGet, this, _1)));
            m_pMethods->AddObject(_T("POST")   , (CObject *) new CMethodHandler(true, std::bind(&CClient365::DoPost, this, _1)));
            m_pMethods->AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true, std::bind(&CClient365::DoOptions, this, _1)));
            m_pMethods->AddObject(_T("HEAD")   , (CObject *) new CMethodHandler(false, std::bind(&CClient365::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("PUT")    , (CObject *) new CMethodHandler(false, std::bind(&CClient365::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("DELETE") , (CObject *) new CMethodHandler(false, std::bind(&CClient365::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("TRACE")  , (CObject *) new CMethodHandler(false, std::bind(&CClient365::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("PATCH")  , (CObject *) new CMethodHandler(false, std::bind(&CClient365::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, std::bind(&CClient365::MethodNotAllowed, this, _1)));
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::QueryToJson(CPQPollQuery *Query, CString& Json, CString &Session) {

            clock_t start = clock();

            CPQResult *Result;
            CString LoginResult;

            for (int i = 0; i < Query->Count(); i++) {
                Result = Query->Results(i);
                if (Result->ExecStatus() != PGRES_TUPLES_OK)
                    throw Delphi::Exception::EDBError(Result->GetErrorMessage());
            }

            Result = Query->Results(0);

            if (Query->ResultCount() == 3) {

                if (SameText(Result->GetValue(0, 1), _T("f")))
                    throw Delphi::Exception::EDBError(Result->GetValue(0, 2));

                Session = Result->GetValue(0, 0);

                Result = Query->Results(1);
            }

            Json = "{\"result\": ";

            if (Result->nTuples() > 0) {

                Json += "[";
                for (int Row = 0; Row < Result->nTuples(); ++Row) {
                    for (int Col = 0; Col < Result->nFields(); ++Col) {
                        if (Row != 0)
                            Json += ", ";
                        if (Result->GetIsNull(Row, Col)) {
                            Json += "null";
                        } else {
                            Json += Result->GetValue(Row, Col);
                        }
                    }
                }
                Json += "]";

            } else {
                Json += "{}";
            }

            Json += "}";

            log_debug1(APP_LOG_DEBUG_CORE, Log(), 0, _T("Query parse runtime: %.2f ms."), (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::DoPostgresQueryExecuted(CPQPollQuery *APollQuery) {
            auto LConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->PollConnection());

            CString Session;

            if (LConnection != nullptr) {

                auto LReply = LConnection->Reply();

                CReply::CStatusType LStatus = CReply::bad_request;

                try {
                    QueryToJson(APollQuery, LReply->Content, Session);

                    if (!Session.IsEmpty()) {
                        LReply->AddHeader(_T("Authenticate"), _T("SESSION=") + Session);
                    }

                    LStatus = CReply::ok;

                    if (!LReply->CacheFile.IsEmpty()) {
                        LReply->Content.SaveToFile(LReply->CacheFile.c_str());
                    }
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

        void CClient365::ReplyError(CHTTPServerConnection *AConnection, CReply::CStatusType ErrorCode, const CString &Message) {
            auto LReply = AConnection->Reply();

            if (ErrorCode == CReply::unauthorized) {
                CReply::AddUnauthorized(LReply, AConnection->Data()["Authorization"] != "Basic", "invalid_client", Message.c_str());
            }

            LReply->Content.Clear();
            LReply->Content.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Delphi::Json::EncodeJsonString(Message).c_str());

            AConnection->SendReply(ErrorCode, nullptr, true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::UpdateCacheList() {
            CString LFile;
            LFile = Config()->ConfPrefix() + _T("cache.conf");
            if (FileExists(LFile.c_str())) {
                m_CacheList.LoadFromFile(LFile.c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CClient365::CacheAge(const CString &FileName) {
            if (FileName.IsEmpty() || !FileExists(FileName.c_str()))
                return false;

            time_t age = FileAge(FileName.c_str());
            if (age == -1)
                return false;

            time_t now = time(nullptr);

            return (now - age) <= 1 * 60; // 1 min
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CClient365::GetCacheFile(const CString &Session, const CString &Path, const CString &Payload) {

            CString LCacheFile;
            CString LData;

            int Index = m_CacheList.IndexOf(Path);

            if (Index != -1) {

                LCacheFile = Config()->CachePrefix();
                LCacheFile += Session;
                LCacheFile += Path;
                LCacheFile += _T("/");

                if (!DirectoryExists(LCacheFile.c_str())) {
                    if (!ForceDirectories(LCacheFile.c_str())) {
                        throw EOSError(errno, "force directories (%s) failed", LCacheFile.c_str());
                    }
                }

                LData = Path;
                LData += Payload.IsEmpty() ? "null" : Payload;

                const auto &sha1 = SHA1(LData);

                LCacheFile += b2a_hex( (const unsigned char*) sha1.c_str(), (int) sha1.size() );
            }

            return LCacheFile;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CClient365::GetSession(CRequest *ARequest) {

            const CString &LAuthenticate = ARequest->Headers.Values(_T("authenticate"));

            if (!LAuthenticate.IsEmpty()) {
                size_t Pos = LAuthenticate.Find('=');
                if (Pos != CString::npos) {
                    return LAuthenticate.SubString(Pos + 1);
                }
            }

            const auto& headerSession = ARequest->Headers.Values(_T("Session"));
            const auto& cookieSession = ARequest->Cookies.Values(_T("SID"));

            return headerSession.IsEmpty() ? cookieSession : headerSession;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CClient365::CheckSession(CRequest *ARequest, CString &Session) {

            const auto& LSession = GetSession(ARequest);

            if (LSession.Length() != 40)
                return false;

            Session = LSession;

            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CClient365::CheckAuthorizationData(CRequest *ARequest, CAuthorization &Authorization) {

            const auto &LHeaders = ARequest->Headers;
            const auto &LCookies = ARequest->Cookies;

            const auto &LAuthorization = LHeaders.Values(_T("Authorization"));

            if (LAuthorization.IsEmpty()) {

                if (!CheckSession(ARequest, Authorization.Username)) {
                    return false;
                }

                Authorization.Schema = CAuthorization::asBasic;
                Authorization.Type = CAuthorization::atSession;

            } else {
                Authorization << LAuthorization;
            }

            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CClient365::CheckAuthorization(CHTTPServerConnection *AConnection, CAuthorization &Authorization) {
            auto LRequest = AConnection->Request();

            try {
                if (CheckAuthorizationData(LRequest, Authorization)) {
                    return true;
                }

                if (Authorization.Schema == CAuthorization::asBasic)
                    AConnection->Data().Values("Authorization", "Basic");

                ReplyError(AConnection, CReply::unauthorized, "Unauthorized.");
            } catch (CAuthorizationError &e) {
                ReplyError(AConnection, CReply::bad_request, e.what());
            } catch (std::exception &e) {
                ReplyError(AConnection, CReply::bad_request, e.what());
            }

            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::AuthorizedFetch(CHTTPServerConnection *AConnection, const CAuthorization &Authorization,
            const CString &Path, const CString &Payload, const CString &Agent, const CString &Host) {

            CStringList SQL;

            if (Authorization.Type == CAuthorization::atSession) {

                SQL.Add(CString().Format("SELECT * FROM api.run(%s, '%s'::jsonb, %s);",
                                         PQQuoteLiteral(Path).c_str(),
                                         Payload.IsEmpty() ? "{}" : Payload.c_str(),
                                         PQQuoteLiteral(Authorization.Username).c_str()
                ));
            } else {

                SQL.Add(CString().Format("SELECT * FROM apis.login(%s, %s, %s);",
                                         PQQuoteLiteral(Authorization.Username).c_str(),
                                         PQQuoteLiteral(Authorization.Password).c_str(),
                                         PQQuoteLiteral(Host).c_str()
                ));

                SQL.Add(CString().Format("SELECT * FROM api.run(%s, '%s'::jsonb);",
                                         PQQuoteLiteral(Path).c_str(),
                                         Payload.IsEmpty() ? "{}" : Payload.c_str()
                ));

                SQL.Add("SELECT * FROM apis.logout();");
            }

            AConnection->Data().Values("authorized", "true");
            AConnection->Data().Values("path", Path);

            if (!StartQuery(AConnection, SQL)) {
                AConnection->SendStockReply(CReply::service_unavailable);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::DoLogin(CHTTPServerConnection *AConnection) {

            auto OnExecuted = [this, AConnection](CPQPollQuery *APollQuery) {

                auto LReply = AConnection->Reply();
                auto LResult = APollQuery->Results(0);

                CString ErrorMessage;
                CReply::CStatusType LStatus = CReply::bad_request;

                try {
                    if (LResult->ExecStatus() != PGRES_TUPLES_OK)
                        throw Delphi::Exception::EDBError(LResult->GetErrorMessage());

                    LStatus = CReply::ok;
                    Postgres::PQResultToJson(LResult, LReply->Content);
                } catch (Delphi::Exception::Exception &E) {
                    ExceptionToJson(LStatus, E, LReply->Content);
                    Log()->Error(APP_LOG_EMERG, 0, E.what());
                }

                AConnection->SendReply(LStatus, nullptr, true);
            };

            auto OnException = [this, AConnection](CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) {

                auto LReply = AConnection->Reply();

                ExceptionToJson(CReply::internal_server_error, *AException, LReply->Content);
                AConnection->SendStockReply(CReply::ok, true);

                Log()->Error(APP_LOG_EMERG, 0, AException->what());

            };

            auto LRequest = AConnection->Request();

            const auto &Host = GetHost(AConnection);

            CStringList SQL;
            CJSON Login;

            Login << LRequest->Content;

            const auto &UserName = Login["username"].AsString();
            const auto &Password = Login["password"].AsString();
            const auto &Session = Login["session"].AsString();
            const auto &Department = Login["department"].AsString();
            const auto &WorkPlace = Login["workplace"].AsString();

            if (Session.IsEmpty()) {

                if (UserName.IsEmpty() || Password.IsEmpty()) {
                    AConnection->SendStockReply(CReply::bad_request);
                    return;
                }

                SQL.Add(CString().Format("SELECT * FROM apis.login(%s, %s, %s, %s, %s);",
                                         PQQuoteLiteral(UserName).c_str(),
                                         PQQuoteLiteral(Password).c_str(),
                                         PQQuoteLiteral(Host).c_str(),
                                         PQQuoteLiteral(Department).c_str(),
                                         PQQuoteLiteral(WorkPlace).c_str()
                ));
            } else {
                SQL.Add(CString().Format("SELECT * FROM apis.slogin(%s, %s, %s, %s);",
                                         PQQuoteLiteral(Session).c_str(),
                                         PQQuoteLiteral(Host).c_str(),
                                         PQQuoteLiteral(Department).c_str(),
                                         PQQuoteLiteral(WorkPlace).c_str()
                ));
            }

            if (!ExecSQL(SQL, AConnection, OnExecuted, OnException)) {
                AConnection->SendStockReply(CReply::service_unavailable);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::DoFetch(CHTTPServerConnection *AConnection, const CString &Path) {

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            const auto& LContentType = LRequest->Headers.Values(_T("Content-Type")).Lower();
            const auto LContentJson = (LContentType.Find(_T("application/json")) != CString::npos);

            CJSON Json;
            if (!LContentJson) {
                ContentToJson(LRequest, Json);
            }

            const auto& LPayload = LContentJson ? LRequest->Content : Json.ToString();
            const auto& LAgent = GetUserAgent(AConnection);
            const auto& LHost = GetHost(AConnection);

            try {
                CAuthorization LAuthorization;
                if (CheckAuthorization(AConnection, LAuthorization)) {

                    if (LAuthorization.Type == CAuthorization::atSession) {
                        LReply->CacheFile = GetCacheFile(LAuthorization.Username, Path, LPayload);

                        if (CacheAge(LReply->CacheFile)) {
                            LReply->Content.LoadFromFile(LReply->CacheFile);
                            AConnection->SendReply(CReply::ok);
                            return;
                        }
                    }

                    AuthorizedFetch(AConnection, LAuthorization, Path, LPayload, LAgent, LHost);
                }
            } catch (std::exception &e) {
                AConnection->CloseConnection(true);
                AConnection->SendStockReply(CReply::bad_request);
                Log()->Error(APP_LOG_EMERG, 0, e.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::DoGet(CHTTPServerConnection *AConnection) {

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            LReply->ContentType = CReply::json;

            CStringList LRouts;
            SplitColumns(LRequest->Location.pathname, LRouts, '/');

            if (LRouts.Count() < 4) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            const auto& LService = LRouts[0].Lower();
            const auto& LVersion = LRouts[1].Lower();
            const auto& LCommand = LRouts[2].Lower();

            if (LRouts[1] == _T("v1")) {
                m_Version = 2; // Всё верно, так нужно
            } else if (LRouts[1] == _T("v2")) {
                m_Version = 1; // Всё верно, так нужно
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

                    CString LPath;
                    for (int I = 3; I < LRouts.Count(); ++I) {
                        LPath.Append('/');
                        LPath.Append(LRouts[I].Lower());
                    }

                    if (LPath.IsEmpty()) {
                        AConnection->SendStockReply(CReply::not_found);
                        return;
                    }

                    DoFetch(AConnection, LPath);
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
            auto LReply = AConnection->Reply();

            LReply->ContentType = CReply::json;

            CStringList LRouts;
            SplitColumns(LRequest->Location.pathname, LRouts, '/');

            if (LRouts.Count() < 4) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            if (LRouts[1] == _T("v1")) {
                m_Version = 2; // Всё верно, так нужно
            } else if (LRouts[1] == _T("v2")) {
                m_Version = 1; // Всё верно, так нужно
            }

            if (LRouts[0] != _T("api") || (m_Version == -1) || LRouts[2] != _T("json")) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            CString LPath;
            for (int I = 3; I < LRouts.Count(); ++I) {
                LPath.Append('/');
                LPath.Append(LRouts[I].Lower());
            }

            if (LPath.IsEmpty()) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            if (LRouts[3] == _T("login")) {
                DoLogin(AConnection);
            } else {
                DoFetch(AConnection, LPath);
            }
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