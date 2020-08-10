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

            static void QueryToJson(CPQPollQuery *Query, CString& Json, CString &Session);

        protected:

            void DoGet(CHTTPServerConnection *AConnection) override;
            void DoPost(CHTTPServerConnection *AConnection);

            void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) override;
            void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) override;

        public:

            explicit CClient365(CModuleProcess *AProcess);

            static class CClient365 *CreateModule(CModuleProcess *AProcess) {
                return new CClient365(AProcess);
            }

            static bool CheckCache(const CString &FileName);

            CStringList &CacheList() { return m_CacheList; };
            const CStringList &CacheList() const { return m_CacheList; };

            bool Enabled() override;
            bool CheckConnection(CHTTPServerConnection *AConnection) override;

        };

    }
}

using namespace Apostol::Client365;
}
#endif //APOSTOL_CLIENT365_HPP
