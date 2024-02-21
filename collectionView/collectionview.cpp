#include "collectionview.h"
#include "objectguihelper.h"
#include "flowlayout.h"

#include <guihelper.h>
#include <objectdatahelper.h>

#include <QPropertyAnimation>
#include <QScrollArea>
#include <QScrollBar>

const QString defaultItemType = QString(); //!< Значение по умолчанию типа элемента
const QString defaultLayoutType = QString(); //!< Значение по умолчанию типа компоновщика
const QString defaultCollectionObjectName = "baseCollectionView"; //!< Значение интервала по умолчанию

CollectionView::CollectionView(const ObjectPropertyList &_properties)
{
    styleName = QString();
    //Забираем свойство имени файла стиля
    ObjectQVariantPropertyPointer styleProperty = findProperty<ObjectQVariantProperty>(
                                                      _properties,
                                                      objectStyleSheetPropertyName);
    if (styleProperty)
        styleName  = styleProperty->value.toString();

    //Забираем свойство типа меню
    ObjectQVariantPropertyPointer menuTypeProperty = findProperty<ObjectQVariantProperty>(
                                                         _properties,
                                                         objectMenuTypePropertyName);
    menuType = (menuTypeProperty
                ? menuTypeProperty->value.toString()
                : QString());
    
    menuSubsystemId = 0;

    //Формируем виджет коллекции
    mainArea = Q_NULLPTR;
    mainCollectionViewFrame = new QFrame();
    mainLayout = new QHBoxLayout(mainCollectionViewFrame);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    //Фрейм для кнопки скрыть
    closeBtnFrame = new QFrame();
    closeBtnFrame->setObjectName("closeBtnFrame");
    closeBtnFrameLayout = new QHBoxLayout(closeBtnFrame);
    closeBtnFrameLayout->setSpacing(0);
    closeBtnFrameLayout->setMargin(0);
    closeBtnFrameLayout->setContentsMargins(6, 0, 0, 0);
    closeBtnFrameLayout->setAlignment(Qt::AlignRight);
    closeBtn = Q_NULLPTR;
    collectionNameLabel = Q_NULLPTR;

    //Синяя полоска в в выпадающей коллекции
    styledLineFrame = new QFrame();
    commonFrame = new QFrame;
    commonFrameLayout = new QVBoxLayout(commonFrame);
    commonFrameLayout->setMargin(0);
    commonFrameLayout->setSpacing(0);
    //Фрейм элементов коллекции
    contentFrame = new QFrame();
    commonFrameLayout->addWidget(closeBtnFrame);
    commonFrameLayout->addWidget(contentFrame);
    styledLineFrame->setFixedWidth(5);
    styledLineFrame->setObjectName("styledLineFrame");
    mainLayout->addWidget(commonFrame);

    //Инициализируем и чистим все переменные
    menuItems.clear();
    menuItemsObjectWidgets.clear();
    mainCollectionViewFrameLayout = Q_NULLPTR;
    elementType = QString();
    layoutType = QString();
    collectionIsMinimized = false;
    collectionIsFavorites = false;
    itemsCaptionsVisible = true;
    currentMenuItem = -1;
    currentMenuItemCoordinate = -1;
    leftMenuCollectionItems.clear();
    currentCollectionItem = Q_NULLPTR;
    currentCollectionFrame = Q_NULLPTR;

    //Устанавливаем свойства
    setProperties(_properties);
}

CollectionView::~CollectionView()
{
    mainCollectionViewFrame->deleteLater();
}

bool CollectionView::eventFilter(QObject *obj,
                                 QEvent *event)
{
    //Обработчик клика мышкой по виджету коллекции
    if (event->type() == QEvent::MouseButtonPress)
    {
        //Обрабатываем только нажатие левой кнопки мыши
        auto mouseEvent = safe_dynamic_cast<QMouseEvent *>(event);
        if (!mouseEvent
            || mouseEvent->button() != Qt::LeftButton)
        {
            return true;
        }

        //Если кликнули по элементу коллекции
        auto collectionViewItemFrame = qobject_cast<QFrame *>(obj);
        if (collectionViewItemFrame)
        {
            QVariantMap elementInfoMap = collectionViewItemFrame->property("itemProperties").toMap();
            if (mouseEvent->modifiers() & Qt::ControlModifier)
                elementInfoMap["hasCtrl"] = true;
            bool isGroupElement = (elementInfoMap.contains("isGroupElement"))
                                  ? elementInfoMap["isGroupElement"].toBool()
                                  : false;
            bool isDropDownMenu = (elementType == ru("ЭлементВыпадающегоМеню"));
            if (isGroupElement
                && isDropDownMenu)
                return true;

            //Зачистим текущий виджет
            currentCollectionItem = Q_NULLPTR;
            if (menuType == menuTypeSubsystemsTree && !collectionIsMinimized)
            {
                //Ищем текущий виджет в списке
                currentCollectionItem = findCurrentCollectionItem(collectionViewItemFrame);
            }

            //Запомним, если надо, верхний фрейм в коллекции
            if (menuType == menuTypeSubsystemsTree && !isDropDownMenu)
            {
                if (collectionIsMinimized)
                {
                    //Снимем выделение с прошлого
                    if (currentCollectionFrame != Q_NULLPTR)
                    {
                        //Снимем выделение
                        currentCollectionFrame->setProperty("menuSelected", false);
                        currentCollectionFrame->setProperty("showIndicator", false);
                        styleHelper->setStyleSheet(this->getWidget(), styleName);
                    }
                    //Запомним новый
                    currentCollectionFrame = collectionViewItemFrame;
                }
                else if (currentCollectionItem != Q_NULLPTR)
                {
                    if (currentCollectionItem->level == 1)
                        currentCollectionFrame = collectionViewItemFrame;
                }
            }

            if (currentCollectionItem != Q_NULLPTR)
            {
                //Если подсистема уже открыта
                if (currentCollectionItem->isOpen && currentCollectionItem->childs.length() > 0)
                {
                    //в цикле удаляем лишние виджеты
                    for (int j = 0; j < currentCollectionItem->childs.length(); ++j)
                        clearChildsCollectionItem(currentCollectionItem->childs[j]);

                    //почистим данные фиджета
                    currentCollectionItem->childs.clear();
                    currentCollectionItem->isOpen = false;

                    //Снимем выделение
                    currentCollectionItem->itemObjectFrame->setProperty("menuSelected", false);
                    currentCollectionItem->itemObjectFrame->setProperty("showIndicator", false);
                    styleHelper->setStyleSheet(this->getWidget(), styleName);

                    //Завершаем обработку события
                    return true;
                }
                else
                {
                    //Для режима дерево закрываем все остальные субсистемы
                    if (menuType == menuTypeSubsystemsTree && currentCollectionItem != Q_NULLPTR)
                    {
                        //получаем родителя
                        CollectionItem *parentItem = currentCollectionItem->parent;
                        if (parentItem == Q_NULLPTR)
                        {
                            //идем по детям
                            for (int i = 0; i < leftMenuCollectionItems.length(); ++i)
                                //если открыт - закрываем
                                if (leftMenuCollectionItems[i]->isOpen)
                                {
                                    for (int j = 0; j < leftMenuCollectionItems[i]->childs.length(); ++j)
                                        clearChildsCollectionItem(leftMenuCollectionItems[i]->childs[j]);

                                    //почистим данные фиджета
                                    leftMenuCollectionItems[i]->childs.clear();
                                    leftMenuCollectionItems[i]->isOpen = false;

                                    //Снимем выделение
                                    leftMenuCollectionItems[i]->itemObjectFrame->setProperty("menuSelected", false);
                                    leftMenuCollectionItems[i]->itemObjectFrame->setProperty("showIndicator", false);
                                    styleHelper->setStyleSheet(this->getWidget(), styleName);
                                }
                        }
                        else
                        {
                            //идем по детям
                            for (int i = 0; i < parentItem->childs.length(); ++i)
                                //если открыт - закрываем
                                if (parentItem->childs[i]->isOpen)
                                {
                                    for (int j = 0; j < parentItem->childs[i]->childs.length(); ++j)
                                        clearChildsCollectionItem(parentItem->childs[i]->childs[j]);

                                    //почистим данные фиджета
                                    parentItem->childs[i]->childs.clear();
                                    parentItem->childs[i]->isOpen = false;

                                    //Снимем выделение
                                    parentItem->childs[i]->itemObjectFrame->setProperty("menuSelected", false);
                                    parentItem->childs[i]->itemObjectFrame->setProperty("showIndicator", false);
                                    styleHelper->setStyleSheet(this->getWidget(), styleName);
                                }
                        }

                        QVariant purpose = collectionViewItemFrame->property("itemProperties").toMap()["purpose"];
                        if (purpose == subsystemObject)
                            currentCollectionItem->itemObjectFrame->setProperty("menuSelected", true);
                        else
                            currentCollectionItem->itemObjectFrame->setProperty("menuSelected", false);

                        if (purpose != procedureObject)
                            currentCollectionItem->itemObjectFrame->setProperty("showIndicator", true);
                        else
                            currentCollectionItem->itemObjectFrame->setProperty("showIndicator", false);
                        styleHelper->setStyleSheet(this->getWidget(), styleName);
                    }
                    //Задаем коллекциии свойство назначения выбранного элемента(нужно в скрипте главного окна с выпадающим меню).
                    //Если не подсистема - меню остается на прежнем выделенном объекте и никуда не двигается
                    objectGuiHelper->setObjectProperty(this,
                                                       "purpose",
                                                       elementInfoMap.value("purpose"));
                    //Запоминаем координату верхнего левого угла выбранного меню,нужна чтобы напротив меню вывести всплывающее
                    currentMenuItemCoordinate = collectionViewItemFrame->mapToParent(QPoint(0,0)).y();

                    //Выдаем сигнал выбора элемента
                    emit menuSelected(elementInfoMap);

                    //Выдаем сигнал отображения коллекции
                    emit showCollection();

                    //Завершаем обработку события
                    return true;
                }
            }
            else
            {
                //Задаем коллекциии свойство назначения выбранного элемента(нужно в скрипте главного окна с выпадающим меню).
                //Если не подсистема - меню остается на прежнем выделенном объекте и никуда не двигается
                objectGuiHelper->setObjectProperty(this,
                                                   "purpose",
                                                   elementInfoMap.value("purpose"));
                QVariantMap elementInfoMapProperties = elementInfoMap["properties"].toMap();
                objectGuiHelper->setObjectProperty(this,
                                                   "isDependingOnDict",
                                                   elementInfoMapProperties.value("isDependingOnDict", false).toBool());
                //Запоминаем координату верхнего левого угла выбранного меню,нужна чтобы напротив меню вывести всплывающее
                currentMenuItemCoordinate = collectionViewItemFrame->mapToParent(QPoint(0,0)).y();

                QString purpose = elementInfoMap.value("purpose").toString();
                if (purpose == subsystemObject
                    || elementInfoMapProperties.contains("dependingOnDictGroupId"))
                    emit allowFooterChangeForSystem(elementInfoMap);

                //Выдаем сигнал выбора элемента
                emit menuSelected(elementInfoMap);

                //Выдаем сигнал отображения коллекции
                emit showCollection();

                //Завершаем обработку события
                return true;
            }
        }
    }
    if (mainArea)
    {
        if (obj == qobject_cast<QObject *>(mainArea))
        {
            if (event->type() == QEvent::Wheel)
            {
                int angleDelta = safe_dynamic_cast<QWheelEvent *>(event)->angleDelta().y();
                int koef = angleDelta>=0?-1:1;
                QScrollBar *scrollbarHor = mainArea->horizontalScrollBar();
                if (scrollbarHor)
                    scrollbarHor->setValue(scrollbarHor->value() + scrollbarHor->pageStep()/2*koef);
            }
        }
    }
    // standard event processing
    return ObjectWidget::eventFilter(obj, event);
}

QString CollectionView::getObjectClassAlias() const
{
    return ru("ФреймКоллекции");
}

ObjectPropertyList CollectionView::getProperties() const
{
    ObjectPropertyList objectProperties = ObjectWidget::getProperties();
    QString groupName = getObjectClassAlias();
    QVariantList menuItemsList;
    QStringList usedItemTemplates;
    usedItemTemplates << ru("ЭлементКоллекцииЛевогоМеню")
                      << ru("ЭлементКоллекцииЦентральнойОбласти")
                      << ru("ЭлементВыпадающегоМеню");
    ObjectQVariantProperty::addOrEditProperty(this,
                                              objectProperties,
                                              collectionViewItemTemplatePropertyName,
                                              elementType,
                                              ru("Используемый тип элемента"),
                                              defaultItemType,
                                              groupName,
                                              listFieldEdit,
                                              usedItemTemplates);
    QStringList usedLayoutType;
    usedLayoutType << ru("Текучий")
                   << ru("Горизонтальный")
                   << ru("Вертикальный");
    ObjectQVariantProperty::addOrEditProperty(this,
                                              objectProperties,
                                              collectionViewLayoutTypePropertyName,
                                              layoutType,
                                              ru("Тип используемого компоновщика"),
                                              defaultLayoutType,
                                              groupName,
                                              listFieldEdit,
                                              usedLayoutType);

    if (elementType == ru("ЭлементКоллекцииЛевогоМеню")
        || elementType == ru("ЭлементКоллекцииЦентральнойОбласти"))
    {
        //Формируем список необходимых для элементов свойств
        QStringList menuItemMainPropertiesNames;
        menuItemMainPropertiesNames << QString("text")
                                    << QString("execMethod")
                                    << QString("purpose")
                                    << QString("param");
        if (elementType == ru("ЭлементКоллекцииЦентральнойОбласти"))
            menuItemMainPropertiesNames << QString("addMethod");

        //Дополняем список menuItemsList
        foreach (const ObjectWidgetPointer &menuObjectWidget, menuItemsObjectWidgets)
        {
            if (!menuObjectWidget)
                continue;

            //Заполняем свойства
            QVariantMap menuItemMap;
            for (const auto &menuItemMainPropertyName : menuItemMainPropertiesNames)
                menuItemMap[menuItemMainPropertyName] = menuObjectWidget->getWidget()->property(menuItemMainPropertyName.toUtf8());

            //Заполняем свойства из свойств виджета
            ObjectPropertyList menuItemObjectWidgetProperies = menuObjectWidget->getProperties();

            ObjectQVariantPropertyPointer imagePathProperty = findProperty<ObjectQVariantProperty>(
                                                                  menuItemObjectWidgetProperies,
                                                                  imagePathPropertyName);
            if (imagePathProperty)
            {
                menuItemMap["imagePath"] = imagePathProperty->value;
            }
            else
            {
                ObjectQVariantPropertyPointer imageProperty = findProperty<ObjectQVariantProperty>(
                                                                  menuItemObjectWidgetProperies,
                                                                  frameImagePropertyName);
                if (imageProperty)
                    menuItemMap["image"] = imageProperty->value;
            }

            menuItemsList << menuItemMap;
        }
    }

    ObjectQVariantPropertyPointer menuItemsProperty = findProperty<ObjectQVariantProperty>(
                                                          objectProperties,
                                                          collectionViewItemsPropertyName);
    if (!menuItemsProperty)
    {
        //Свойство не найдено, добавляем
        menuItemsProperty = createProperty<ObjectQVariantProperty>(
                                collectionViewItemsPropertyName,
                                menuItemsList,
                                true);
        objectProperties << menuItemsProperty;
    }
    ObjectQVariantProperty::addOrEditProperty(this,
                                              objectProperties,
                                              collectionObjectNamePropertyName,
                                              mainCollectionViewFrame->objectName(),
                                              ru("Имя фрейма коллекции"),
                                              defaultCollectionObjectName,
                                              groupName,
                                              textFieldEdit);
    return objectProperties;
}

QWidget *CollectionView::getWidget() const
{
    return mainCollectionViewFrame;
}

void CollectionView::refreshWidget(const QVariantMap &params)
{
    ObjectWidget::refreshWidget(params);

    ObjectQVariantPropertyPointer collectionObjectNameProperty = findProperty<ObjectQVariantProperty>(
                                                                     properties,
                                                                     collectionObjectNamePropertyName,
                                                                     getObjectName());
    if (collectionObjectNameProperty)
        mainCollectionViewFrame->setObjectName(collectionObjectNameProperty->value.toString());
    ObjectQVariantPropertyPointer itemTypeProperty = findProperty<ObjectQVariantProperty>(
                                                         properties,
                                                         collectionViewItemTemplatePropertyName,
                                                         getObjectName());
    if (itemTypeProperty)
        elementType = itemTypeProperty->value.toString();
    ObjectQVariantPropertyPointer layoutTypeProperty = findProperty<ObjectQVariantProperty>(
                                                           properties,
                                                           collectionViewLayoutTypePropertyName,
                                                           getObjectName());
    if (layoutTypeProperty)
        layoutType = layoutTypeProperty->value.toString();
    if (itemTypeProperty)
        elementType = itemTypeProperty->value.toString();

    if (elementType == ru("ЭлементКоллекцииЛевогоМеню"))
    {
        if (mainCollectionViewFrameLayout)
        {
            if (!menuItemsObjectWidgets.isEmpty())
            {
                qDeleteAll(menuItemsObjectWidgets);
                menuItemsObjectWidgets.clear();
            }
            if (!menuItems.isEmpty())
            {
                qDeleteAll(menuItems);
                menuItems.clear();
            }
        }
        else
        {
            if (layoutType == ru("Горизонтальный"))
            {
                mainCollectionViewFrameLayout = new QHBoxLayout(contentFrame);
            }
            else if (layoutType == ru("Вертикальный")
                     || layoutType.isEmpty())
            {
                if (menuType == menuTypeSubsystemsTree)
                {
                    commonFrameLayout->removeWidget(contentFrame);
                    //Добавим скролл
                    auto leftScrollArea = new QScrollArea();
                    leftScrollArea->setObjectName("leftCollectionScrollArea");
                    leftScrollArea->verticalScrollBar()->setProperty("menuItem", true);
                    leftScrollArea->setWidgetResizable(true);
                    leftScrollArea->setContentsMargins(0,0,0,0);
                    leftScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    leftScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                    leftScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                    styleHelper->setStyleSheet(leftScrollArea, "mainWindowComponent");
                    mainCollectionViewFrameLayout = new QVBoxLayout(contentFrame);
                    mainCollectionViewFrameLayout->setSpacing(0);
                    mainCollectionViewFrameLayout->setMargin(0);
                    leftScrollArea->setWidget(contentFrame);
                    commonFrameLayout->addWidget(leftScrollArea);
                }
                else
                {
                    mainCollectionViewFrameLayout = new QVBoxLayout(contentFrame);
                }
            }
            else if (layoutType == ru("Текучий"))
            {
                mainCollectionViewFrameLayout = new FlowLayout(contentFrame);
                mainCollectionViewFrameLayout->setSpacing(0);
                mainCollectionViewFrameLayout->setMargin(0);
                auto flow = qobject_cast<FlowLayout *>(mainCollectionViewFrameLayout);
                if (flow)
                {
                    flow->setMaxColumnWidth(270);
                    flow->setHorizontalSpacing(0);
                    flow->setVerticalSpacing(0);
                }
                mainCollectionViewFrame->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
            }
            mainCollectionViewFrameLayout->setMargin(0);
        }
    }
    else if (elementType == ru("ЭлементКоллекцииЦентральнойОбласти"))
    {
        if (mainCollectionViewFrameLayout)
        {
            if (!menuItemsObjectWidgets.isEmpty())
            {
                qDeleteAll(menuItemsObjectWidgets);
                menuItemsObjectWidgets.clear();
            }
            if (!menuItems.isEmpty())
            {
                qDeleteAll(menuItems);
                menuItems.clear();
            }
        }
        else
        {
            if (layoutType == ru("Горизонтальный"))
            {
                mainCollectionViewFrameLayout = new QHBoxLayout(contentFrame);
            }
            else if (layoutType == ru("Вертикальный"))
            {
                mainCollectionViewFrameLayout = new QVBoxLayout(contentFrame);
            }
            else if (layoutType == ru("Текучий")||layoutType.isEmpty())
            {
                mainCollectionViewFrameLayout = new FlowLayout(contentFrame);
                mainCollectionViewFrameLayout->setMargin(0);
                mainCollectionViewFrame->setObjectName("centralCollectionFrame");
            }
        }
    }
    else if (elementType == ru("ЭлементВыпадающегоМеню"))
    {
        if (mainCollectionViewFrameLayout)
        {
            if (!menuItemsObjectWidgets.isEmpty())
            {
                qDeleteAll(menuItemsObjectWidgets);
                menuItemsObjectWidgets.clear();
            }
            if (!menuItems.isEmpty())
            {
                qDeleteAll(menuItems);
                menuItems.clear();
            }
        }
        else
        {
            mainLayout->insertWidget(0,styledLineFrame);
            if (layoutType == ru("Горизонтальный"))
            {
                mainCollectionViewFrameLayout = new QHBoxLayout(contentFrame);
            }
            else if (layoutType == ru("Вертикальный")||layoutType.isEmpty())
            {
                mainCollectionViewFrameLayout = new QVBoxLayout(contentFrame);
            }
            else if (layoutType == ru("Текучий"))
            {
                commonFrameLayout->removeWidget(contentFrame);
                mainArea = new QScrollArea();
                mainArea->setObjectName("droppedCollectionScrollArea");
                mainArea->setContentsMargins(0,0,0,0);
                mainArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                mainArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                mainCollectionViewFrameLayout = new FlowLayout(contentFrame);
                mainCollectionViewFrameLayout->setMargin(0);
                mainCollectionViewFrameLayout->setSpacing(0);
                auto flow = qobject_cast<FlowLayout *>(mainCollectionViewFrameLayout);
                if (flow)
                {
                    flow->setDirections(Qt::Vertical);
                    flow->setHorizontalSpacing(0);
                    flow->setVerticalSpacing(0);
                }
                mainCollectionViewFrame->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
                mainArea->setWidget(contentFrame);
                mainArea->installEventFilter(this);
                commonFrameLayout->addWidget(mainArea);
            }
        }
    }

    ObjectWidget::refreshWidgetFinished();
}

QVariant CollectionView::getValue(const QString &name,
                                  const QString &param) const
{
    if (name == "favoritesVisible")
        return collectionIsFavorites;
    if (name == "collapsed")
        return collectionIsMinimized;
    if (name == "menuItemsObjectWidgets")
    {
        QVariantList result;
        for (ObjectWidgetPointer menuItemObjectWidget : menuItemsObjectWidgets)
            result << QVariant::fromValue<ObjectWidget *>(menuItemObjectWidget);

        return result;
    }

    return ObjectWidget::getValue(name,
                                  param);
}

void CollectionView::changeCollectionIsMinimizedFlag()
{
    //Просто меняем флаг на противоположный
    collectionIsMinimized = !collectionIsMinimized;
    if (collectionNameLabel)
        collectionNameLabel->setVisible(collectionIsMinimized);

    if (currentCollectionItem != Q_NULLPTR
        && collectionIsMinimized)
    {
        //идем по детям
        for (int i = 0; i < leftMenuCollectionItems.length(); ++i)
        {
            if (!leftMenuCollectionItems[i]->isOpen)
                continue;

            //если открыт - закрываем
            for (int j = 0; j < leftMenuCollectionItems[i]->childs.length(); ++j)
                clearChildsCollectionItem(leftMenuCollectionItems[i]->childs[j]);

            //почистим данные фиджета
            leftMenuCollectionItems[i]->childs.clear();
            leftMenuCollectionItems[i]->isOpen = false;
            currentCollectionItem = Q_NULLPTR;

            QFrame *collectionViewItemFrame = leftMenuCollectionItems[i]->itemObjectFrame;
            QVariantMap elementInfoMap = collectionViewItemFrame->property("itemProperties").toMap();

            //Задаем коллекциии свойство назначения выбранного элемента(нужно в скрипте главного окна с выпадающим меню).
            //Если не подсистема - меню остается на прежнем выделенном объекте и никуда не двигается
            objectGuiHelper->setObjectProperty(this,
                                               "purpose",
                                               elementInfoMap.value("purpose"));
            QVariantMap elementInfoMapProperties = elementInfoMap["properties"].toMap();
            objectGuiHelper->setObjectProperty(this,
                                               "isDependingOnDict",
                                               elementInfoMapProperties.value("isDependingOnDict", false).toBool());
            //Запоминаем координату верхнего левого угла выбранного меню,нужна чтобы напротив меню вывести всплывающее
            currentMenuItemCoordinate = collectionViewItemFrame->mapToParent(QPoint(0,0)).y();

            //Выдаем сигнал выбора элемента
            emit menuSelected(collectionViewItemFrame->property("itemProperties"));
        }
        currentCollectionItem = Q_NULLPTR;
    }
    else if (menuType == menuTypeSubsystemsTree
             && !collectionIsMinimized
             && currentCollectionFrame != Q_NULLPTR)
    {
        QFrame *collectionViewItemFrame = currentCollectionFrame;
        QVariantMap elementInfoMap = collectionViewItemFrame->property("itemProperties").toMap();
        bool isGroupElement = (elementInfoMap.contains("isGroupElement"))
                              ? elementInfoMap["isGroupElement"].toBool()
                              : false;
        bool isDropDownMenu = (elementType == ru("ЭлементВыпадающегоМеню"));
        if (isGroupElement
            && isDropDownMenu)
            return;

        //Ищем текущий виджет в списке
        currentCollectionItem = findCurrentCollectionItem(collectionViewItemFrame);

        //Задаем коллекциии свойство назначения выбранного элемента(нужно в скрипте главного окна с выпадающим меню).
        //Если не подсистема - меню остается на прежнем выделенном объекте и никуда не двигается
        objectGuiHelper->setObjectProperty(this,
                                           "purpose",
                                           elementInfoMap.value("purpose"));
        //Запоминаем координату верхнего левого угла выбранного меню,нужна чтобы напротив меню вывести всплывающее
        currentMenuItemCoordinate = collectionViewItemFrame->mapToParent(QPoint(0,0)).y();
        //Выдаем сигнал выбора элемента
        emit menuSelected(collectionViewItemFrame->property("itemProperties"));
        //Выдаем сигнал отображения коллекции
        emit showCollection();
    }
}

void CollectionView::changeCollectionIsFavoritesFlag()
{
    //Просто меняем флаг на противоположный
    collectionIsFavorites = !collectionIsFavorites;

    //Очищаем всё что необходимо
    currentCollectionItem = Q_NULLPTR;
    currentCollectionFrame = Q_NULLPTR;
    currentMenuItem = -1;
    //Если в коллекции уже что-то было, то очищаем
    if (mainCollectionViewFrameLayout)
    {
        if (!menuItemsObjectWidgets.isEmpty())
        {
            qDeleteAll(menuItemsObjectWidgets);
            menuItemsObjectWidgets.clear();
        }
        if (!menuItems.isEmpty())
        {
            qDeleteAll(menuItems);
            menuItems.clear();
        }
        if (closeBtn)
        {
            closeBtn->deleteLater();
            closeBtn = Q_NULLPTR;

            collectionNameLabel->deleteLater();
            collectionNameLabel = Q_NULLPTR;
        }
    }
    //Удалим пружинку в конец списка
    if (menuType == menuTypeSubsystemsTree)
    {
        auto vboxlayout = qobject_cast<QVBoxLayout *>(mainCollectionViewFrameLayout);
        if (vboxlayout)
            delete vboxlayout->takeAt(vboxlayout->count() - 1);
    }
    //идем по детям
    for (int i = 0; i < leftMenuCollectionItems.length(); ++i)
    {
        //если открыт - закрываем
        if (leftMenuCollectionItems[i]->isOpen)
        {
            for (int j = 0; j < leftMenuCollectionItems[i]->childs.length(); ++j)
                clearChildsCollectionItem(leftMenuCollectionItems[i]->childs[j]);

            //почистим данные фиджета
            leftMenuCollectionItems[i]->childs.clear();
        }
        //Удаляем объект
        delete leftMenuCollectionItems[i]->itemObjectWidget;
    }
    leftMenuCollectionItems.clear();

    //Выдаем сигнал выбора элемента
    if (!collectionIsFavorites)
    {
        emit menuSelectedDefault();
        return;
    }

    //Забираем информацию об объекте
    ObjectHelper::ObjectInfo favoritesObjectInfo = objectHelper->getObjectByAlias(ru("Избранное"),
                                                                                  ObjectHelper::ObjectInfo(dictTableObject),
                                                                                  ObjectHelper::ObjectInfo(dictTablesObject));
    //Получаем список свойств справочника
    QVariantMap favoritesInfoMap = favoritesObjectInfo.properties;
    //Заполняем map дополнительных свойств
    QVariantMap propertiesInfoMap = favoritesInfoMap["properties"].toMap();
    propertiesInfoMap["isDependingOnDict"] = true;
    propertiesInfoMap["dependingOnDictDictMetaAlias"] = ru("Избранное");
    propertiesInfoMap["elementCaptionFieldMetaAlias"] = ru("Наименование");
    propertiesInfoMap["elementAliasFieldMetaAlias"] = ru("ПсевдонимОбъекта");
    propertiesInfoMap["isGroupElementFieldMetaAlias"] = ru("ПризнакГруппы");
    propertiesInfoMap["groupElementIdFieldMetaAlias"] = ru("ИдентификаторРодительскойЗаписи");
    propertiesInfoMap["objectParentPurposeFieldMetaAlias"] = ru("ТипРодительскогоОбъекта");
    propertiesInfoMap["objectParentIdFieldMetaAlias"] = ru("КодРодительскогоОбъекта");
    propertiesInfoMap["objectPurposeFieldMetaAlias"] = ru("ТипОбъекта");
    propertiesInfoMap["objectIdFieldMetaAlias"] = ru("КодОбъекта");
    //Дополнительный фильтр по пользователю
    if (globalProjectSettings.getUser().id != 0)
        propertiesInfoMap["dictWhereCondition"] = QString("Избранное.Пользователь = %1")
                                                  .arg(globalProjectSettings.getUser().id);
    else
        propertiesInfoMap["dictWhereCondition"] = QString("Избранное.Пользователь IS NULL");

    favoritesInfoMap["param"] = favoritesObjectInfo.objectId;
    favoritesInfoMap["purpose"] = favoritesObjectInfo.objectPurpose;
    favoritesInfoMap["properties"] = propertiesInfoMap;
    favoritesInfoMap["execMethod"] = objectHelper->fillObjectAlias(favoritesObjectInfo);
    favoritesInfoMap["text"] = ru("Избранное");
    favoritesInfoMap["toolTip"] = ru("Избранное");

    //Выдаем сигнал выбора элемента
    emit menuSelectedFavorites(favoritesInfoMap);


    if (!favoritesNotificationConnect)
    {
        //Подключаемся на изменение справочника "Избранное"
        favoritesNotificationConnect = connect(dbUtils, SIGNAL(notification(QString,QSqlDriver::NotificationSource,QVariantMap)),
                                               this, SLOT(notificationHandlerForFavorites(QString,QSqlDriver::NotificationSource,QVariantMap)));

        //Подписываемся на событие изменения версии
        dbUtils->subscribeToNotification(versionChangedNotify);
    }
}

void CollectionView::notificationHandlerForFavorites(const QString &name,
                                                     QSqlDriver::NotificationSource source,
                                                     const QVariantMap &payload)
{
    Q_UNUSED(source)

    //Обрабатываем только уведомления от справочника "Избранное"
    if (name != versionChangedNotify
        || payload["alias"].toString() != ru("Избранное"))
        return;

    //Если меню "Избранное" не отображено, то просто выходим
    if (!collectionIsFavorites)
        return;

    //Перегружаем меню "Избранное"
    collectionIsFavorites = false;
    changeCollectionIsFavoritesFlag();
}

void CollectionView::showItemsCaptions()
{
    Debug(Log::ErrorMessages) << "Method CollectionView::showItemsCaptions is DEPRECATED"
                              << "Use CollectionView::setItemsCaptionsVisible instead.";
    setItemsCaptionsVisible(!itemsCaptionsVisible);
}

bool CollectionView::getItemsCaptionsVisible()
{
    return itemsCaptionsVisible;
}

void CollectionView::setItemsCaptionsVisible(bool visible)
{
    itemsCaptionsVisible = visible;

    if (layoutType == ru("Текучий"))
    {
        auto flow = qobject_cast<FlowLayout *>(mainCollectionViewFrameLayout);
        if (flow)
        {
            flow->setMaxColumnWidth((collectionIsMinimized)
                                    ? 40
                                    : 270);
        }
    }

    //Скрываем или отображаем подписи менюшек
    for (const ObjectWidgetPointer &menuObjectWidget : menuItemsObjectWidgets)
    {
        if (!menuObjectWidget)
            continue;

        for (const ObjectWidgetPointer &childObjectWidget : menuObjectWidget->getAllChildObjectWidgets())
        {
            if (!childObjectWidget
                || childObjectWidget->getObjectName() != ru("ПодписьЭлементаЛевогоМеню"))
                continue;

            //Получаем свойство видимости и оперируем им
            ObjectPropertyList objectWidgetProperies = childObjectWidget->getProperties();
            ObjectQVariantPropertyPointer visibleProperty = findProperty<ObjectQVariantProperty>(
                                                                objectWidgetProperies,
                                                                visiblePropertyName);
            if (!visibleProperty)
                continue;

            visibleProperty->value = visible;
            childObjectWidget->setProperties(objectWidgetProperies);
            childObjectWidget->refreshWidget();
        }
    }

    //Если режим не дерева: выходим
    if (menuType != menuTypeSubsystemsTree)
        return;

    //идем по верхнему уровню
    for (CollectionItem *leftMenuCollectionItem : leftMenuCollectionItems)
    {
        if (!leftMenuCollectionItem->itemObjectWidget)
            continue;

        for (const ObjectWidgetPointer &childObjectWidget : leftMenuCollectionItem->itemObjectWidget->getAllChildObjectWidgets())
        {
            if (!childObjectWidget
                || childObjectWidget->getObjectName() != ru("ПодписьЭлементаЛевогоМеню"))
                continue;

            //Получаем свойство видимости и оперируем им
            ObjectPropertyList objectWidgetProperies = childObjectWidget->getProperties();
            ObjectQVariantPropertyPointer visibleProperty = findProperty<ObjectQVariantProperty>(
                                                                objectWidgetProperies,
                                                                visiblePropertyName);
            if (!visibleProperty)
                continue;

            visibleProperty->value = visible;
            childObjectWidget->setProperties(objectWidgetProperies);
            childObjectWidget->refreshWidget();
        }
    }
}

bool CollectionView::getCollectionMinimizedFlag()
{
    return collectionIsMinimized;
}

void CollectionView::fillCollection(const QVariantList &items,
                                    const QVariantMap &params)
{
    //Если в коллекции уже что-то было, то очищаем
    if (mainCollectionViewFrameLayout)
    {
        if (!menuItemsObjectWidgets.isEmpty())
        {
            qDeleteAll(menuItemsObjectWidgets);
            menuItemsObjectWidgets.clear();
        }
        if (!menuItems.isEmpty())
        {
            qDeleteAll(menuItems);
            menuItems.clear();
        }
        if (closeBtn)
        {
            closeBtn->deleteLater();
            closeBtn = Q_NULLPTR;

            collectionNameLabel->deleteLater();
            collectionNameLabel = Q_NULLPTR;
        }
    }

    //Вставим фрейм с кнопкой закрывашки
    if (elementType == ru("ЭлементВыпадающегоМеню"))
    {
        collectionNameLabel = new QLabel(items.value(0).toMap().value("collectionName").toString());
        collectionNameLabel->setObjectName("collectionNameLabel");

        closeBtn = new QToolButton();
        closeBtn->setObjectName("droppedCollectionCloseBtn");
        connect(closeBtn, SIGNAL(clicked(bool)), this, SLOT(closeCollection()));
        if (closeBtnFrameLayout)
        {
            closeBtnFrameLayout->addWidget(collectionNameLabel, 1, Qt::AlignLeft);
            closeBtnFrameLayout->addWidget(closeBtn);
        }

        //КОСТЫЛЬ: если меню иерархический список, то в выпдающем меню показываем подпись
        collectionNameLabel->setVisible(collectionIsMinimized
                                        || menuType == menuTypeSubsystemsTree);
    }

    //Разбираем параметры
    QVariantMap currentItemInfo(params["currentItemInfo"].toMap());

    //Запоминаем информацию о текущем элементе
    mainCollectionViewFrame->setProperty("itemProperties", currentItemInfo);

    if (items.isEmpty())
    {
        addInfoItem(ru("Информация отсутствует"));
        return;
    }

    //Номер вставляемого виджета
    int insertNum = 1;

    //Зададим размер отступа
    QString indentText;
    if (currentCollectionItem != Q_NULLPTR)
        for (int i = 0; i < (currentCollectionItem->level); ++i)
            indentText = indentText + "      ";

    //Заполняем коллекции элементами,для каждого типа элемента свое заполнение
    for (const QVariant &elem : items)
    {
        QVariantMap elementInfoMap = elem.toMap();

        if (elementType == ru("ЭлементВыпадающегоМеню"))
        {
            //Показатель, является ли элемент групповым
            bool isGroupElement = elementInfoMap.value("isGroupElement", false).toBool();

            //Формируем шаблон интерфейса коллекции и список свойств
            auto objectTemplate = new ObjectBaseTemplate(ru("%1_ЭлементВыпадающегоМеню")
                                                         .arg(getObjectName()),
                                                         baseLabelObjectWidget);
            ObjectPropertyList objectProperties;
            objectProperties << createProperty<ObjectQVariantProperty>(
                                    labelCaptionPropertyName,
                                    elementInfoMap.value("textCentral"));

            //Здесь есть уровни элементов,к разным уровням применяются разные стили,поэтому заполняем имена объектов в зависимости от уровня
            int elementLevel = elementInfoMap.value("level").toInt();
            if (elementLevel > 0
                && elementLevel < 4)
            {
                const QString labelObjectNameTemplate = ((isGroupElement)
                                                         ? QString("dropMenuTextLabelGroupLvl%1")
                                                         : QString("dropMenuTextLabelLvl%1"));
                bool isNotSubsystemObject = (elementInfoMap.value("purpose").toString() != subsystemObject && !elementInfoMap.contains("dependingOnDictGroupId"));
                objectProperties << createProperty<ObjectQVariantProperty>(
                                        baseLabelLabelObjectNamePropertyName,
                                        (elementLevel == 1
                                         && !isNotSubsystemObject)
                                        ? QString("dropSubsystemTextLabel")
                                        : labelObjectNameTemplate.arg(elementLevel));
            }

            //Создаем виджет коллекции
            ObjectWidget *collectionItemObjectWidget = ObjectWidgetCreator::createObjectWidget(objectTemplate,
                                                                                               objectProperties,
                                                                                               getRootWidget(),
                                                                                               contentFrame);
            if (collectionItemObjectWidget)
            {
                //Устанавливаем свойства
                collectionItemObjectWidget->getWidget()->setProperty("itemProperties", elementInfoMap);

                //Все подсистемы в выплывающем меню обозначены серым цветом,
                //и кликать по ним будет нелья,
                //кликать можно по элементам подсистемы,
                //поэтому вешаем ивент фильтр только на них
                if (elementInfoMap["purpose"].toString() != subsystemObject
                    && !elementInfoMap.contains("dependingOnDictGroupId"))
                    collectionItemObjectWidget->getWidget()->installEventFilter(this);

                //Добавляем фрейм элемента в список
                auto collectionViewItemFrame = qobject_cast<QFrame *>(collectionItemObjectWidget->getWidget());
                if (collectionViewItemFrame)
                    menuItems << collectionViewItemFrame;
                collectionItemObjectWidget->refreshWidget();
                collectionItemObjectWidget->showWidget();
                collectionItemObjectWidget->setWidgetProperty("collectionItem", true);

                //Добавляем виджет в компоновщик
                if (mainCollectionViewFrameLayout)
                    mainCollectionViewFrameLayout->addWidget(collectionItemObjectWidget->getWidget());

                //Добавляем objectWidget элемента в список
                menuItemsObjectWidgets << collectionItemObjectWidget;
            }
        }
        else if (menuType == menuTypeSubsystemsTree)
        {
            //Здесь формируем элементы типов левого меню и центральной области
            if (collectionIsMinimized)
                continue;

            //Формируем шаблон интерфейса и список свойств
            ObjectBaseTemplate *objectTemplate = Q_NULLPTR;
            ObjectPropertyList objectProperties;
            if (elementType == ru("ЭлементКоллекцииЛевогоМеню")
                || elementType == ru("ЭлементКоллекцииЦентральнойОбласти"))
            {
                objectTemplate = new ObjectBaseTemplate(ru("ЭлементКоллекцииЛевогоМеню"),
                                                        QString("collectionElementForLeftMenu"));
                objectProperties << createProperty<ObjectQVariantProperty>(
                                        (elementInfoMap.contains(imagePathPropertyName)
                                         ? imagePathPropertyName
                                         : frameImagePropertyName),
                                        elementInfoMap.value(imagePathPropertyName,
                                                             elementInfoMap.value("image")),
                                        false);
            }
            if (!objectTemplate)
                continue;

            //Задаем свойства колонок для дерева
            ObjectQVariantPropertyPointer captionObjectProperty = createProperty<ObjectQVariantProperty>(
                                                                      labelCaptionPropertyName,
                                                                      elementInfoMap.value("text"),
                                                                      false);
            captionObjectProperty->targetNames << ru("ПодписьЭлементаЛевогоМеню");

            //Задаем свойства колонок для дерева
            ObjectQVariantPropertyPointer indentObjectProperty = createProperty<ObjectQVariantProperty>(
                                                                     labelCaptionPropertyName,
                                                                     indentText,
                                                                     false);
            indentObjectProperty->targetNames << ru("ОтступЭлементаЛевогоМеню");

            //Добавим свойства
            objectProperties << captionObjectProperty
                             << indentObjectProperty;

            //Создаем интерфейс
            ObjectWidget *collectionItemObjectWidget = ObjectWidgetCreator::createObjectWidget(objectTemplate,
                                                                                               objectProperties,
                                                                                               getRootWidget(),
                                                                                               contentFrame);
            if (!collectionItemObjectWidget)
                continue;

            //Определяем уровень элемента
            int level = 0;
            if (currentCollectionItem != Q_NULLPTR)
                level = currentCollectionItem->level;

            QWidget *collectionItemWidget = collectionItemObjectWidget->getWidget();

            //Устанавливаем свойства
            collectionItemWidget->setProperty("itemProperties", elementInfoMap);
            collectionItemWidget->setProperty("itemLevel", QString::number(level));
            if (elementType == ru("ЭлементКоллекцииЛевогоМеню")
                || elementType == ru("ЭлементКоллекцииЦентральнойОбласти"))
            {
                collectionItemWidget->setProperty("menuItem", true);
                collectionItemWidget->setProperty("menuSelected", false);
                collectionItemWidget->setToolTip(elementInfoMap["toolTip"].toString());
            }
            collectionItemWidget->installEventFilter(this);

            //Добавляем фрейм элемента в список
            auto collectionViewItemFrame = qobject_cast<QFrame *>(collectionItemWidget);
            collectionItemObjectWidget->refreshWidget();
            collectionItemObjectWidget->showWidget();
            collectionItemObjectWidget->setWidgetProperty("collectionItem", true);

            //Добавляем виджет в компоновщик
            collectionItemWidget = collectionItemObjectWidget->getWidget();
            if (currentCollectionItem == Q_NULLPTR)
            {
                mainCollectionViewFrameLayout->addWidget(collectionItemWidget);
            }
            else if (mainCollectionViewFrameLayout)
            {
                auto vboxlayout = qobject_cast<QVBoxLayout *>(mainCollectionViewFrameLayout);
                if (vboxlayout)
                {
                    int vboxlayoutIndex = vboxlayout->indexOf(currentCollectionItem->itemObjectWidget->getWidget());
                    vboxlayout->insertWidget(vboxlayoutIndex + insertNum,
                                             collectionItemWidget);
                    insertNum++;
                }
                else
                {
                    mainCollectionViewFrameLayout->addWidget(collectionItemWidget);
                }
            }

            //Добавляем новый виджет в структуру
            DBSerial collectionItemWidgetParamValue = collectionItemWidget->property("param").toDBSerial();
            CollectionItem *childCollectionItem = new CollectionItem(collectionItemWidgetParamValue,
                                                                     collectionItemObjectWidget,
                                                                     collectionViewItemFrame,
                                                                     currentCollectionItem,
                                                                     level + 1);
            //Добавляем в структуру
            if (currentCollectionItem != Q_NULLPTR)
            {
                //Добавляем детей
                currentCollectionItem->childs << childCollectionItem;
                //Устанавливаем свойство открытого виджета
                currentCollectionItem->isOpen = true;
            }
            else
            {
                //Добавляем в корень
                leftMenuCollectionItems << childCollectionItem;
            }
        }
        else
        {
            //Здесь формируем элементы типов левого меню и центральной области
            //Формируем шаблон интерфейса и список свойств
            ObjectBaseTemplate *objectTemplate = Q_NULLPTR;
            ObjectPropertyList objectProperties;
            if (elementType == ru("ЭлементКоллекцииЛевогоМеню"))
            {
                objectTemplate = new ObjectBaseTemplate(ru("ЭлементКоллекцииЛевогоМеню"),
                                                        QString("collectionElementForLeftMenu"));
                objectProperties << createProperty<ObjectQVariantProperty>(
                                        (elementInfoMap.contains(imagePathPropertyName)
                                         ? imagePathPropertyName
                                         : frameImagePropertyName),
                                        elementInfoMap.value(imagePathPropertyName,
                                                             elementInfoMap.value("image")),
                                        false);

                //Задаем свойства колонок для дерева
                ObjectQVariantPropertyPointer captionObjectProperty = createProperty<ObjectQVariantProperty>(
                                                                          labelCaptionPropertyName,
                                                                          elementInfoMap.value("text"),
                                                                          false);
                captionObjectProperty->targetNames << ru("ПодписьЭлементаЛевогоМеню");

                //Добавим свойства
                objectProperties << captionObjectProperty;
            }
            else if (elementType == ru("ЭлементКоллекцииЦентральнойОбласти"))
            {
                //Определяем класс шаблона элемента
                QVariantMap currentItemInfoProperties(currentItemInfo["properties"].toMap());
                QString childItemObjectTemplateClass(currentItemInfoProperties.value("childItemObjectTemplateClass",
                                                                                     QString("collectionElementForCentralFrame")).toString());
                //Создаем шаблон
                objectTemplate = new ObjectBaseTemplate(ru("ЭлементКоллекцииЦентральнойОбласти"),
                                                        childItemObjectTemplateClass);
                //Добавляем свойство иконки
                objectProperties << createProperty<ObjectQVariantProperty>(
                                        (elementInfoMap.contains("imageCentralPath")
                                         ? imagePathPropertyName
                                         : frameImagePropertyName),
                                        elementInfoMap.value("imageCentralPath",
                                                             elementInfoMap.value("imageCentral")),
                                        false);

                //Добавляем свойство текста
                ObjectQVariantPropertyPointer labelCaptionProperty = createProperty<ObjectQVariantProperty>(
                                                                         labelCaptionPropertyName,
                                                                         elementInfoMap.value("textCentral"),
                                                                         false);
                labelCaptionProperty->targetNames << ru("ПодписьЭлемента");
                objectProperties << labelCaptionProperty;
            }
            if (!objectTemplate)
                continue;

            //Создаем интерфейс
            ObjectWidget *collectionItemObjectWidget = ObjectWidgetCreator::createObjectWidget(objectTemplate,
                                                                                               objectProperties,
                                                                                               getRootWidget(),
                                                                                               contentFrame);
            if (!collectionItemObjectWidget)
                continue;

            QWidget *collectionItemWidget = collectionItemObjectWidget->getWidget();

            //Устанавливаем свойства
            collectionItemWidget->setProperty("itemProperties", elementInfoMap);
            if (elementType == ru("ЭлементКоллекцииЛевогоМеню"))
            {
                collectionItemWidget->setProperty("menuItem", true);
                collectionItemWidget->setProperty("menuSelected", false);
                collectionItemWidget->setToolTip(elementInfoMap["toolTip"].toString());
            }
            else if (elementType == ru("ЭлементКоллекцииЦентральнойОбласти"))
            {
                collectionItemWidget->setProperty("centralItem", true);
                collectionItemWidget->setToolTip(elementInfoMap["toolTipCentral"].toString());
            }
            collectionItemWidget->installEventFilter(this);

            //Добавляем фрейм элемента в список
            auto collectionViewItemFrame = qobject_cast<QFrame *>(collectionItemWidget);
            if (collectionViewItemFrame)
                menuItems << collectionViewItemFrame;
            collectionItemObjectWidget->refreshWidget();
            collectionItemObjectWidget->showWidget();
            collectionItemObjectWidget->setWidgetProperty("collectionItem", true);

            //Добавляем виджет в компоновщик
            collectionItemWidget = collectionItemObjectWidget->getWidget();
            if (mainCollectionViewFrameLayout)
                mainCollectionViewFrameLayout->addWidget(collectionItemWidget);

            //Добавляем objectWidget элемента в список
            menuItemsObjectWidgets << collectionItemObjectWidget;
        }
    }

    //Добавим пружинку в конец списка
    if (menuType == menuTypeSubsystemsTree
        && !currentCollectionItem
        && !collectionIsMinimized)
    {
        auto vboxlayout = qobject_cast<QVBoxLayout *>(mainCollectionViewFrameLayout);
        if (vboxlayout)
            vboxlayout->addStretch();
    }

    //Обновляем стиль чтобы выровнять элементы
    styleHelper->setStyleSheet(this->getWidget(), styleName);
}

CollectionView::CollectionItem *CollectionView::findCurrentCollectionItem(QFrame *widgetFrame)
{
    CollectionItem *findCollectionItem = Q_NULLPTR;
    for (int i = 0; i < leftMenuCollectionItems.length(); ++i)
    {
        if (leftMenuCollectionItems[i]->itemObjectFrame == widgetFrame)
            return leftMenuCollectionItems[i];

        for (int j = 0; j < leftMenuCollectionItems[i]->childs.length(); ++j)
        {
            findCollectionItem = findChildCurrentCollectionItem(leftMenuCollectionItems[i]->childs[j],
                                                                widgetFrame);
            if (findCollectionItem)
                return findCollectionItem;
        }
    }
    return findCollectionItem;
}

CollectionView::CollectionItem *CollectionView::findChildCurrentCollectionItem(CollectionItem *childCollectionItem,
                                                                               QFrame *widgetFrame)
{
    CollectionItem *findCollectionItem = Q_NULLPTR;
    if (childCollectionItem->itemObjectFrame == widgetFrame)
        return childCollectionItem;

    for (int j = 0; j < childCollectionItem->childs.length(); ++j)
    {
        findCollectionItem = findChildCurrentCollectionItem(childCollectionItem->childs[j],
                                                            widgetFrame);
        if (findCollectionItem)
            return findCollectionItem;
    }
    return findCollectionItem;
}

void CollectionView::clearChildsCollectionItem(CollectionItem *collectionItem)
{
    for (int j = 0; j < collectionItem->childs.length(); ++j)
        clearChildsCollectionItem(collectionItem->childs[j]);
    collectionItem->childs.clear();
    //Удаляем объект
    delete collectionItem->itemObjectWidget;
    //delete itemStruct->itemObjectFrame;
    delete collectionItem;
}

void CollectionView::setSelection(const QVariant &menuProps)
{
    //Если еще ничего не выбрано
    if (currentMenuItem == -1)
    {
        //То выделять будем только в случае выбора подпунтка главного меню
        for (int i = 0; i < menuItems.count(); ++i)
        {
            QVariant param = menuItems[i]->property("itemProperties").toMap()["param"];
            QVariant execMethod = menuItems[i]->property("itemProperties").toMap()["execMethod"];
            QVariant purpose = menuItems[i]->property("itemProperties").toMap()["purpose"];
            if (menuProps.toMap()["execMethod"] == execMethod
                && menuProps.toMap()["param"] == param)
            {
                menuItems[i]->setProperty("menuSelected", true);
                menuItems[i]->setProperty("showIndicator", purpose != procedureObject);
                currentMenuItem = i;
                styleHelper->setStyleSheet(this->getWidget(), styleName);
                return;
            }
        }
    }
    //Если же какой-либо из пунктов меню уже выбран
    else
    {
        //Если выбрали главное меню,то снимаем выделение
        if (menuProps.toMap()["param"] == objectDataHelper->getProjectSetting(ru("ПодсистемаМеню")).toDBSerial())
        {
            for (int i = 0; i < menuItems.count(); ++i)
            {
                menuItems[i]->setProperty("menuSelected", false);
                menuItems[i]->setProperty("showIndicator", false);
            }
            currentMenuItem = -1;
            styleHelper->setStyleSheet(this->getWidget(), styleName);
            return;
        }
        //Если другой пункт,то проверим,явлеятся ли он подпунктом главного меню
        else
        {
            //Позиция подпункта в меню
            int newPosition = -1;
            for (int i = 0; i < menuItems.count(); ++i)
            {
                QVariant param = menuItems[i]->property("itemProperties").toMap()["param"];
                QVariant execMethod = menuItems[i]->property("itemProperties").toMap()["execMethod"];
                if (menuProps.toMap()["execMethod"] == execMethod
                    && menuProps.toMap()["param"] == param)
                {
                    newPosition = i;
                    break;
                }
            }

            //Если не нашли,значит мы находимся внутри какого -либо подпункта и менять выделение не нужно
            if (newPosition == -1)
            {
                return;
            }
            //Если нашли,то меняем в случае,если вновь выбранный элемент не совпадает с уже выделенным
            else
            {
                if (newPosition != currentMenuItem)
                {
                    menuItems[currentMenuItem]->setProperty("menuSelected", false);
                    menuItems[currentMenuItem]->setProperty("showIndicator", false);
                    menuItems[newPosition]->setProperty("menuSelected", true);
                    menuItems[newPosition]->setProperty("showIndicator", menuItems[newPosition]->property("itemProperties").toMap()["purpose"] != procedureObject);
                    currentMenuItem = newPosition;
                    styleHelper->setStyleSheet(this->getWidget(), styleName);
                }
            }
        }
    }

    //В режиме дерева при свернутом меню ищем по другому
    if (menuType == menuTypeSubsystemsTree && collectionIsMinimized)
    {
        QVariantMap elementInfoMap = currentCollectionFrame->property("itemProperties").toMap();
        QVariantMap elementInfoMapProperties = elementInfoMap["properties"].toMap();
        QVariant purpose = currentCollectionFrame->property("itemProperties").toMap()["purpose"];

        if (purpose == subsystemObject || elementInfoMapProperties.contains("dependingOnDictGroupId"))
            currentCollectionFrame->setProperty("menuSelected", true);
        else
            currentCollectionFrame->setProperty("menuSelected", false);

        if (purpose != procedureObject)
            currentCollectionFrame->setProperty("showIndicator", true);
        else
            currentCollectionFrame->setProperty("showIndicator", false);
        styleHelper->setStyleSheet(this->getWidget(), styleName);
    }
}

QWidget *CollectionView::getCurrentMenuItem()
{
    return menuItems.value(currentMenuItem, Q_NULLPTR);
}

int CollectionView::getCurrentItemPos()
{
    return currentMenuItemCoordinate;
}

void CollectionView::setWidgetPosition(int x,
                                       int y)
{
    if (!mainCollectionViewFrame->isVisible())
        mainCollectionViewFrame->setVisible(!mainCollectionViewFrame->isVisible());
    mainCollectionViewFrame->move(x,y);
}

int CollectionView::calculateHeight()
{
    int height = 0;
    foreach (QFrame* frame, menuItems)
        height += frame->sizeHint().height();

    return height;
}

int CollectionView::getItemHeight()
{
    return (menuItems.isEmpty()
            ? 0
            : menuItems.first()->sizeHint().height());
}

QVariant CollectionView::getWidgetSize()
{
    QSize size = mainCollectionViewFrame->size();

    QVariantMap result;
    result["width"] = size.width();
    result["height"] = size.height();
    return result;
}

void CollectionView::setWidgetSize(int width,
                                   int height)
{
    mainCollectionViewFrame->setFixedSize(width,
                                          height);
}

QVariant CollectionView::getScrollAreaSize()
{
    if (!contentFrame)
        return QVariant();

    QSize size = contentFrame->size();

    QVariantMap result;
    result["width"] = size.width();
    result["height"] = size.height();
    return result;
}

void CollectionView::setSCrollAreaSize(int width,
                                       int height)
{
    if (!contentFrame)
        return;

    contentFrame->setFixedSize(width,
                               height);
}

void CollectionView::moveCollection(int x)
{
    //Задаем анимацию плавного перехода между состояниями:
    QPropertyAnimation *animationForCollection = new QPropertyAnimation(mainCollectionViewFrame, "geometry");
    QRect geomResized = mainCollectionViewFrame->geometry();
    geomResized.setX(x);
    //Конечное положение (в которое попадаем)
    animationForCollection->setEndValue(geomResized);
    animationForCollection->start(QPropertyAnimation::DeleteWhenStopped);
}

bool CollectionView::isVisible(bool needWidgetState)
{
    Q_UNUSED(needWidgetState)

    return mainCollectionViewFrame->isVisible();
}

void CollectionView::setVisible(bool visibleFlag)
{
    mainCollectionViewFrame->setVisible(visibleFlag);
}

void CollectionView::closeCollection()
{
    mainCollectionViewFrame->setVisible(false);
    emit collectionClosed();
}

int CollectionView::getCloseButtonFrameHeight()
{
    if (closeBtn)
        return closeBtnFrame->sizeHint().height();
    return 0;
}

void CollectionView::addInfoItem(const QString &itemCaption)
{
    if (!mainCollectionViewFrameLayout)
        return;

    //Формируем шаблон интерфейса коллекции
    auto objectTemplate = new ObjectBaseTemplate(ru("%1_ЭлементИнформации")
                                                 .arg(getObjectName()),
                                                 baseLabelObjectWidget);
    //Формируем список свойств
    ObjectPropertyList objectProperties;
    objectProperties << createProperty<ObjectQVariantProperty>(
                            labelCaptionPropertyName,
                            itemCaption)
                     << createProperty<ObjectQVariantProperty>(
                            baseLabelLabelObjectNamePropertyName,
                            "dropSubsystemTextLabel");
    ObjectWidget *collectionItemObjectWidget = ObjectWidgetCreator::createObjectWidget(objectTemplate,
                                                                                       objectProperties,
                                                                                       getRootWidget(),
                                                                                       contentFrame);
    if (!collectionItemObjectWidget)
        return;

    //Добавляем фрейм элемента в список
    auto collectionViewItemFrame = qobject_cast<QFrame *>(collectionItemObjectWidget->getWidget());
    if (collectionViewItemFrame)
        menuItems.append(collectionViewItemFrame);
    collectionItemObjectWidget->refreshWidget();
    collectionItemObjectWidget->showWidget();
    collectionItemObjectWidget->setWidgetProperty("collectionItem", true);

    //Добавляем виджет в компоновщик
    if (mainCollectionViewFrameLayout)
        mainCollectionViewFrameLayout->addWidget(collectionItemObjectWidget->getWidget());

    //Добавляем objectWidget элемента в список
    menuItemsObjectWidgets.append(collectionItemObjectWidget);
}


QString CollectionViewCreator::getObjectClass() const
{
    return collectionViewWidget;
}

QString CollectionViewCreator::getObjectCaption() const
{
    return ru("ФреймКоллекции");
}

QString CollectionViewCreator::getObjectDescription() const
{
    return ru("ФреймКоллекции");
}

QString CollectionViewCreator::getObjectIconName() const
{
    return "components/components_base_label";
}

ObjectBase *CollectionViewCreator::construct(const ObjectPropertyList &_properties)
{
    return new CollectionView(_properties);
}
