#include "modelmanagerwidgetitem.h"
#include "ui_modelmanagerwidgetitem.h"

#include <QMessageBox>
#include <QtMath>

ModelManagerWidgetItem::ModelManagerWidgetItem(ModelManagerItem *item, ModelManager *manager, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ModelManagerWidgetItem)
    , mItem(item)
    , mModelManager(manager)
{
    ui->setupUi(this);
    ui->stopBtn->setVisible(mItem->downloading());
    ui->deleteBtn->setVisible(!mItem->downloading());
    ui->progressBar->setVisible(mItem->downloading());
    ui->status->setVisible(mItem->downloading());
    ui->status->setText(tr("Checking..."));
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(0);

    ui->title->setText(item->name());

    connect(mItem, &ModelManagerItem::downloadingChanged, this, [this](){
        ui->stopBtn->setVisible(mItem->downloading());
        ui->deleteBtn->setVisible(!mItem->downloading());
        ui->progressBar->setVisible(mItem->downloading());
        ui->progressBar->setValue(0);
        ui->progressBar->setMaximum(0);
    });
    connect(mItem, &ModelManagerItem::downloadedChanged, this, [this](){
        ui->progressBar->setMaximum(mItem->total());
        ui->progressBar->setValue(mItem->downloaded());
        ui->status->setText(
                tr("%1MB -> %2MB\n%3%")
                    .arg(std::floor(mItem->downloaded()/1000000))
                    .arg(std::floor(mItem->total()/1000000))
                    .arg(std::floor(1000*mItem->downloaded()/mItem->total())/10.0)
            );
    });
    connect(ui->stopBtn, &QPushButton::clicked, this, [this](){
        if (QMessageBox::warning(this, tr("Cancel"), tr("Are you sure about cancel download process?"), QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes)
            return;

        mItem->stop();
    });
    connect(ui->deleteBtn, &QPushButton::clicked, this, [this](){
        if (QMessageBox::warning(this, tr("Delete"), tr("Are you sure about delete this model?"), QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes)
            return;

        mModelManager->remove(mItem->name());
    });
}

ModelManagerWidgetItem::~ModelManagerWidgetItem()
{
    delete ui;
}
