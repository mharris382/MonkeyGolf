#pragma once

#include "Modules/ModuleManager.h"

class FVRGolfEnviornment : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
