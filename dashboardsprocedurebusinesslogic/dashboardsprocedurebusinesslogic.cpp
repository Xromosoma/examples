#include "dashboardsprocedurebusinesslogic.h"

DashBoardsProcedureBusinessLogic::DashBoardsProcedureBusinessLogic(const ObjectHelper::ObjectInfo &_objectInfo,
                                                                   const ObjectHelper::ObjectInfo &_parentObjectInfo) :
    ProcedureBusinessLogic(_objectInfo, _parentObjectInfo)
{
    setObjectName("DashBoardsProcedureBusinessLogic");

    dashBoardWindow = Q_NULLPTR;
}

DashBoardsProcedureBusinessLogic::~DashBoardsProcedureBusinessLogic()
{
    if (dashBoardWindow)
        delete dashBoardWindow;
}

void DashBoardsProcedureBusinessLogic::executeOperation(const ObjectHelper::ObjectOperation &operation,
                                                        const ObjectPropertyList &properties)
{
    Q_UNUSED(properties)

    if (operation != objectOperationSelect)
        return;

    if (!dashBoardWindow)
        dashBoardWindow = new DashBoardWindow();
    dashBoardWindow->dashBoardWindowObjectInfo = objectInfo;
    dashBoardWindow->refresh();
    dashBoardWindow->show();
    dashBoardWindow->raise();
}


QString DashBoardsProcedureBusinessLogicCreator::getDestinationPurpose()
{
    return objectHelper->getObjectTypeAlias(procedureObject);
}

QString DashBoardsProcedureBusinessLogicCreator::getDestinationAlias()
{
    return ru("Statistics");
}

BaseBusinessLogic *DashBoardsProcedureBusinessLogicCreator::create(const ObjectHelper::ObjectInfo &objectInfo,
                                                                   const ObjectHelper::ObjectInfo &parentObjectInfo)
{
    return (new DashBoardsProcedureBusinessLogic(objectInfo,
                                                 parentObjectInfo));
}

#ifndef HAVE_QT5
Q_EXPORT_PLUGIN2(BaseBusinessLogicCreator, DashBoardsProcedureBusinessLogicCreator)
#endif
