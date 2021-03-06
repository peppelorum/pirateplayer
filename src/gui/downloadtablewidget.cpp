#include "downloadtablewidget.h"
#include "../network/abstractdownload.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QDir>

DownloadTableWidget::DownloadTableWidget(QWidget *parent) :
    QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    table = new QTableView(this);
    table->setGridStyle(Qt::NoPen);
    table->setItemDelegate(new DownloadDelegate(this));
    //table->setColumnWidth(2, 200);
    table->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    removeButton = new QPushButton("Ta bort", this);
    removeFromDiskButton = new QPushButton(QString::fromUtf8("Ta bort från disk"), this);
    abortButton = new QPushButton("Avbryt", this);
    openButton = new QPushButton(QString::fromUtf8("Öppna"), this);
    enableButtons(false);
    abortButton->setEnabled(false);

    connect(openButton, SIGNAL(clicked()), this, SLOT(openCurrentFile()));
    connect(abortButton, SIGNAL(clicked()), this, SLOT(abortCurrent()));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(removeCurrent()));
    connect(removeFromDiskButton, SIGNAL(clicked()), this, SLOT(removeCurrentFromDisk()));

    layout->addWidget(table, 1);
    layout->addLayout(buttonLayout);
    buttonLayout->addWidget(abortButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addWidget(removeFromDiskButton);
    buttonLayout->addWidget(openButton);
    buttonLayout->addStretch();

    selectionModel = 0;
}

void DownloadTableWidget::setModel(DownloadListModel *m) {
    model = m;
    table->setModel(model);
    if (selectionModel != 0)
        delete selectionModel;
    selectionModel = new QItemSelectionModel(model, this);
    table->setSelectionModel(selectionModel);

    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)));
    connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
}

int DownloadTableWidget::currentRow() {
    return selectionModel->currentIndex().row();
}

QModelIndex DownloadTableWidget::currentStatusIndex() {
    return model->index(currentRow(), DownloadListModel::StatusColumn);
}

bool DownloadTableWidget::currentIsDownloading() {
    return model->data(currentStatusIndex(), Qt::UserRole).toInt() == AbstractDownload::Downloading;
}

void DownloadTableWidget::enableButtons(bool enabled) {
    removeButton->setEnabled(enabled);
    removeFromDiskButton->setEnabled(enabled);
    openButton->setEnabled(enabled);
}

void DownloadTableWidget::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
    QItemSelection range = QItemSelection(topLeft, bottomRight);
    if (range.contains(currentStatusIndex()))
        abortButton->setEnabled(currentIsDownloading());
}

void DownloadTableWidget::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    bool anythingSelected = selected.count() > 0;

    enableButtons(anythingSelected);
    abortButton->setEnabled(anythingSelected && currentIsDownloading());
}

void DownloadTableWidget::openCurrentFile() {
    QString fileName = model->data(model->index(currentRow(), DownloadListModel::FileNameColumn), Qt::DisplayRole).toString();
    qDebug() << QUrl("file:///" + QDir::fromNativeSeparators(fileName), QUrl::TolerantMode);
    QDesktopServices::openUrl(QUrl("file:///" + QDir::fromNativeSeparators(fileName), QUrl::TolerantMode));
}

void DownloadTableWidget::abortCurrent() {
    model->abortDownload(currentRow());
}

void DownloadTableWidget::removeCurrent() {
    if (currentIsDownloading())
        abortCurrent();
    model->removeRow(currentRow());
}

void DownloadTableWidget::removeCurrentFromDisk() {
    QString path = model->data(model->index(currentRow(), DownloadListModel::FileNameColumn), Qt::DisplayRole).toString();
    removeCurrent();
    QFile::remove(path);
}
