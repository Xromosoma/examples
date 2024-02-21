#ifndef COLLECTIONVIEW_H
#define COLLECTIONVIEW_H

#include "objectwidgetcreator.h"

#include <QObject>
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QToolButton>
#include <QLabel>

//! \brief Виджет центральной зоны главного окна программы,отображающий элементы подсистем
/*! Реализует возможности элементов подсистем.
 */
class MetaBaseGuiUtilsExport CollectionView : public ObjectWidget
{
    Q_OBJECT
protected:
    struct CollectionItem //! элемент коллекции
    {
    public:
        DBSerial id; //!< id виджета элемента коллекции
        ObjectWidget *itemObjectWidget; //!< виджет элемента коллекции
        QFrame *itemObjectFrame; //!< фрейм элемента коллекции
        CollectionItem *parent; //!< родитель
        int level; //!< уровень вложенности
        bool isOpen; //!< свойство раскрытости подсистемы
        QList<CollectionItem *> childs; //!< дети

    public:
        //! \brief конструктор
        CollectionItem(const DBSerial &_id = -1,
                       ObjectWidget *_itemObjectWidget = Q_NULLPTR,
                       QFrame *_itemObjectFrame = Q_NULLPTR,
                       CollectionItem *_parent = Q_NULLPTR,
                       int _level = 0,
                       bool _isOpen = false)
        {
            id  = _id;
            itemObjectWidget = _itemObjectWidget;
            itemObjectFrame = _itemObjectFrame;
            parent = _parent;
            level = _level;
            isOpen = _isOpen;
            childs.clear();
        }
    };

protected:
    QString styleName; //!< имя файла стилей, используемого в коллекции
    QString menuType; //!< тип меню, используемого в коллекции
    QScrollArea *mainArea; //!< область прокрутки для элементов выпадающего списка
    QFrame *mainCollectionViewFrame; //!< главный фрейм
    QLayout *mainCollectionViewFrameLayout; //!< компоновщик виджета
    QHBoxLayout *closeBtnFrameLayout; //!< компоновщик для кнопки закрытия
    QFrame *closeBtnFrame; //!< фрейм кнопки закрытия дял выпадающей коллекции
    QLabel *collectionNameLabel; //!< подпись для заголовка коллекции
    QToolButton *closeBtn; //!< кнопка закрытия выпадающего списка
    DBSerial menuSubsystemId; //!< идентификатор подсистемы для главного меню
    QString menuSubsystemText; //!< заголовок подсистемы для главного меню
    QList<QFrame *> menuItems; //!< список фреймов элементов меню
    QList<ObjectWidgetPointer> menuItemsObjectWidgets; //!< список фреймов элементов меню
    QString elementType; //!< тип элемента коллекции (имя шаблона)
    QString layoutType; //!< тип используемого компоновщика
    bool collectionIsMinimized; //!< признак отображения коллекции в полном или сокращенном виде
    bool collectionIsFavorites; //!< признак отображения коллекции в режиме показа избранного
    bool itemsCaptionsVisible; //!< признак видимости подписей элементов
    QMetaObject::Connection favoritesNotificationConnect; //!< подключение подписки на изменение справочника "Избранное"
    int currentMenuItem; //!< текущий выбранный элемент главного меню
    int currentMenuItemCoordinate; //!< координата вновь выбранного элемента главного меню
    QList<CollectionItem *> leftMenuCollectionItems; //!< список фреймов элементов меню
    CollectionItem *currentCollectionItem; //!< текущий выбранный элемент левого меню
    QFrame *currentCollectionFrame;
    QFrame *contentFrame; //!< указатель на фрейм
    QFrame *commonFrame; //!< фрейм виджета с областью прокрутки
    QVBoxLayout *commonFrameLayout; //!< компоновщик фрейма-виджета с областью прокрутки
    QHBoxLayout *mainLayout; //!< указатель на компоновщик
    QFrame *styledLineFrame; //!< указатель на фрейм

public:
    //! Конструктор
    /*! \param _properties - массив свойств.
     * Создает объект класса \e CollectionView.
    */
    CollectionView(const ObjectPropertyList &_properties);
    //! Деструктор
    /*! Уничтожает объект класса \e CollectionView.
    */
    ~CollectionView() Q_DECL_OVERRIDE;

private:
    //! Обработчик событий
    /*! \param obj - объект,
     *  \param event - событие,
     *  \return - результат обработки.
     *  Обрабатывает события объекта.
     */
    bool eventFilter(QObject *obj,
                     QEvent *event) Q_DECL_OVERRIDE;

public slots:
    /*!
     * \brief getObjectClassAlias - возвращает русский псевдоним класса
     * \return псевдоним класса
     */
    QString getObjectClassAlias() const Q_DECL_OVERRIDE;
    //! Выдача списка свойств виджета
    /*! \return - список свойств виджета.
    */
    ObjectPropertyList getProperties() const Q_DECL_OVERRIDE;
    //! Выдача указателя на виджет
    /*! \return - указатель на виджет.
    */
    QWidget *getWidget() const Q_DECL_OVERRIDE;
    //! Метод обновления виджета
    /*! \param params - параметры обновления.
     */
    void refreshWidget(const QVariantMap &params = QVariantMap()) Q_DECL_OVERRIDE;

public slots:
    //! Выдача свойства виджета
    /*! \param name - имя свойства,
     *  \param param - параметр свойства.
     *  \return - значение свойства.
    */
    QVariant getValue(const QString &name,
                      const QString &param = QString()) const Q_DECL_OVERRIDE;

public slots:
    //! Обработка клика по кнопке "Скрыть" главного окна
    void changeCollectionIsMinimizedFlag();
    //! Обработка клика по кнопке "Избранное" главного окна
    void changeCollectionIsFavoritesFlag();
    //! Обработчик события изменения объектов БД
    /*! \param name - имя события
     *  \param source - источик события
     *  \param payload - нагрузка
     */
    void notificationHandlerForFavorites(const QString &name,
                                         QSqlDriver::NotificationSource source,
                                         const QVariantMap &payload);
    //! Отображает/скрывает названия элементов-подсистем
    void showItemsCaptions();
    //! Возвращает видимость подписей элементов
    /*! \return - признак видимости.
     */
    bool getItemsCaptionsVisible();
    //! Устанавливает видимость подписей элементов
    /*! \param visible - признак видимости.
     */
    void setItemsCaptionsVisible(bool visible);
    //! \brief геттер флага режима отображения коллекции
    /*! \return true - коллекция в свернутом положении,false - в развернутом
     */
    bool getCollectionMinimizedFlag();
    //! \brief процедура заполнения коллекции элементами
    /*! \param items - элементы коллекции,
     *  \param params - параметры операции.
     */
    void fillCollection(const QVariantList &items,
                        const QVariantMap &params = QVariantMap());
    //! \brief поиск виджета в структуре
    /*! \param widgetFrame - фрейм виджета
     */
    CollectionItem *findCurrentCollectionItem(QFrame *widgetFrame);
    //! \brief поиск виджета в структуре от конкретного элемента
    /*! \param childCollectionItem - элемент структуры
     *  \param widgetFrame - фрейм виджета.
     */
    CollectionItem *findChildCurrentCollectionItem(CollectionItem *childCollectionItem,
                                                   QFrame *widgetFrame);
    //! \brief поиск виджета в структуре
    /*! \param childCollectionItem - элемент структуры
     */
    void clearChildsCollectionItem(CollectionItem *collectionItem);
    //! \brief установка выделения пункта коллекции
    /*! \param menuProps - свойства выделяемого пункта
     */
    void setSelection(const QVariant &menuProps);
    //! Возвращает виджет текущего элемента
    /*! \return - виджет текущего элемента.
     */
    QWidget *getCurrentMenuItem();
    //! \brief получение новой позиции элемента
    int getCurrentItemPos();
    //! \brief установка позиции виджета
    /*! \param x - координаты x,
     *  \param y - координаты y.
     */
    void setWidgetPosition(int x,
                           int y);
    //! \brief расчёт высоты.
    int calculateHeight();
    //!
    //! \brief getItemHeight - выдача высоты одного элемента
    //! \return  - высота
    //!
    int getItemHeight();
    //! Возвращает размер виджета
    /*! \return - размер виджета.
     */
    QVariant getWidgetSize();
    //! Устанавливает размеры виджета
    /*! \param width - ширина,
     *  \param height - высота.
     */
    void setWidgetSize(int width,
                       int height);
    //! Возвращает размер области прокрутки
    /*! \return - размер области прокрутки.
     */
    QVariant getScrollAreaSize();
    //! Устанавливает размеры области прокрутки
    /*! \param width - ширина,
     *  \param height - высота.
     */
    void setSCrollAreaSize(int width,
                           int height);
    //!
    //! \brief moveCollection - процедура перемещения коллекции в точку
    //! \param x - абсцисса
    //! \param y - ордината
    //!
    void moveCollection(int x);
    //! Выдача видимости виджета
    /*! \param needWidgetState - признак необходимости состояния виджета,
     *  \return - признак, виден ли виджет или нет.
     */
    bool isVisible(bool needWidgetState = false) Q_DECL_OVERRIDE;
    //!
    //! \brief setVisible - установка флага отображения колекции
    //! \param visibleFlag - показать/скрыть
    //!
    void setVisible(bool visibleFlag) Q_DECL_OVERRIDE;
    //!
    //! \brief closeCollection - обрабочтки кнопки закрытия выпадающей коллекции
    //!
    void closeCollection();
    //!
    //! \brief getCloseButtonFrameHeight - выдача размера фрейма кнопки закрытия
    //! \return высота фрейма
    //!
    int getCloseButtonFrameHeight();
    //!
    //! \brief addInfoItem - добавление элемента простой подписи в компоновщик
    //! \param itemCaption - текст подписи
    //!
    void addInfoItem(const QString &itemCaption);

signals:
    //!
    //! \brief allowFooterChangeForSystem - сигнал,разрешающий замену футера в выбранной подсистеме
    //! \param menuProperty - свойства выбранной подсистемы
    //!
    void allowFooterChangeForSystem(const QVariant &menuProperty);
    //! \brief menuSelected - сигнал выбора пункта меню коллекции
    //! \param menuProperty - свойства выбранного меню
    void menuSelected(const QVariant &menuProperty);
    //! \brief menuSelectedFavorites - сигнал выбора коллекции меню избранного
    //! \param menuProperty - свойства выбранного меню
    void menuSelectedFavorites(const QVariant &menuProperty);
    //! \brief menuSelected - сигнал выбора коллекции меню по умолчанию
    void menuSelectedDefault();
    //!
    //! \brief collectionClosed - сигнал закрытия коллекции
    //!
    void collectionClosed();
    //!
    //! \brief showCollection - сигнал отображения коллекции
    //!
    void showCollection();
};


//! \brief Загрузчик виджета объекта
/*! Создает виджет объекта.
*/
class MetaBaseGuiUtilsExport CollectionViewCreator : public ObjectWidgetCreator
{
    Q_OBJECT
public:
    //! Выдача класса виджета объекта
    /*! \return - класс виджета объекта.
    */
    QString getObjectClass() const Q_DECL_OVERRIDE;
    //! Выдача подписи объекта
    /*! \return - подпись объекта.
    */
    QString getObjectCaption() const Q_DECL_OVERRIDE;
    //! Выдача описания объекта
    /*! \return - описание объекта.
    */
    QString getObjectDescription() const Q_DECL_OVERRIDE;
    //! Выдача пути к иконке объекта
    /*! \return - путь к иконке объекта.
    */
    QString getObjectIconName() const Q_DECL_OVERRIDE;

public:
    //! Создание виджета объекта
    /*! \param _properties - массив свойств.
     *  Создает виджет объекта.
    */
    ObjectBase *construct(const ObjectPropertyList &_properties) Q_DECL_OVERRIDE;
};

#endif // COLLECTIONVIEW_H
