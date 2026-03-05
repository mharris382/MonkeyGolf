#pragma once

#include "Modules/ModuleManager.h"

class FVRGolfPlayer : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
