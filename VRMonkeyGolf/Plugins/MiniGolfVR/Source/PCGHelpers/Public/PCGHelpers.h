#pragma once

#include "Modules/ModuleManager.h"

class FPCGHelpers : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
