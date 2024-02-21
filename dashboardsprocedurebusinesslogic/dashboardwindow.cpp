#include "dashboardwindow.h"
#include "ui_dashboardwindow.h"
#include "dashboardsettingsdialog.h"

#include <guihelper.h>
#include <waitdialog.h>
#include <objectdatahelper.h>
#include <objectguihelper.h>

#include <QBuffer>
#include <QFileDialog>
#include <math.h>

#ifdef PL_USE_AX
#include <QAxObject>
#endif

const QString notDefinedString = ru("Не определен");
const QString totalCountString = ru("Общее количество");

int takeYearFromDate(const QString &date)
{
    QString yearStr;
    for (int i=date.length()-1;i>=0 && yearStr.length()<4;i--)
        if (date[i].isDigit())
            yearStr.prepend(date[i]);
    return yearStr.toInt();
}

DashBoardWindow::DashBoardWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DashBoardWindow)
{
    ui->setupUi(this);

    isChildVisible = false;
    currentDashName = QString();

    guiHelper->initTopFrame(ui->topFrame, ui->topFrameLayout);
    guiHelper->initMainArea(ui->mainArea, ui->mainAreaContents);
}

DashBoardWindow::~DashBoardWindow()
{
    clear();
    delete ui;
}

void DashBoardWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F1)
        showHelp();
    else if (event->key() == Qt::Key_Escape)
        close();
    QMainWindow::keyPressEvent(event);
}

void DashBoardWindow::refresh()
{
    //Чистим форму
    clear();

    //Готовим диалог ожидания
    systemUtils->showWaitDialog(false);

    //Формируем список дашбордов
    addDashFrame(ru("receivedAndOnBalanceRids"), ru("Динамика получения охранных документов"));
    addDashFrame(ru("patentedRids"), ru("Количество полученных патентов"));
    addDashFrame(ru("patentedRidsFromTotal"), ru("Количество полученных патентов (в процентах от общего количества за год)"));
    addDashFrame(ru("contractsByTypeAndWorkType"), ru("Договорные документы по типам и видам работ"));
    addDashFrame(ru("startedAndFinishedContracts"), ru("Контракты (начатые и завершенные)"));
    addDashFrame(ru("startedAndFinishedStages"), ru("Этапы (начатые и завершенные)"));

    //Прячем диалог ожидания
    systemUtils->hideWaitDialog();
}

void DashBoardWindow::clear()
{
    //Удаляем все фреймы дашбордов
    foreach (DashFrame *dashFrame, dashFrames)
    {
        guiHelper->freeFrame(dashFrame->mainFrame);
        delete dashFrame->mainFrame;
        delete dashFrame->childFrame;
    }

    //Очищаем список дашбордов
    dashFrames.clear();
}

DashBoardWindow::DashFrame *DashBoardWindow::addDashFrame(const QString &dashName,
                                                          const QString &dashCaption)
{
    //Добавляем фрейм дашборда
    DashFrame *dashFrame = new DashFrame();
    dashFrame->dashName = dashName;
    dashFrame->dashCaption = dashCaption;

    //Добавляем фрейм заголовка
    dashFrame->mainFrame = new QFrame(ui->mainAreaContents);
    dashFrame->mainFrameLayout = new QHBoxLayout(dashFrame->mainFrame);
    dashFrame->mainFrameLabel = new QLabel(dashFrame->mainFrame);
    dashFrame->mainFrameLabel->setText(dashFrame->dashCaption);
    dashFrame->mainFrameLayout->addWidget(dashFrame->mainFrameLabel);
    dashFrame->mainFrameLayout->addStretch();

    //Добавляем дочерний фрейм для графика и таблицы
    dashFrame->childFrame = new QFrame(ui->mainAreaContents);
    dashFrame->childFrameLayout = new QVBoxLayout(dashFrame->childFrame);

    //Добавляем фрейм кнопок
    dashFrame->buttonsFrame = new QFrame(dashFrame->childFrame);
    dashFrame->buttonsFrame->setSizePolicy(QSizePolicy::Expanding,
                                           QSizePolicy::Fixed);
    dashFrame->buttonsFrameLayout = new QHBoxLayout(dashFrame->buttonsFrame);
    dashFrame->buttonsFrameLayout->setMargin(0);
    //Добавляем кнопку выбора фильтров
    dashFrame->setupFilterButton = new QToolButton(dashFrame->buttonsFrame);
    dashFrame->setupFilterButton->setProperty("dashName", dashFrame->dashName);
    dashFrame->setupFilterButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    dashFrame->setupFilterButton->setText(ru("Выбор фильтра"));
    dashFrame->setupFilterButton->setIcon(QIcon(":/icons/icons/filter.png"));
    connect(dashFrame->setupFilterButton, SIGNAL(clicked()), this, SLOT(dashSetupFilterButtonClicked()));
    //Добавляем кнопку выбора дополнительных фильтров
    dashFrame->setupAdditionalFilterButton = new QToolButton(dashFrame->buttonsFrame);
    dashFrame->setupAdditionalFilterButton->setProperty("dashName", dashFrame->dashName);
    dashFrame->setupAdditionalFilterButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    dashFrame->setupAdditionalFilterButton->setText(ru("Выбор дополнительного фильтра"));
    dashFrame->setupAdditionalFilterButton->setIcon(QIcon(":/icons/icons/filter.png"));
    connect(dashFrame->setupAdditionalFilterButton, SIGNAL(clicked()), this, SLOT(dashSetupAdditionalFilterButtonClicked()));
    //Добавляем поле выбора типа отображения
    dashFrame->viewTypeBox = new QComboBox(dashFrame->buttonsFrame);
    dashFrame->viewTypeBox->setProperty("dashName", dashFrame->dashName);
    dashFrame->viewTypeBox->addItems(QStringList()
                                     << viewSettingsTableTypeTable
                                     << viewSettingsTableTypeChart
                                     << viewSettingsTableTypeComplex);
    //Устанавливаем тип отображения по умолчанию
    dashFrame->viewTypeBox->setCurrentIndex(dashFrame->viewTypeBox->findText(viewSettingsTableTypeComplex));
    //Подключаем слот
    connect(dashFrame->viewTypeBox, SIGNAL(currentIndexChanged(int)), this, SLOT(viewTypeBoxCurrentIndexChanged(int)));
    //Добавляем кнопку сохранения в HTML
    dashFrame->saveButtonHTML = new QToolButton(dashFrame->buttonsFrame);
    dashFrame->saveButtonHTML->setProperty("dashName", dashFrame->dashName);
    dashFrame->saveButtonHTML->setText(ru("Сохранить в HTML"));
    connect(dashFrame->saveButtonHTML, SIGNAL(clicked()), this, SLOT(dashFrameSaveButtonHTMLClicked()));
    //Добавляем кнопку сохранения в Excel
    dashFrame->saveButtonExcel = new QToolButton(dashFrame->buttonsFrame);
    dashFrame->saveButtonExcel->setProperty("dashName", dashFrame->dashName);
    dashFrame->saveButtonExcel->setText(ru("Сохранить в Excel"));
    connect(dashFrame->saveButtonExcel, SIGNAL(clicked()), this, SLOT(dashFrameSaveButtonExcelClicked()));
    //Добавляем кнопку сохранения в CSV
    dashFrame->saveButtonCSV = new QToolButton(dashFrame->buttonsFrame);
    dashFrame->saveButtonCSV->setProperty("dashName", dashFrame->dashName);
    dashFrame->saveButtonCSV->setText(ru("Сохранить в CSV"));
    connect(dashFrame->saveButtonCSV, SIGNAL(clicked()), this, SLOT(dashFrameSaveButtonCSVClicked()));
    //Добавляем виджеты на фрейм кнопок
    dashFrame->buttonsFrameLayout->addWidget(dashFrame->setupFilterButton);
    dashFrame->buttonsFrameLayout->addWidget(dashFrame->setupAdditionalFilterButton);
    dashFrame->buttonsFrameLayout->addWidget(dashFrame->viewTypeBox);
    dashFrame->buttonsFrameLayout->addWidget(dashFrame->saveButtonHTML);
    dashFrame->buttonsFrameLayout->addWidget(dashFrame->saveButtonExcel);
    dashFrame->buttonsFrameLayout->addWidget(dashFrame->saveButtonCSV);

    //Создаем фрейм для отображения фильтров
    dashFrame->filtersFrame = new QFrame(dashFrame->childFrame);
    dashFrame->filtersFrame->setSizePolicy(QSizePolicy::Expanding,
                                           QSizePolicy::Fixed);
    dashFrame->filtersFrameLayout = new QHBoxLayout(dashFrame->filtersFrame);
    dashFrame->filtersFrameLayout->setMargin(0);
    //Добавляем метку для отображения фильтров
    dashFrame->filtersLabel = new QLabel(dashFrame->filtersFrame);
    //Добавляем виджеты на фрейм отображения фильтров
    dashFrame->filtersFrameLayout->addWidget(dashFrame->filtersLabel);

    //Создаем область отображения графиков
    dashFrame->chartView = new QWebEngineView(dashFrame->childFrame);
    dashFrame->chartView->setProperty("dashName", dashFrame->dashName);
    dashFrame->chartView->setMinimumHeight(chartDefaultHeight+30);
    dashFrame->chartView->setMaximumHeight(chartDefaultHeight+30);
    connect(dashFrame->chartView, SIGNAL(loadFinished(bool)), this, SLOT(chartViewLoadFinished(bool)));

    //Создаем таблицу значений
    dashFrame->tableWidget = new QTableWidget(dashFrame->childFrame);
    dashFrame->tableWidget->setProperty("dashName", dashFrame->dashName);
    //Устанавливаем свойства таблицы
    guiHelper->initTableWidget(dashFrame->tableWidget);
    //Убираем сортировку
    dashFrame->tableWidget->setSortingEnabled(false);

    //Добавляем виджеты на фрейм
    dashFrame->childFrameLayout->addWidget(dashFrame->buttonsFrame);
    dashFrame->childFrameLayout->addWidget(dashFrame->filtersFrame);
    dashFrame->childFrameLayout->addWidget(dashFrame->chartView);
    dashFrame->childFrameLayout->addWidget(dashFrame->tableWidget);

    //Добавляем фреймы на форму
    ui->mainAreaContentsLayout->insertWidget(ui->mainAreaContentsLayout->indexOf(ui->mainAreaSpacer->widget()), dashFrame->mainFrame);
    ui->mainAreaContentsLayout->insertWidget(ui->mainAreaContentsLayout->indexOf(ui->mainAreaSpacer->widget()), dashFrame->childFrame);

    //Устанавливаем свойства фреймов
    guiHelper->initFrame(dashFrame->mainFrame,
                         dashFrame->mainFrameLabel,
                         dashFrame->mainFrameLayout,
                         dashFrame->childFrame);

    //Формируем информацию об объекте
    if (dashName == ru("contractsByTypeAndWorkType")
        || dashName == ru("startedAndFinishedContracts"))
    {
        //Запрашиваем информацию о справочнике ДоговорныеДокументы
        dashFrame->objectInfo = objectHelper->getObjectByAlias(ru("ДоговорныеДокументы"),
                                                               ObjectHelper::ObjectInfo(dictTableObject),
                                                               ObjectHelper::ObjectInfo(dictTablesObject));
    }
    else if (dashName == ru("startedAndFinishedStages"))
    {
        //Запрашиваем информацию о справочнике Этапы
        dashFrame->objectInfo = objectHelper->getObjectByAlias(ru("Этапы"),
                                                               ObjectHelper::ObjectInfo(dictTableObject),
                                                               ObjectHelper::ObjectInfo(dictTablesObject));
    }
    else if (dashName == ru("receivedAndOnBalanceRids")
             || dashName == ru("patentedRids")
             || dashName == ru("patentedRidsFromTotal"))
    {
        //Запрашиваем информацию о справочнике РезультатыНиокр
        dashFrame->objectInfo = objectHelper->getObjectByAlias(ru("РезультатыНиокр"),
                                                               ObjectHelper::ObjectInfo(dictTableObject),
                                                               ObjectHelper::ObjectInfo(dictTablesObject));
        //Запрашиваем информацию о табличной части ПравообладательРезультатыНИОКР
        dashFrame->additionalObjectInfo = objectHelper->getObjectByAlias(ru("ПравообладательРезультатыНИОКР"),
                                                                         ObjectHelper::ObjectInfo(partTableObject),
                                                                         ObjectHelper::ObjectInfo(partTablesObject));
    }

    //Создаем помощник для заполнения полей
    dashFrame->fieldsHelper = FieldsHelper::fillFieldsHelper(objectHelper->getDataUnit(dashFrame->objectInfo),
                                                             nullptr,
                                                             nullptr,
                                                             nullptr,
                                                             nullptr,
                                                             nullptr,
                                                             nullptr,
                                                             QString(),
                                                             QString(),
                                                             QString(),
                                                             objectDataHelper->checkAccessibleFieldsStatic);
    dashFrame->fieldsHelper->removeFilters(false);
    connect(dashFrame->fieldsHelper, SIGNAL(refreshed()), this, SLOT(refreshCurrentDash()));

    //Создаем помощник для заполнения дополнительных полей
    dashFrame->additionalFieldsHelper = FieldsHelper::fillFieldsHelper(objectHelper->getDataUnit(dashFrame->additionalObjectInfo),
                                                                       nullptr,
                                                                       nullptr,
                                                                       nullptr,
                                                                       nullptr,
                                                                       nullptr,
                                                                       nullptr,
                                                                       QString(),
                                                                       QString(),
                                                                       QString(),
                                                                       objectDataHelper->checkAccessibleFieldsStatic);
    dashFrame->additionalFieldsHelper->removeFilters(false);
    connect(dashFrame->additionalFieldsHelper, SIGNAL(refreshed()), this, SLOT(refreshCurrentDash()));

    //Прячем лишние компоненты
    dashFrame->setupAdditionalFilterButton->setVisible(!dashFrame->additionalObjectInfo.isEmpty());

    //Добавляем фрейм в список
    dashFrames[dashName] = dashFrame;

    //Обновляем фрейм
    refreshDashFrame(dashName);

    //Выдаем результат
    return dashFrame;
}

void DashBoardWindow::refreshDashFrame(const QString &dashName)
{
    if (!dashFrames.contains(dashName))
        return;

    //Получаем указатель на фрейм
    DashFrame *dashFrame = dashFrames[dashName];

    //Обновляем диалог ожидания
    systemUtils->setupWaitDialogProgress(ru("Построение графика %1").arg(dashFrame->dashCaption));

    //Готовим данные
    QMap<int,int> contractsAdded;
    QMap<int,int> contractsStarted;
    QMap<int,int> contractsFinished;
    QMap<int,int> contractsInWork;
    typedef QMap<QString,int> ContractsByWorkType;
    QMap<QString,ContractsByWorkType> contractsByTypeAndWorkType;
    if (dashName == ru("contractsByTypeAndWorkType")
        || dashName == ru("startedAndFinishedContracts"))
    {
        //Запрашиваем модуль данных для справочника ДоговорныеДокументы
        ObjectDataUnit *objectDataUnit = objectDataHelper->getContentDataUnit(dashFrame->objectInfo,
                                                                              ObjectHelper::ObjectInfo());
        if (objectDataUnit)
        {
            //Обновляем модуль данных
            if (!objectDataUnit->isRefreshed())
                objectDataUnit->refresh();

            //Получаем данные по фильтрам
            QPair<QString, QueryBindValueList> filtersDataForSelect = dashFrame->fieldsHelper->getFiltersDataForSelect();

            //Формируем условия отбора
            QStringList whereConditions;
            whereConditions << filtersDataForSelect.first
                            << dashFrame->fieldsHelper->additionalSelectionForSelect();
            whereConditions.removeAll(QString());

            //Формируем значения биндов
            QueryBindValueList bindValues = filtersDataForSelect.second;

            //Фильтруем модуль данных
            objectDataUnit->refresh(whereConditions,
                                    bindValues,
                                    QStringList(),
                                    QStringList(),
                                    QStringList()
                                    << ru("ТипДоговорногоДокумента")
                                    << ru("ВидРаботы")
                                    << ru("ДатаЗаключенияКонтракта")
                                    << ru("ДатаНачалаВыполненияРабот")
                                    << ru("ДатаОкончанияВыполненияРабот"));
            //Обновляем диалог ожидания
            systemUtils->setupWaitDialogProgress(ru("Загрузка контрактов"), objectDataUnit->getDataItemsCount());
            //Размер блока
            int blocksSize = 100;
            //Количество строк
            int dataUnitItemsCount = objectDataUnit->getDataItemsCount();
            //Количество блоков
            int blocksCount = dataUnitItemsCount/blocksSize + 1;
            //Читаем контракты и считаем количества
            for (int blockNumber = 0; blockNumber < blocksCount; ++blockNumber)
            {
                //Чистим накопленные записи
                objectDataUnit->clearDataItems();
                //Готовим записи
                objectDataUnit->prepareDataItems(blockNumber * blocksSize,
                                                 blocksSize);
                //Теперь читаем записи блока
                for (int i = blockNumber * blocksSize; i < blockNumber * blocksSize + blocksSize && i < dataUnitItemsCount; ++i)
                {
                    //Получаем запись объекта из словаря
                    ObjectDataUnit::DataItem dataItem = objectDataUnit->getObjectDataItem(i,
                                                                                          objectDataUnit->getDataItemsCount());
                    QString contractType = dataItem.getDataField(ru("ТипДоговорногоДокумента")).getView();
                    QString contractWorkType = dataItem.getDataField(ru("ВидРаботы")).getView();
                    if (contractType.isEmpty())
                    {
                        contractType = notDefinedString;
                    }
                    if (contractWorkType.isEmpty())
                    {
                        contractWorkType = notDefinedString;
                    }
                    contractsAdded[dataItem.getDataField(ru("ДатаЗаключенияКонтракта")).getValue().toDate().year()]++;
                    contractsStarted[dataItem.getDataField(ru("ДатаНачалаВыполненияРабот")).getValue().toDate().year()]++;
                    contractsFinished[dataItem.getDataField(ru("ДатаОкончанияВыполненияРабот")).getValue().toDate().year()]++;
                    contractsByTypeAndWorkType[contractType][contractWorkType]++;
                    contractsByTypeAndWorkType[contractType][totalCountString]++;
                    contractsByTypeAndWorkType[totalCountString][contractWorkType]++;
                    contractsByTypeAndWorkType[totalCountString][totalCountString]++;
                    //Обновляем диалог ожидания
                    systemUtils->refreshWaitDialog(i);
                }
                //Чистим накопленные записи
                objectDataUnit->clearDataItems();
            }
            //Удаляем модуль данных
            objectDataHelper->deleteObjectDataUnit(objectDataUnit);
        }
    }

    //Готовим данные
    QMap<int,int> stagesStarted;
    QMap<int,int> stagesFinished;
    if (dashName == ru("startedAndFinishedStages"))
    {
        //Запрашиваем модуль данных для справочника Этапы
        ObjectDataUnit *objectDataUnit = objectDataHelper->getContentDataUnit(dashFrame->objectInfo,
                                                                              ObjectHelper::ObjectInfo());
        if (objectDataUnit)
        {
            //Обновляем модуль данных
            if (!objectDataUnit->isRefreshed())
                objectDataUnit->refresh();

            //Получаем данные по фильтрам
            QPair<QString, QueryBindValueList> filtersDataForSelect = dashFrame->fieldsHelper->getFiltersDataForSelect();

            //Формируем условия отбора
            QStringList whereConditions;
            whereConditions << filtersDataForSelect.first
                            << dashFrame->fieldsHelper->additionalSelectionForSelect();
            whereConditions.removeAll(QString());

            //Формируем значения биндов
            QueryBindValueList bindValues = filtersDataForSelect.second;

            //Фильтруем модуль данных
            objectDataUnit->refresh(whereConditions,
                                    bindValues,
                                    QStringList(),
                                    QStringList(),
                                    QStringList()
                                    << ru("ДатаНачала")
                                    << ru("ДатаОкончания"));
            //Обновляем диалог ожидания
            systemUtils->setupWaitDialogProgress(ru("Загрузка этапов"), objectDataUnit->getDataItemsCount());
            //Размер блока
            int blocksSize = 100;
            //Количество строк
            int dataUnitItemsCount = objectDataUnit->getDataItemsCount();
            //Количество блоков
            int blocksCount = dataUnitItemsCount/blocksSize + 1;
            //Читаем этапы и считаем количества
            for (int blockNumber = 0; blockNumber < blocksCount; ++blockNumber)
            {
                //Чистим накопленные записи
                objectDataUnit->clearDataItems();
                //Готовим записи
                objectDataUnit->prepareDataItems(blockNumber * blocksSize,
                                                 blocksSize);
                //Теперь читаем записи блока
                for (int i = blockNumber * blocksSize; i < blockNumber * blocksSize + blocksSize && i < dataUnitItemsCount; ++i)
                {
                    //Получаем запись объекта из словаря
                    ObjectDataUnit::DataItem dataItem = objectDataUnit->getObjectDataItem(i,
                                                                                          objectDataUnit->getDataItemsCount());
                    stagesStarted[dataItem.getDataField(ru("ДатаНачала")).getValue().toDate().year()]++;
                    stagesFinished[dataItem.getDataField(ru("ДатаОкончания")).getValue().toDate().year()]++;
                    //Обновляем диалог ожидания
                    systemUtils->refreshWaitDialog(i);
                }
                //Чистим накопленные записи
                objectDataUnit->clearDataItems();
            }
            //Удаляем модуль данных
            objectDataHelper->deleteObjectDataUnit(objectDataUnit);
        }
    }

    //Готовим данные
    QMap<int,int> ridsOnBalance;
    QMap<int,int> ridsTotal;
    QMap<int,int> ridsPatented;
    QStringList niokrIds;
    if (dashName == ru("receivedAndOnBalanceRids")
        || dashName == ru("patentedRids")
        || dashName == ru("patentedRidsFromTotal"))
    {
        //Формируем условие фильтрации по правообладателям
        QString additionalFilter;

        //Запрашиваем модуль данных для справочника РезультатыНиокр
        ObjectDataUnit *objectDataUnit = objectDataHelper->getContentDataUnit(dashFrame->objectInfo,
                                                                              ObjectHelper::ObjectInfo());
        if (objectDataUnit)
        {
            //Обновляем модуль данных
            if (!objectDataUnit->isRefreshed())
                objectDataUnit->refresh();

            //Получаем данные по фильтрам
            QPair<QString, QueryBindValueList> filtersDataForSelect = dashFrame->fieldsHelper->getFiltersDataForSelect();

            //Формируем условия отбора
            QStringList whereConditions;
            whereConditions << filtersDataForSelect.first
                            << dashFrame->fieldsHelper->additionalSelectionForSelect()
                            << additionalFilter;
            whereConditions.removeAll(QString());

            //Формируем значения биндов
            QueryBindValueList bindValues = filtersDataForSelect.second;

            //Фильтруем модуль данных
            objectDataUnit->refresh(whereConditions,
                                    bindValues,
                                    QStringList(),
                                    QStringList(),
                                    QStringList()
                                    << ru("ДатаРегистрации")
                                    << ru("ДатаПостановкиНаБухУчетДляОтображения")
                                    << ru("ПравоустанавливающийДокумент")
                                    << ru("НомерОхранногоДокумента"));
            //Обновляем диалог ожидания
            systemUtils->setupWaitDialogProgress(ru("Загрузка результатов НИОКР"), objectDataUnit->getDataItemsCount());
            //Размер блока
            int blocksSize = 100;
            //Количество строк
            int dataUnitItemsCount = objectDataUnit->getDataItemsCount();
            //Количество блоков
            int blocksCount = dataUnitItemsCount/blocksSize + 1;
            //Читаем результаты и считаем количества
            for (int blockNumber = 0; blockNumber < blocksCount; ++blockNumber)
            {
                //Чистим накопленные записи
                objectDataUnit->clearDataItems();
                //Готовим записи
                objectDataUnit->prepareDataItems(blockNumber * blocksSize,
                                                 blocksSize);
                //Теперь читаем записи блока
                for (int i = blockNumber * blocksSize; i < blockNumber * blocksSize + blocksSize && i < dataUnitItemsCount; ++i)
                {
                    //Получаем запись объекта из словаря
                    ObjectDataUnit::DataItem dataItem = objectDataUnit->getObjectDataItem(i, objectDataUnit->getDataItemsCount());
                    niokrIds.append(QString::number(objectDataUnit->getObjectFieldValue(ru("Код"), i).toDBSerial()));
                    int creationYear = dataItem.getDataField(ru("ДатаРегистрации")).getValue().toDate().year();
                    if (creationYear)
                        ridsTotal[creationYear]++;
                    if (dataItem.getDataField(ru("ПравоустанавливающийДокумент")).getView() == ru("Патент")
                        && !dataItem.getDataField(ru("НомерОхранногоДокумента")).getValue().toString().simplified().isEmpty()
                        && dataItem.getDataField(ru("ДатаРегистрации")).getValue().toDate().isValid())
                        ridsPatented[creationYear]++;

                    //Обновляем диалог ожидания
                    systemUtils->refreshWaitDialog(i);
                }
                //Чистим накопленные записи
                objectDataUnit->clearDataItems();
            }
            //Удаляем модуль данных
            objectDataHelper->deleteObjectDataUnit(objectDataUnit);
        }
    }

    if (dashName == ru("receivedAndOnBalanceRids"))
    {
        //Готовим данные
        QMap<int,bool> years;
        foreach(int year, ridsTotal.keys())
            if (year>0 && !years.contains(year))
                years[year] = true;

        //Посчитаем количество поставленных на баланс.Для этого получим массив данных
        QString ridsOnBalanceByYearsSelect = QString("SELECT date_part('year',  СведенияОПостановкеНаУчет.ДатаПостановкиНаБухУчет) AS Год, "
                                                     "СведенияОПостановкеНаУчет.Ссылка AS Ссылка "
                                                     "FROM СведенияОПостановкеНаУчет AS СведенияОПостановкеНаУчет "
                                                     "GROUP BY Год,Ссылка "
                                                     "ORDER BY Ссылка ");
        if (!niokrIds.isEmpty())
            ridsOnBalanceByYearsSelect = QString("SELECT date_part('year',  СведенияОПостановкеНаУчет.ДатаПостановкиНаБухУчет) AS Год, "
                                                 "СведенияОПостановкеНаУчет.Ссылка AS Ссылка "
                                                 "FROM СведенияОПостановкеНаУчет AS СведенияОПостановкеНаУчет "
                                                 "WHERE Ссылка IN (%1) "
                                                 "GROUP BY Год,Ссылка "
                                                 "ORDER BY Ссылка ").arg(niokrIds.join(","));

        QueryResult ridsOnBalanceByYearsResult = queryExecutor->execQuery(ridsOnBalanceByYearsSelect, QueryParams(QueryParamsType::resultFieldMetaAliases));
        if (ridsOnBalanceByYearsResult.result)
            for (const QVariant &record : ridsOnBalanceByYearsResult.records)
                ridsOnBalance[ridsOnBalanceByYearsResult.getRecordValue(ru("Год"), record).toInt()]++;

        foreach(int year, ridsOnBalance.keys())
            if (year>0 && !years.contains(year))
                years[year] = true;

        //Готовим таблицу
        dashFrame->tableWidget->setRowCount(0);
        dashFrame->tableWidget->setColumnCount(3);
        dashFrame->tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem(ru("Год")));
        dashFrame->tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem(ru("Получено")));
        dashFrame->tableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem(ru("Поставлено на баланс")));

        //Добавляем ряды данных
        QStringList xValues;
        QStringList yValuesReceived;
        QStringList yValuesOnBalance;
        foreach(int year, years.keys())
        {
            //Добавляем данные в ряды
            xValues.append(ru("'%1 год'").arg(year));
            yValuesReceived.append(ru("%1").arg(ridsTotal[year]));
            yValuesOnBalance.append(ru("%1").arg(ridsOnBalance[year]));
            //Добавляем строку таблицы
            int tableWidgetRow = dashFrame->tableWidget->rowCount();
            dashFrame->tableWidget->setRowCount(dashFrame->tableWidget->rowCount()+1);
            dashFrame->tableWidget->setItem(tableWidgetRow, 0, new QTableWidgetItem(ru("%1 год").arg(year)));
            dashFrame->tableWidget->setItem(tableWidgetRow, 1, new QTableWidgetItem(ru("%1").arg(ridsTotal[year])));
            dashFrame->tableWidget->setItem(tableWidgetRow, 2, new QTableWidgetItem(ru("%1").arg(ridsOnBalance[year])));
        }

        //Подготавливаем скрипт добавления графика
        dashFrame->chartScript = getChartScriptTemplate(dashFrame->dashName)
                                 .arg(chartDefaultHeight)
                                 .arg(xValues.join(","))
                                 .arg(yValuesReceived.join(","))
                                 .arg(yValuesOnBalance.join(","));
    }
    else if (dashName == ru("patentedRids"))
    {
        //Готовим данные
        QMap<int,bool> years;
        int totalRidsPatented = 0;
        foreach(int year, ridsTotal.keys())
            if (year>0 && !years.contains(year))
                years[year] = true;
        foreach(int year, ridsPatented.keys())
            if (year>0 && !years.contains(year))
                years[year] = true;
        foreach(int year, years.keys())
            totalRidsPatented += ridsPatented[year];

        //Готовим таблицу
        dashFrame->tableWidget->setRowCount(0);
        dashFrame->tableWidget->setColumnCount(3);
        dashFrame->tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem(ru("Год")));
        dashFrame->tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem(ru("Получено патентов (в процентах от общего количества)")));
        dashFrame->tableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem(ru("Количество патентов")));

        //Добавляем ряды данных
        QStringList seriesPatented;
        foreach(int year, years.keys())
        {
            double percent =  round(double(100*(double(ridsPatented[year])/totalRidsPatented))*100)/100;
            //Добавляем данные в ряды
            seriesPatented.append(ru("{ name: '%1', y: %2 }")
                                  .arg(year)
                                  .arg(percent));
            //Добавляем строку таблицы
            int tableWidgetRow = dashFrame->tableWidget->rowCount();
            dashFrame->tableWidget->setRowCount(dashFrame->tableWidget->rowCount()+1);
            dashFrame->tableWidget->setItem(tableWidgetRow, 0, new QTableWidgetItem(ru("%1 год").arg(year)));
            dashFrame->tableWidget->setItem(tableWidgetRow, 1, new QTableWidgetItem(ru("%1 %").arg(percent)));
            dashFrame->tableWidget->setItem(tableWidgetRow, 2, new QTableWidgetItem(ru("%1").arg(ridsPatented[year])));
        }

        //Добавляем общее количество
        int tableWidgetRow = dashFrame->tableWidget->rowCount();
        dashFrame->tableWidget->setRowCount(dashFrame->tableWidget->rowCount()+1);
        dashFrame->tableWidget->setItem(tableWidgetRow, 0, new QTableWidgetItem(ru("%1").arg(totalCountString)));
        dashFrame->tableWidget->setItem(tableWidgetRow, 1, new QTableWidgetItem(ru("%1%").arg(100)));
        dashFrame->tableWidget->setItem(tableWidgetRow, 2, new QTableWidgetItem(ru("%1").arg(totalRidsPatented)));

        //Подготавливаем скрипт добавления графика
        dashFrame->chartScript =  getChartScriptTemplate(dashFrame->dashName)
                                  .arg(chartDefaultHeight)
                                  .arg(seriesPatented.join(","));
    }
    else if (dashName == ru("patentedRidsFromTotal"))
    {
        //Готовим данные
        QMap<int,bool> years;
        foreach(int year, ridsTotal.keys())
            if (year>0 && !years.contains(year))
                years[year] = true;
        foreach(int year, ridsPatented.keys())
            if (year>0 && !years.contains(year))
                years[year] = true;

        //Готовим таблицу
        dashFrame->tableWidget->setRowCount(0);
        dashFrame->tableWidget->setColumnCount(4);
        dashFrame->tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem(ru("Год")));
        dashFrame->tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem(ru("Получено патентов (в процентах от общего количества за год)")));
        dashFrame->tableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem(ru("Количество патентов")));
        dashFrame->tableWidget->setHorizontalHeaderItem(3, new QTableWidgetItem(ru("Общее количество РИД")));

        //Добавляем ряды данных
        //Ось x
        QStringList xValues;

        //Общий контент для колонок
        QString totalSeriesData;
        //Контент запатентованных РИД
        QStringList patentedSeriesData;
        //Контент незапатентованных РИД
        QStringList unpatentedSeriesData;
        foreach(int year, years.keys())
        {
            double percent = ridsTotal[year]>0?round(double(100*(double(ridsPatented[year])/ridsTotal[year]))*100)/100:0;
            xValues.append(ru("'%1 год'").arg(year));
            patentedSeriesData.append(ru("%1").arg(ridsPatented[year]));
            unpatentedSeriesData.append(ru("%1").arg(ridsTotal[year]-ridsPatented[year]));
            //Добавляем строку таблицы
            int tableWidgetRow = dashFrame->tableWidget->rowCount();
            dashFrame->tableWidget->setRowCount(dashFrame->tableWidget->rowCount()+1);
            dashFrame->tableWidget->setItem(tableWidgetRow, 0, new QTableWidgetItem(ru("%1 год").arg(year)));
            dashFrame->tableWidget->setItem(tableWidgetRow, 1, new QTableWidgetItem(ru("%1 %").arg(percent)));
            dashFrame->tableWidget->setItem(tableWidgetRow, 2, new QTableWidgetItem(ru("%1").arg(ridsPatented[year])));
            dashFrame->tableWidget->setItem(tableWidgetRow, 3, new QTableWidgetItem(ru("%1").arg(ridsTotal[year])));
        }

        totalSeriesData.append(ru("{ name: 'Не запатентовано', data: [%1] }, {name : 'Запатентовано', data : [%2]}")
                               .arg(unpatentedSeriesData.join(","))
                               .arg(patentedSeriesData.join(",")));
        qDebug()<<"!@#TOTAL SERIES = "<<totalSeriesData;
        //Подготавливаем скрипт добавления графика
        dashFrame->chartScript =  getChartScriptTemplate(dashFrame->dashName)
                                  .arg(chartDefaultHeight)
                                  .arg(xValues.join(","))
                                  .arg(totalSeriesData);
    }
    else if (dashName == ru("contractsByTypeAndWorkType"))
    {
        //Готовим данные
        QList<QString> contractsTypes;
        QList<QString> contractsWorkTypes;
        QMapIterator<QString,ContractsByWorkType> contractsByTypeAndWorkTypeIterator(contractsByTypeAndWorkType);
        while (contractsByTypeAndWorkTypeIterator.hasNext())
        {
            contractsByTypeAndWorkTypeIterator.next();
            QString contractsType = contractsByTypeAndWorkTypeIterator.key();
            if (!contractsTypes.contains(contractsType))
                contractsTypes.append(contractsType);
            foreach(QString contractsWorkType, contractsByTypeAndWorkTypeIterator.value().keys())
            {
                if (!contractsWorkTypes.contains(contractsWorkType))
                    contractsWorkTypes.append(contractsWorkType);
            }
        }

        //Ставим "Не определен" в конец списков
        if (contractsTypes.removeAll(notDefinedString) > 0)
            contractsTypes.append(notDefinedString);
        if (contractsWorkTypes.removeAll(notDefinedString) > 0)
            contractsWorkTypes.append(notDefinedString);

        //Ставим "Общее количество" в конец списков
        if (contractsTypes.removeAll(totalCountString) > 0)
            contractsTypes.append(totalCountString);
        if (contractsWorkTypes.removeAll(totalCountString) > 0)
            contractsWorkTypes.append(totalCountString);

        //Готовим таблицу
        dashFrame->tableWidget->setRowCount(0);
        dashFrame->tableWidget->setColumnCount(contractsWorkTypes.size()+1);
        dashFrame->tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem(ru("Тип договорного документа")));
        for (int i=0;i<contractsWorkTypes.size();i++)
            dashFrame->tableWidget->setHorizontalHeaderItem(i+1, new QTableWidgetItem(contractsWorkTypes[i]));

        //Добавляем данные в ряды
        QStringList xValues;
        QHash<QString,QStringList> yValues;
        QStringList ySeries;
        foreach(QString contractsWorkType, contractsWorkTypes)
        {
            xValues.append(ru("'%1'").arg(contractsWorkType));
        }
        foreach(QString contractsType, contractsTypes)
        {
            for (int i=0;i<contractsWorkTypes.size();i++)
            {
                yValues[contractsType].append(ru("%1").arg(contractsByTypeAndWorkType[contractsType][contractsWorkTypes[i]]));
            }
            ySeries.append(QString("{name: '%1', data: [%2]}").arg(contractsType).arg(yValues[contractsType].join(",")));
        }

        //Заполняем таблицу
        foreach(QString contractsType, contractsTypes)
        {
            //Добавляем строку таблицы
            int tableWidgetRow = dashFrame->tableWidget->rowCount();
            dashFrame->tableWidget->setRowCount(dashFrame->tableWidget->rowCount()+1);
            dashFrame->tableWidget->setItem(tableWidgetRow, 0, new QTableWidgetItem(contractsType));
            for (int i=0;i<contractsWorkTypes.size();i++)
                dashFrame->tableWidget->setItem(tableWidgetRow, i+1, new QTableWidgetItem(yValues[contractsType][i]));
        }

        //Подготавливаем скрипт добавления графика
        dashFrame->chartScript = getChartScriptTemplate(dashFrame->dashName)
                                 .arg(chartDefaultHeight)
                                 .arg(xValues.join(","))
                                 .arg(ySeries.join(","));
    }
    else if (dashName == ru("startedAndFinishedContracts"))
    {
        //Готовим данные
        QMap<int,bool> years;
        foreach(int year, contractsAdded.keys())
            if (year>0 && !years.contains(year))
                years[year] = true;
        foreach(int year, contractsStarted.keys())
            if (year>0 && !years.contains(year))
                years[year] = true;
        foreach(int year, contractsFinished.keys())
            if (year>0 && !years.contains(year))
                years[year] = true;
        //Определяем контракты в работе
        int contractsInWorkCount = 0;
        foreach(int year, years.keys())
        {
            contractsInWorkCount += contractsStarted[year] - contractsFinished[year];
            contractsInWork[year] = contractsInWorkCount;
        }

        //Готовим таблицу
        dashFrame->tableWidget->setRowCount(0);
        dashFrame->tableWidget->setColumnCount(5);
        dashFrame->tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem(ru("Год")));
        dashFrame->tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem(ru("Заключено контрактов")));
        dashFrame->tableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem(ru("Начато работ по контрактам")));
        dashFrame->tableWidget->setHorizontalHeaderItem(3, new QTableWidgetItem(ru("Завершено контрактов")));
        dashFrame->tableWidget->setHorizontalHeaderItem(4, new QTableWidgetItem(ru("Контрактов в работе")));

        //Добавляем ряды данных
        QStringList xValues;
        QStringList yValuesAdded;
        QStringList yValuesStarted;
        QStringList yValuesFinished;
        QStringList yValuesInWork;
        foreach(int year, years.keys())
        {
            //Добавляем данные в ряды
            xValues.append(ru("'%1 год'").arg(year));
            yValuesAdded.append(ru("%1").arg(contractsAdded[year]));
            yValuesStarted.append(ru("%1").arg(contractsStarted[year]));
            yValuesFinished.append(ru("%1").arg(contractsFinished[year]));
            yValuesInWork.append(ru("%1").arg(contractsInWork[year]));
            //Добавляем строку таблицы
            int tableWidgetRow = dashFrame->tableWidget->rowCount();
            dashFrame->tableWidget->setRowCount(dashFrame->tableWidget->rowCount()+1);
            dashFrame->tableWidget->setItem(tableWidgetRow, 0, new QTableWidgetItem(ru("%1 год").arg(year)));
            dashFrame->tableWidget->setItem(tableWidgetRow, 1, new QTableWidgetItem(ru("%1").arg(contractsAdded[year])));
            dashFrame->tableWidget->setItem(tableWidgetRow, 2, new QTableWidgetItem(ru("%1").arg(contractsStarted[year])));
            dashFrame->tableWidget->setItem(tableWidgetRow, 3, new QTableWidgetItem(ru("%1").arg(contractsFinished[year])));
            dashFrame->tableWidget->setItem(tableWidgetRow, 4, new QTableWidgetItem(ru("%1").arg(contractsInWork[year])));
        }

        //Подготавливаем скрипт добавления графика
        dashFrame->chartScript = getChartScriptTemplate(dashFrame->dashName)
                                 .arg(chartDefaultHeight)
                                 .arg(xValues.join(","))
                                 .arg(yValuesAdded.join(","))
                                 .arg(yValuesStarted.join(","))
                                 .arg(yValuesFinished.join(","))
                                 .arg(yValuesInWork.join(","));
    }
    else if (dashName == ru("startedAndFinishedStages"))
    {
        //Готовим данные
        QMap<int,bool> years;
        foreach(int year, stagesStarted.keys())
            if (year>0 && !years.contains(year))
                years[year] = true;
        foreach(int year, stagesFinished.keys())
            if (year>0 && !years.contains(year))
                years[year] = true;

        //Готовим таблицу
        dashFrame->tableWidget->setRowCount(0);
        dashFrame->tableWidget->setColumnCount(3);
        dashFrame->tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem(ru("Год")));
        dashFrame->tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem(ru("Начатые")));
        dashFrame->tableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem(ru("Завершенные")));

        //Добавляем ряды данных
        QStringList xValues;
        QStringList yValuesStarted;
        QStringList yValuesFinished;
        foreach(int year, years.keys())
        {
            //Добавляем данные в ряды
            xValues.append(ru("'%1 год'").arg(year));
            yValuesStarted.append(ru("%1").arg(stagesStarted[year]));
            yValuesFinished.append(ru("%1").arg(stagesFinished[year]));
            //Добавляем строку таблицы
            int tableWidgetRow = dashFrame->tableWidget->rowCount();
            dashFrame->tableWidget->setRowCount(dashFrame->tableWidget->rowCount()+1);
            dashFrame->tableWidget->setItem(tableWidgetRow, 0, new QTableWidgetItem(ru("%1 год").arg(year)));
            dashFrame->tableWidget->setItem(tableWidgetRow, 1, new QTableWidgetItem(ru("%1").arg(stagesStarted[year])));
            dashFrame->tableWidget->setItem(tableWidgetRow, 2, new QTableWidgetItem(ru("%1").arg(stagesFinished[year])));
        }

        //Подготавливаем скрипт добавления графика
        dashFrame->chartScript = getChartScriptTemplate(dashFrame->dashName)
                                 .arg(chartDefaultHeight)
                                 .arg(xValues.join(","))
                                 .arg(yValuesStarted.join(","))
                                 .arg(yValuesFinished.join(","));
    }

    //Выравниваем размеры столбцов
    FieldsHelper::resizeAllColumnsToContents(dashFrame->tableWidget);

    //Формируем список фильтров
    //КОСТЫЛЬ: в платформе уже нет данных для filterForShow
    /*QString filterForShow = dashFrame->fieldsHelper->filterForShow();
    QString additionalFilterForShow = dashFrame->additionalFieldsHelper->filterForShow();*/
    QString filterForShow;
    QString additionalFilterForShow;

    //Отображаем список фильтров
    QStringList filtersForShow;
    if (!filterForShow.isEmpty())
        filtersForShow.append(ru("<b>Фильтры:</b><br>%1").arg(filterForShow.replace("\n","<br>")));
    if (!additionalFilterForShow.isEmpty())
        filtersForShow.append(ru("<b>Дополнительные фильтры:</b><br>%1").arg(additionalFilterForShow.replace("\n","<br>")));
    //Отображаем фильтры
    dashFrame->filtersLabel->setText(filtersForShow.join("<br>"));
    //Отображаем или скрываем фрейм отображения фильтров
    dashFrame->filtersFrame->setVisible(!filtersForShow.isEmpty());

    //Обновляем добавленный фрейм дашборда
    updateDashFrame(dashFrame->dashName);
}

void DashBoardWindow::refreshCurrentDash()
{
    //Готовим диалог ожидания
    systemUtils->showWaitDialog(false);
    if (!currentDashName.isEmpty())
        refreshDashFrame(currentDashName);
    //Прячем диалог ожидания
    systemUtils->hideWaitDialog();
}

void DashBoardWindow::updateDashFrame(const QString &dashName)
{
    if (!dashFrames.contains(dashName))
        return;
    DashFrame *dashFrame = dashFrames[dashName];
    bool showChart = dashFrame->viewTypeBox->currentText() != viewSettingsTableTypeTable;
    bool showTable = dashFrame->viewTypeBox->currentText() != viewSettingsTableTypeChart;
    dashFrame->chartView->setVisible(showChart);
    dashFrame->tableWidget->setVisible(showTable);

    //Отображаем график
    if (showChart)
    {
        //Грузим страницу графиков
        qDebug() << "DashBoardWindow::Load chart" << systemUtils->getDataDir(ProjectSettings::dataStorageForApp) + "/templates/charts/chartEngine/highcharts.html";
        dashFrame->chartView->load(QUrl::fromLocalFile(systemUtils->getDataDir(ProjectSettings::dataStorageForApp) + "/templates/charts/chartEngine/highcharts.html"));
        //Ожидаем окончания загрузки страницы
        QApplication::processEvents();
    }
}

void DashBoardWindow::updateChartView(const QString &dashName)
{
    if (!dashFrames.contains(dashName))
        return;
    DashFrame *dashFrame = dashFrames[dashName];
    //Выполняем скрипт добавления графика
    dashFrame->chartView->page()->runJavaScript(dashFrame->chartScript);
}

QString DashBoardWindow::getChartScriptTemplate(const QString &dashName)
{
    QString chartScriptFileName = QString("%1/templates/charts/%2.js")
                                  .arg(systemUtils->getDataDir(ProjectSettings::dataStorageForApp))
                                  .arg(dashName);
    return QString::fromUtf8(systemUtils->readDataFromFile(chartScriptFileName));
}

void DashBoardWindow::showHelp()
{
    objectGuiHelper->showHelp(dashBoardWindowObjectInfo);
}

void DashBoardWindow::setupFilter(const QString &dashName)
{
    if (!dashFrames.contains(dashName))
        return;
    DashFrame *dashFrame = dashFrames[dashName];
    currentDashName = dashName;
    objectGuiHelper->showSetFiltersDialog(dashFrame->fieldsHelper,isModal());

}

void DashBoardWindow::setupAdditionalFilter(const QString &dashName)
{
    if (!dashFrames.contains(dashName))
        return;
    DashFrame *dashFrame = dashFrames[dashName];
    currentDashName = dashName;
    objectGuiHelper->showSetFiltersDialog(dashFrame->additionalFieldsHelper,isModal());
}

void DashBoardWindow::saveToHTML(const QString &dashName)
{
    if (!dashFrames.contains(dashName))
        return;
    DashFrame *dashFrame = dashFrames[dashName];
    bool showChart = dashFrame->viewTypeBox->currentText() == viewSettingsTableTypeChart;

    //Формируем заголовок таблицы
    QStringList tableHeaderItems;
    for (int column=0;column<dashFrame->tableWidget->columnCount();column++)
        tableHeaderItems.append(QString("<th style=\"font-size: 100%; font-family: times\">%1</th>").arg(dashFrame->tableWidget->horizontalHeaderItem(column)->text()));
    QString tableHeader;
    for(int column=0;column<tableHeaderItems.size();column++)
        tableHeader += tableHeaderItems[column];

    //Формируем список данных
    QStringList tableRows;
    for (int row=0;row<dashFrame->tableWidget->rowCount();row++)
    {
        QString tableRow;
        for (int column=0;column<dashFrame->tableWidget->columnCount();column++)
            tableRow+=QString("<td>%1</td>").arg(dashFrame->tableWidget->item(row, column)->text());
        tableRows.append(QString("<tr>%1</tr>").arg(tableRow));
    }

    //Фоткаем график (если есть)
    QString chartImg;
    if (dashFrame->chartView->isVisible())
    {
        //Печатаем в файл
        QPixmap *pixmap = new QPixmap(dashFrame->chartView->width(), dashFrame->chartView->height());
        QPainter painter(pixmap);
        dashFrame->chartView->render(&painter);
        painter.end();
        QImage image = pixmap->toImage();
        QByteArray chartData;
        QBuffer buffer(&chartData);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        //Формируем тег графика
        chartImg = QString("<img src=\"data:image/png;base64,%1\">\n").arg(QString(chartData.toBase64()));
    }

    //Формируем html
    QString html;
    if (showChart)
    {
        html = QString("<head >\n"
                       "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\n"
                       "<style>\n"
                       " table { width: 100%; border: 1px double black; border-collapse: collapse; }\n"
                       " th { text-align: left; border: 1px solid black; }\n"
                       " td { border: 1px solid black; }\n"
                       "</style>\n"
                       "</head>\n"
                       "<body>\n"
                       "<h1 style=\"font-size: 150%; font-family: times\">%1</h1>\n"
                       "%2\n"
                       "</body>")
               .arg(dashFrame->dashCaption)
               .arg(chartImg);
    }
    else
    {
        html = QString("<head >\n"
                       "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>\n"
                       "<style>\n"
                       " table { width: 100%; border: 1px double black; border-collapse: collapse; }\n"
                       " th { text-align: left; border: 1px solid black; }\n"
                       " td { border: 1px solid black; }\n"
                       "</style>\n"
                       "</head>\n"
                       "<body>\n"
                       "<h1 style=\"font-size: 150%; font-family: times\">%1</h1>\n"
                       "<table>\n"
                       "%2\n"
                       "%3\n"
                       "</table>\n"
                       "%4\n"
                       "</body>")
               .arg(dashFrame->dashCaption)
               .arg(tableHeader)
               .arg(tableRows.join("\n"))
               .arg(chartImg);
    }
    //Выдаем диалог выбора файла для сохранения
    QString fileTemplate = QString("%1/%2_%3.%4")
                           .arg(systemUtils->getLastDir(),
                                "dashboard",
                                QDateTime::currentDateTime().toString(DateTimeFormatForFile),
                                "html");
    QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                    ru("Выберите путь к файлу"),
                                                    fileTemplate,
                                                    ru("Файлы HTML (*.html);;Все файлы (*.*)"));;
    if (!fileName.isEmpty())
    {
        //Запоминаем последний открытый файл
        systemUtils->setLastFileName(fileName);

        QFile htmlFile(fileName);
        if (htmlFile.open(QIODevice::WriteOnly))
        {
            QTextStream out(&htmlFile);
            out.setCodec(systemUtils->getCodec());
            out << html;
            htmlFile.close();
        }
    }
}

void DashBoardWindow::saveToExcel(const QString &dashName)
{
#ifdef PL_USE_AX
    if (!dashFrames.contains(dashName))
        return;
    DashFrame *dashFrame = dashFrames[dashName];

    //Формируем заголовок таблицы
    QStringList tableHeaderItems;
    for (int column=0;column<dashFrame->tableWidget->columnCount();column++)
        tableHeaderItems.append(QString("<th style=\"font-size: 100%; font-family: times\">%1</th>").arg(dashFrame->tableWidget->horizontalHeaderItem(column)->text()));

    //Формируем excel
    // Создаем COM-объект Excel
    QAxObject* excel = new QAxObject("Excel.Application", this);
    if (excel->isNull() == false)
    {
        // Делаем окно Excel видимым на экране
        //excel->dynamicCall("SetVisible(bool)",true);
        // Создаем новую рабочую книгу
        QAxObject *workbooks = excel->querySubObject("Workbooks");
        QAxObject *workbook = workbooks->querySubObject("Add()");
        // QAxObject *workbook = workbooks->querySubObject("Open(const QString&)", fileNameXLSX.c_str());
        // Создаем новый рабочий лист и делаем его текущим
        QAxObject *sheets = workbook->querySubObject("Sheets");
        QAxObject *sheet = sheets->querySubObject("Item(1)");
        sheet->dynamicCall("Select()");
        // Устанавливаем размер шрифта по умолчанию
        //sheet->dynamicCall("SetStandartFontSize(double)", 9);
        // Ширина столбца заголовка
        QAxObject *row = sheet->querySubObject("Columns(int)", 1);
        row->setProperty("ColumnWidth", 30);
        // Вставляем заголовок таблицы
        QAxObject *cell = sheet->querySubObject("cells(1, 1)");
        cell->dynamicCall("Select()");
        QAxObject *font = cell->querySubObject("Font");
        font->setProperty("Size", 16);
        font->setProperty("Bold", true);
        cell->setProperty("Value", dashFrame->dashCaption);
        // Заголовки строк таблицы
        QString tableHeaderValue = 0;
        for(int column=0;column<tableHeaderItems.size();column++)
        {
            // Значение заголовка
            tableHeaderValue = dashFrame->tableWidget->horizontalHeaderItem(column)->text();
            // Вставляем заголовок столбца
            if (tableHeaderValue.size() > 0)
            {
                QAxObject *cell = sheet->querySubObject("cells(int, int)", 2, column+1);
                QAxObject *font = cell->querySubObject("Font");
                font->setProperty("Size", 12);
                font->setProperty("Bold", true);
                //cell->setProperty("HorizontalAlignment", -4108);
                cell->setProperty("VerticalAlignment", -4108);
                cell->setProperty("Value", tableHeaderValue);
            }
            // Ширина столбца
            QAxObject *row = sheet->querySubObject("Columns(int)", column+2);
            row->setProperty("ColumnWidth", 30);
            // Значение ячейки
            QString tableValue = 0;
            // Ячейки таблицы
            for (int row=0;row<dashFrame->tableWidget->rowCount();row++)
            {
                //dashFrame->tableWidget->topLevelItem(j)->text(i)
                if (tableHeaderValue.size() > 0)
                {
                    QAxObject *cell = sheet->querySubObject("cells(int, int)", row+3, column+1);
                    tableValue = dashFrame->tableWidget->item(row, column)->text();
                    cell->setProperty("NumberFormat", "@");
                    cell->setProperty("Value", tableValue);
                }
            }
        }
        // Левый верхний угол
        QAxObject *cell1 = sheet->querySubObject("cells(int, int)", 2, 1);
        // Правый нижний угол
        QAxObject *cell2 = sheet->querySubObject("cells(int, int)", dashFrame->tableWidget->rowCount()+2, tableHeaderItems.size());
        // Диапазон ячеек
        QAxObject *range = sheet->querySubObject("Range(const QVariant&, const QVariant&)",
                                                 QVariant(cell1->dynamicCall("Address()")),
                                                 QVariant(cell2->dynamicCall("Address()")));
        // Обрамление
        QAxObject *borders = range->querySubObject("Borders");
        borders->setProperty("LineStyle", 1);

        //Выдаем диалог выбора файла для сохранения
        QString fileTemplateXLSX = QString("%1/%2_%3.%4")
                                   .arg(systemUtils->getLastDir(),
                                        "dashboard",
                                        QDateTime::currentDateTime().toString(DateTimeFormatForFile),
                                        "xlsx");
        QString fileNameXLSX = QFileDialog::getSaveFileName(NULL,
                                                            ru("Выберите путь к файлу"),
                                                            fileTemplateXLSX,
                                                            ru("Лист Excel (*.xlsx);;Все файлы (*.*)"));;
        if (!fileNameXLSX.isEmpty())
        {
            //Запоминаем последний открытый файл
            systemUtils->setLastFileName(fileNameXLSX);

            fileNameXLSX = fileNameXLSX.replace("/", "\\");
            //excel->dynamicCall("SaveAs(const QString&)",fileNameXLSX);
            workbook->querySubObject("SaveAs(const QString&)", fileNameXLSX.toLocal8Bit());
            //workbook->querySubObject("SaveAs(const QString&)", "D:\\work\\test4.xlsx");
            // Делаем окно Excel видимым на экране
            excel->dynamicCall("SetVisible(bool)", true);
        }
        //        excel->dynamicCall("Quit()");
    }
#endif
}

void DashBoardWindow::saveToCSV(const QString &dashName)
{
    if (!dashFrames.contains(dashName))
        return;
    DashFrame *dashFrame = dashFrames[dashName];
    //Формируем заголовок таблицы
    QStringList tableHeaderItems;
    for (int column=0;column<dashFrame->tableWidget->columnCount();column++)
        tableHeaderItems.append(QString("<th style=\"font-size: 100%; font-family: times\">%1</th>").arg(dashFrame->tableWidget->horizontalHeaderItem(column)->text()));
    //Выдаем диалог выбора файла для сохранения
    QString fileTemplateCSV = QString("%1/%2_%3.%4")
                              .arg(systemUtils->getLastDir(),
                                   "dashboard",
                                   QDateTime::currentDateTime().toString(DateTimeFormatForFile),
                                   "csv");
    QString fileNameCSV = QFileDialog::getSaveFileName(NULL,
                                                       ru("Выберите путь к файлу"),
                                                       fileTemplateCSV,
                                                       ru("Файл CSV (*.csv);;Все файлы (*.*)"));;
    if (!fileNameCSV.isEmpty())
    {
        //Запоминаем последний открытый файл
        systemUtils->setLastFileName(fileNameCSV);

        QFile csvFile(fileNameCSV);
        if (csvFile.open(QIODevice::WriteOnly))
        {
            //Формируем CSV
            QTextStream csvTextStream(&csvFile);
            // Строка таблицы
            QStringList strList;
            // Вставляем заголовок таблицы
            strList << ""+dashFrame->dashCaption.simplified()+"";
            csvTextStream << strList.join( ";" )+"\n";
            strList.clear();
            strList << "" "";
            // Заголовки строк таблицы
            QString tableHeaderValue = 0;
            for(int column=0;column<tableHeaderItems.size();column++)
            {
                // Значение заголовка
                tableHeaderValue = dashFrame->tableWidget->horizontalHeaderItem(column)->text();
                strList << ""+tableHeaderValue+"";
            }
            csvTextStream << strList.join( ";" )+"\n";
            // Ячейки таблицы
            for (int row=0;row<dashFrame->tableWidget->rowCount();row++)
            {
                // Значение ячейки
                QString tableValue = 0;
                // Очищаем строку
                strList.clear();
                strList << "" "";
                // Заполняем строку
                for(int column=0;column<tableHeaderItems.size();column++)
                {
                    tableValue = dashFrame->tableWidget->item(row, column)->text();
                    strList << ""+tableValue+"";
                }
                csvTextStream << strList.join( ";" )+"\n";
            }
        }
        csvFile.close();
    }
}

void DashBoardWindow::dashSetupFilterButtonClicked()
{
    QToolButton *setupFilterButton = qobject_cast<QToolButton*>(sender());
    if (setupFilterButton)
        setupFilter(setupFilterButton->property("dashName").toString());
}

void DashBoardWindow::dashSetupAdditionalFilterButtonClicked()
{
    QToolButton *setupAdditionalFilterButton = qobject_cast<QToolButton*>(sender());
    if (setupAdditionalFilterButton)
        setupAdditionalFilter(setupAdditionalFilterButton->property("dashName").toString());
}

void DashBoardWindow::dashFrameSaveButtonHTMLClicked()
{
    QToolButton *saveButtonHTML = qobject_cast<QToolButton*>(sender());
    if (saveButtonHTML)
        saveToHTML(saveButtonHTML->property("dashName").toString());
}

void DashBoardWindow::dashFrameSaveButtonExcelClicked()
{
    QToolButton *saveButtonExcel = qobject_cast<QToolButton*>(sender());
    if (saveButtonExcel)
        saveToExcel(saveButtonExcel->property("dashName").toString());
}

void DashBoardWindow::dashFrameSaveButtonCSVClicked()
{
    QToolButton *saveButtonCSV = qobject_cast<QToolButton*>(sender());
    if (saveButtonCSV)
        saveToCSV(saveButtonCSV->property("dashName").toString());
}

void DashBoardWindow::viewTypeBoxCurrentIndexChanged(int index)
{
    Q_UNUSED(index);
    QComboBox *viewTypeBox = qobject_cast<QComboBox*>(sender());
    if (viewTypeBox)
        updateDashFrame(viewTypeBox->property("dashName").toString());
}

void DashBoardWindow::chartViewLoadFinished(bool ok)
{
    QWebEngineView *chartView = qobject_cast<QWebEngineView*>(sender());
    if (chartView && ok)
        updateChartView(chartView->property("dashName").toString());
}

void DashBoardWindow::on_showAllButton_clicked()
{
    //Устанавливаем флаг
    isChildVisible = !isChildVisible;
    ui->showAllButton->setText(isChildVisible?ru("Свернуть всё"):ru("Развернуть всё"));
    ui->showAllButton->setIcon(isChildVisible?QIcon(":/icons/icons/up.png"):QIcon(":/icons/icons/down.png"));
    //Теперь бегаем по всем фреймам и устанавливаем видимость дочерних фреймов
    QMap<QString,DashFrame*>::iterator i;
    for (i = dashFrames.begin(); i != dashFrames.end(); i++)
    {
        if (i.value()->mainFrame->isVisible())
            guiHelper->setFrameChildVisible(i.value()->mainFrame,
                                            isChildVisible);
    }
    this->update();
}

void DashBoardWindow::on_settingsToolButton_clicked()
{
    //Создаем диалог настроек отображения
    DashBoardSettingsDialog *dashBoardSettingsDialog = new DashBoardSettingsDialog();
    foreach(DashFrame *dashFrame, dashFrames.values())
        dashBoardSettingsDialog->addDashBoard(dashFrame->dashName,
                                              dashFrame->dashCaption,
                                              dashFrame->mainFrame->isVisible());
    if (dashBoardSettingsDialog->exec())
    {
        //Получаем видимость дашбордов

        QMap<QString,DashFrame*>::iterator i;
        for (i = dashFrames.begin(); i != dashFrames.end(); i++)
        {
            //Прячем или показываем дашборды
            bool dashFrameIsVisible = dashBoardSettingsDialog->isDashBoardVisible(i.value()->dashName);
            i.value()->mainFrame->setVisible(dashFrameIsVisible);
            if ( i.value()->childFrame->isVisible())
                i.value()->childFrame->setVisible(dashFrameIsVisible);
        }
    }
    //Удаляем диалог
    delete dashBoardSettingsDialog;
}

void DashBoardWindow::on_helpButton_clicked()
{
    showHelp();
}
