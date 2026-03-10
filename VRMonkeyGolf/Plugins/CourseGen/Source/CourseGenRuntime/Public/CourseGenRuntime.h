#pragma once

#include "Modules/ModuleManager.h"

class FCourseGenRuntime : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
