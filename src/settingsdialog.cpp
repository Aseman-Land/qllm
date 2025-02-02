#include "modelmanagerwidgetitem.h"
#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QProgressBar>
#include <QInputDialog>

SettingsDialog::SettingsDialog(QSettings *settings, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
    , mSettings(settings)
{
    ui->setupUi(this);
    ui->listWidget->setCurrentRow(0);

    ui->ollamaHost->setText( mSettings->value("Ollama/host", "localhost").toString() );
    ui->ollamaPort->setValue( mSettings->value("Ollama/port", 11434).toInt() );

    mModelManager = new ModelManager(this);

    connect(mModelManager, &ModelManager::itemsChanged, this, &SettingsDialog::itemsChanged);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

ModelManager *SettingsDialog::modelManager() const
{
    return mModelManager;
}

void SettingsDialog::on_listWidget_currentRowChanged(int currentRow)
{
    if (currentRow < 0)
        return;

    ui->stackedWidget->setCurrentIndex(currentRow);
    ui->tabTitle->setText(ui->listWidget->item(currentRow)->text());
}

void SettingsDialog::itemsChanged()
{
    QSet<QString> items;
    for (const auto &item: mModelManager->items())
    {
        items.insert(item->name());
        if (mItems.contains(item->name()))
            continue;

        auto i = new QListWidgetItem();
        i->setSizeHint(QSize(100, 70));
        i->setData(Qt::UserRole, QVariant::fromValue<QObject*>(item));

        auto wgt = new ModelManagerWidgetItem(item, mModelManager);

        ui->modelsList->addItem(i);
        ui->modelsList->setItemWidget(i, wgt);
        mItems[item->name()] = i;
    }

    for (const auto &name: mItems.keys())
    {
        if (items.contains(name))
            continue;

        auto i = mItems.take(name);
        // auto item = qobject_cast<ModelManagerItem*>(i->data(Qt::UserRole).value<QObject*>());
        delete i;
    }
}

QString SettingsDialog::baseUrl() const
{
    return mBaseUrl;
}

void SettingsDialog::setBaseUrl(const QString &newBaseUrl)
{
    mBaseUrl = newBaseUrl;
    mModelManager->setBaseUrl(mBaseUrl);
}

void SettingsDialog::setCurrentTab(int index)
{
    ui->listWidget->setCurrentRow(index);
}

void SettingsDialog::accept()
{
    mSettings->setValue("Ollama/host", ui->ollamaHost->text());
    mSettings->setValue("Ollama/port", ui->ollamaPort->value());

    QDialog::accept();
}

void SettingsDialog::on_pullModelBtn_clicked()
{
    const auto modelName = QInputDialog::getText(this, tr("Pull model"), tr("Please enter model name below:")).trimmed();
    if (modelName.isEmpty())
        return;

    mModelManager->pull(modelName);
}

