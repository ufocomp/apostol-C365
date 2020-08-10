/*++

Program name:

  apostol

Module Name:

  Workers.hpp

Notices:

  Apostol

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_WORKERS_HPP
#define APOSTOL_WORKERS_HPP
//----------------------------------------------------------------------------------------------------------------------

#include "Client365/Client365.hpp"
#include "WebServer/WebServer.hpp"
//----------------------------------------------------------------------------------------------------------------------

static inline void CreateWorkers(CModuleProcess *AProcess) {
    CClient365::CreateModule(AProcess);
    CWebServer::CreateModule(AProcess);
}

#endif //APOSTOL_WORKERS_HPP
