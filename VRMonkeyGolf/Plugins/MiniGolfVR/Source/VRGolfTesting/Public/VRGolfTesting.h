#pragma once

#include "Modules/ModuleManager.h"

class FVRGolfTesting : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
