#ifndef MODELMANAGERWIDGETITEM_H
#define MODELMANAGERWIDGETITEM_H

#include <QWidget>

#include "modelmanager.h"

namespace Ui {
class ModelManagerWidgetItem;
}

class ModelManagerWidgetItem : public QWidget
{
    Q_OBJECT

public:
    explicit ModelManagerWidgetItem(ModelManagerItem *item, ModelManager *manager, QWidget *parent = nullptr);
    ~ModelManagerWidgetItem();

private:
    Ui::ModelManagerWidgetItem *ui;
    ModelManagerItem *mItem;
    ModelManager *mModelManager = nullptr;
};

#endif // MODELMANAGERWIDGETITEM_H
