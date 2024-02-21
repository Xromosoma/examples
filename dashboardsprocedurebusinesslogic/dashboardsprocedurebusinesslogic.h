#ifndef DashBoardsProcedureBusinessLogic_H
#define DashBoardsProcedureBusinessLogic_H

#include "procedurebusinesslogic.h"

#include "dashboardwindow.h"

//! \brief Объект бизнес-логики для процедуры отображения дашбордов
/*! Реализует функции отображения дашбордов.
*/
class DashBoardsProcedureBusinessLogic : public ProcedureBusinessLogic
{
    Q_OBJECT
private:
    DashBoardWindow *dashBoardWindow; //!< окно отображения дашбордов

public:
    //! Конструктор
    /*! \param _objectInfo - информация об объекте,
     *  \param _parentObjectInfo - информация о родительском объекте.
     *  Создает объект класса \e DashBoardsProcedureBusinessLogic.
     */
    DashBoardsProcedureBusinessLogic(const ObjectHelper::ObjectInfo &_objectInfo,
                                     const ObjectHelper::ObjectInfo &_parentObjectInfo);
    //! Деструктор
    /*! Уничтожает объект класса \e DashBoardsProcedureBusinessLogic.
     */
    ~DashBoardsProcedureBusinessLogic();

public:
    //! Выполнение операции
    /*! \param operation - операция,
     *  \param properties - массив свойств.
     */
    void executeOperation(const ObjectHelper::ObjectOperation &operation,
                          const ObjectPropertyList &properties);
};

//! \brief Загрузчик бизнес-логики для процедуры отображения дашбордов
/*! Создает объект бизнес-логики.
*/
class DashBoardsProcedureBusinessLogicCreator : public BaseBusinessLogicCreator
{
    Q_OBJECT
#ifdef HAVE_QT5
    Q_PLUGIN_METADATA(IID "OPVF.PL.DashBoardsProcedureBusinessLogicCreator")
#endif
    Q_INTERFACES(BaseBusinessLogicCreator)

public:
    //! Выдача назначения бизнес-логики
    /*! \return - назначение объекта бизнес-логики.
    */
    QString getDestinationPurpose();
    //! Выдача псевдонима для назначения бизнес-логики
    /*! \return - псевдоним объекта бизнес-логики.
    */
    QString getDestinationAlias();
    //! Создание объекта бизнес-логики
    /*! \param objectInfo - информация об объекте,
     *  \param parentObjectInfo - информация о родительском объекте.
     *  \return - указатель на созданный объект.
     */
    BaseBusinessLogic *create(const ObjectHelper::ObjectInfo &objectInfo,
                              const ObjectHelper::ObjectInfo &parentObjectInfo);
};

#endif // DashBoardsProcedureBusinessLogic_H
