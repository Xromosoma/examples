#ifndef DashBoardWindow_H
#define DashBoardWindow_H

#include <fields.h>
#include <fieldshelper.h>
#include <objecthelper.h>

#include <QMainWindow>
#include <QFrame>
#include <QWebEngineView>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QComboBox>
#include <QToolButton>
#include <QLabel>

namespace Ui {
    class DashBoardWindow;
}

const QString viewSettingsTableTypeTable = QString::fromUtf8("Табличный"); //!< тип таблицы - Табличный
const QString viewSettingsTableTypeChart = QString::fromUtf8("Графический"); //!< тип таблицы - Графический
const QString viewSettingsTableTypeComplex = QString::fromUtf8("Совмещенный"); //!< тип таблицы - Совмещенный

const int chartDefaultHeight = 350;

//! \brief Окно отображения дашбордов
/*! Реализует отображение окна дашбордов.
 */
class DashBoardWindow : public QMainWindow
{
    Q_OBJECT
private:
    //! Структура фрейма дашборда
    struct DashFrame
    {
        QString dashName; //!< имя дашборда
        QString dashCaption; //!< подпись дашборда
        QFrame *mainFrame; //!< фрейм заголовка дашборда
        QHBoxLayout *mainFrameLayout; //!< контейнер для размещения компонент
        QLabel *mainFrameLabel; //!< подпись дашборда
        QFrame *childFrame; //!< основной фрейм компонент дашборда
        QVBoxLayout *childFrameLayout; //!< контейнер для размещения компонент
        QFrame *buttonsFrame; //!< фрейм для кнопок
        QHBoxLayout *buttonsFrameLayout; //!< контейнер для размещения компонент
        QToolButton *setupFilterButton; //!< кнопка настройки фильтров
        QToolButton *setupAdditionalFilterButton; //!< кнопка настройки дополнительных фильтров
        QComboBox *viewTypeBox; //!< поле выбора типа отображения дашборда
        QToolButton *saveButtonHTML; //!< кнопка сохранения в HTML
        QToolButton *saveButtonExcel; //!< кнопка сохранения в Excel
        QToolButton *saveButtonCSV; //!< кнопка сохранения в CSV
        QFrame *filtersFrame; //!< фрейм для отображения фильтров
        QHBoxLayout *filtersFrameLayout; //!< контейнер для размещения компонент
        QLabel *filtersLabel; //!< метка для отображения фильтров
        QWebEngineView *chartView; //!< область отображения графика
        QString chartScript; //!< скрипт данных графика
        QTableWidget *tableWidget; //!< таблица данных
        ObjectHelper::ObjectInfo objectInfo; //!< информация об объекте данных
        FieldsHelper *fieldsHelper; //!< помощник для отображения и фильтрации полей
        ObjectHelper::ObjectInfo additionalObjectInfo; //!< информация об дополнительном объекте данных
        FieldsHelper *additionalFieldsHelper; //!< помощник для отображения и фильтрации дополнительных полей
    };

private:
    Ui::DashBoardWindow *ui; //!< интерфейс окна
    bool isChildVisible; //!< признак видимости дочерних фреймов
    QMap<QString,DashFrame*> dashFrames; //!< список фреймов дашбордов
    QString currentDashName; //!< имя активного дашборда,в котором работает пользователь

public:
    ObjectHelper::ObjectInfo dashBoardWindowObjectInfo; //!< информация об объекте процедуры дашбордов

public:
    //! Конструктор
    /*! \param parent - указатель на родительское окно.
     *  Создает объект класса \e DashBoardWindow.
     */
    DashBoardWindow(QWidget *parent = 0);
    //! Деструктор
    /*! Уничтожает объект класса \e DashBoardWindow.
     */
    ~DashBoardWindow();

private:
    //! Обработка события нажатия кнопки
    /*! \param event - событие.
     *  Вызывается при нажатии кнопки клавиатуры.
     */
    void keyPressEvent(QKeyEvent *event);

public slots:
    //! Обновление окна
    void refresh();
    //! Очистка окна
    void clear();
    //! Добавление баяна дашборда
    /*! \param dashName - наименование дашборда,
     *  \param dashCaption - подпись.
     *  \return - указатель на добавленный фрейм.
     */
    DashFrame* addDashFrame(const QString &dashName,
                            const QString &dashCaption);
    //! Обновление данных дашборда
    /*! \param dashName - наименование дашборда.
     */
    void refreshDashFrame(const QString &dashName);
    //!
    //! \brief refreshCurrentDash - обновление текущего дашборда, в котором изменяем фильтр
    //!
    void refreshCurrentDash();
    //! Обновление дашборда
    /*! \param dashName - наименование дашборда.
     */
    void updateDashFrame(const QString &dashName);
    //! Обновление графика дашборда
    /*! \param dashName - наименование дашборда.
     */
    void updateChartView(const QString &dashName);
    //! Получение шаблона скрипта графика дашборда
    /*! \param dashName - наименование дашборда.
     *  \return - шаблон скрипта графика.
     */
    QString getChartScriptTemplate(const QString &dashName);
    //! Метод отображения справки
    void showHelp();

public:
    //! Установка фильтра дашборда
    /*! \param dashName - наименование дашборда.
     */
    void setupFilter(const QString &dashName);
    //! Установка дополнительного фильтра дашборда
    /*! \param dashName - наименование дашборда.
     */
    void setupAdditionalFilter(const QString &dashName);
    //! Сохранение данных дашборда в HTML
    /*! \param dashName - наименование дашборда.
     */
    void saveToHTML(const QString &dashName);
    //! Сохранение данных дашборда в Excel
    /*! \param dashName - наименование дашборда.
     */
    void saveToExcel(const QString &dashName);
    //! Сохранение данных дашборда в CSV
    /*! \param dashName - наименование дашборда.
     */
    void saveToCSV(const QString &dashName);

private slots:
    //! Обработка нажатия кнопки выбора фильтра
    void dashSetupFilterButtonClicked();
    //! Обработка нажатия кнопки выбора дополнительного фильтра
    void dashSetupAdditionalFilterButtonClicked();
    //! Обработка сохранения в HTML содержимого дашборда
    void dashFrameSaveButtonHTMLClicked();
    //! Обработка сохранения в Excel содержимого дашборда
    void dashFrameSaveButtonExcelClicked();
    //! Обработка сохранения в CSV содержимого дашборда
    void dashFrameSaveButtonCSVClicked();
    //! Обработка выбора элемента в QComboBox выбора типа отображения дашборда
    /*! \param index - индекс элемента.
     *  Вызывается при выборе элемента.
     */
    void viewTypeBoxCurrentIndexChanged(int index);
    //! Обработка окончания загрузки графика
    /*! \param ok - результат загрузки.
     */
    void chartViewLoadFinished(bool ok);
    //! Обработка нажатия кнопки сворачивания/разворачивания всех вкладок
    void on_showAllButton_clicked();
    //! Обработка нажатия кнопки отображения настроек
    void on_settingsToolButton_clicked();
    //! Обработка нажатия кнопки отображения справки
    void on_helpButton_clicked();
};

#endif // DashBoardWindow_H
