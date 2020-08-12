/*++

Programm name:

  Apostol

Module Name:

  Client365.hpp

Notices:

  Apostol Web Service (Client365)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_CLIENT365_HPP
#define APOSTOL_CLIENT365_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Client365 {

        //--------------------------------------------------------------------------------------------------------------

        //-- CClient365 ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CClient365: public CApostolModule {
        private:

            CStringList m_CacheList;

            void InitMethods() override;

            void UpdateCacheList();

            static bool CheckAuthorizationData(CRequest *ARequest, CAuthorization &Authorization);

            static void QueryToJson(CPQPollQuery *Query, CString& Json, CString &Session);

            static void ReplyError(CHTTPServerConnection *AConnection, CReply::CStatusType ErrorCode, const CString &Message);

        protected:

            void DoGet(CHTTPServerConnection *AConnection) override;
            void DoPost(CHTTPServerConnection *AConnection);

            void DoLogin(CHTTPServerConnection *AConnection);
            void DoFetch(CHTTPServerConnection *AConnection, const CString& Path);

            void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) override;
            void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) override;

        public:

            explicit CClient365(CModuleProcess *AProcess);

            static class CClient365 *CreateModule(CModuleProcess *AProcess) {
                return new CClient365(AProcess);
            }

            static bool CacheAge(const CString &FileName);
            CString GetCacheFile(const CString &Session, const CString &Path, const CString &Payload);

            CStringList &CacheList() { return m_CacheList; };
            const CStringList &CacheList() const { return m_CacheList; };

            static bool CheckAuthorization(CHTTPServerConnection *AConnection, CAuthorization &Authorization);

            void AuthorizedFetch(CHTTPServerConnection *AConnection, const CAuthorization &Authorization,
                const CString &Path, const CString &Payload, const CString &Agent, const CString &Host);

            static CString GetSession(CRequest *ARequest);
            static bool CheckSession(CRequest *ARequest, CString &Session);

            bool Enabled() override;
            bool CheckConnection(CHTTPServerConnection *AConnection) override;

        };

    }
}

using namespace Apostol::Client365;
}
#endif //APOSTOL_CLIENT365_HPP
