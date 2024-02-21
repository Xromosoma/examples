#ifndef DashBoardSettingsDialog_H
#define DashBoardSettingsDialog_H

#include <QDialog>
#include <QTreeWidget>

namespace Ui {
class DashBoardSettingsDialog;
}

//! \brief Диалог настроек отображения дашбордов
/*! Реализует возможности выбора фильтров и параметров отображения дашбордов.
 */
class DashBoardSettingsDialog : public QDialog
{
    Q_OBJECT
private:
    Ui::DashBoardSettingsDialog *ui; //!< интерфейс окна

public:
    //! Конструктор
    /*! \param parent - указатель на родительское окно.
     *  Создает объект класса \e DashBoardSettingsDialog.
     */
    DashBoardSettingsDialog(QWidget *parent = 0);
    //! Деструктор
    /*! Уничтожает объект класса \e DashBoardSettingsDialog.
     */
    ~DashBoardSettingsDialog();

public:
    //! Добавление дашборда
    /*! \param dashName - наименование дашборда,
     *  \param dashCaption - подпись,
     *  \param visible - видимость дашборда.
     */
    void addDashBoard(const QString &dashName,
                      const QString &dashCaption,
                      bool visible);
    //! Выдача видимости дашборда
    /*! \param dashName - наименование дашборда.
     *  \return - видимость дашборда.
     */
    bool isDashBoardVisible(const QString &dashName);

private slots:
    //! Обработка нажатия кнопки установки/сброса выбора всех дашбордов
    void on_selectAllButton_clicked();
};

#endif // DashBoardSettingsDialog_H
