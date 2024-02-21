#include "ui_dashboardsettingsdialog.h"
#include "dashboardsettingsdialog.h"

#include <systemutils.h>

DashBoardSettingsDialog::DashBoardSettingsDialog(QWidget *parent) :
    QDialog(parent, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
    ui(new Ui::DashBoardSettingsDialog)
{
    ui->setupUi(this);
}

DashBoardSettingsDialog::~DashBoardSettingsDialog()
{
    delete ui;
}

void DashBoardSettingsDialog::addDashBoard(const QString &dashName,
                                           const QString &dashCaption,
                                           bool visible)
{
    QTreeWidgetItem *dashBoardItem = new QTreeWidgetItem(ui->dashBoardsTreeWidget);
    dashBoardItem->setText(0, dashCaption);
    dashBoardItem->setData(0, Qt::UserRole, dashName);
    dashBoardItem->setCheckState(1, visible?Qt::Checked:Qt::Unchecked);
    ui->dashBoardsTreeWidget->resizeColumnToContents(0);
}

bool DashBoardSettingsDialog::isDashBoardVisible(const QString &dashName)
{
    for (int i=0;i<ui->dashBoardsTreeWidget->topLevelItemCount();i++)
    {
        QTreeWidgetItem *dashBoardItem = ui->dashBoardsTreeWidget->topLevelItem(i);
        if (dashBoardItem->data(0, Qt::UserRole).toString() == dashName)
            return (dashBoardItem->checkState(1) == Qt::Checked);
    }
    return false;
}

void DashBoardSettingsDialog::on_selectAllButton_clicked()
{
    bool selected = !ui->selectAllButton->property("selected").toBool();
    ui->selectAllButton->setProperty("selected", selected);
    if (selected)
    {
        ui->selectAllButton->setText(ru("Снять выбор всего"));
    }
    else
    {
        ui->selectAllButton->setText(ru("Выбрать всё"));
    }
    for (int i=0;i<ui->dashBoardsTreeWidget->topLevelItemCount();i++)
        ui->dashBoardsTreeWidget->topLevelItem(i)->setCheckState(1, selected?Qt::Checked:Qt::Unchecked);
}
