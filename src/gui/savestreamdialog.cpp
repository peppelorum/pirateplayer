#include "savestreamdialog.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QDir>
#include <QtAlgorithms>
#include <QDebug>
#include <QUrl>
#include <QDataWidgetMapper>
#include <QInputDialog>
#include <QDebug>
#include <QProcess>
#include <QMessageBox>
#include <QStringBuilder>

SaveStreamDialog::SaveStreamDialog(StreamTableModel *model, const QHash<QString, QVariant> &settings, const QString &streamTitle, QWidget *parent):
    QDialog(parent)
{
    this->settings = settings;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QFormLayout *formLayout = new QFormLayout();

    QDataWidgetMapper *dataMapper = new QDataWidgetMapper(this);

    editFileName = new FilePathEdit(this);
    QLabel *suffixHint = new QLabel(this);
    checkSubtitles = new QCheckBox("Ladda ner undertexter till:", this);
    checkSubtitles->hide();
    editSubFileName = new FilePathEdit(this);
    editSubFileName->setEnabled(false);
    editSubFileName->hide();
    comboQuality = new QComboBox(this);
    editStreamUrl = new QLineEdit(this);
    editStreamUrl->hide();
    editSubUrl = new QLineEdit(this);
    editSubUrl->hide();
    editSubUrl->setEnabled(false);
    fileNameInfo = new QLabel("Ange ett giltigt filnamn att ladda ner till", this);
    fileNameInfo->setForegroundRole(QPalette::BrightText);

    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setGeometry(QRect(320, 150, 118, 3));
    line->setFrameShadow(QFrame::Sunken);
    QCheckBox *checkEdit = new QCheckBox(QString::fromUtf8("Redigera addresser"), this);
    QPushButton *buttonPlay = new QPushButton("Spela upp", this);
    buttonSubmit = new QPushButton("Ladda ner", this);
    buttonSubmit->setDefault(true);
    QPushButton *buttonCancel = new QPushButton("Avbryt", this);

    connect(checkEdit, SIGNAL(clicked(bool)), editStreamUrl, SLOT(setVisible(bool)));
    connect(checkEdit, SIGNAL(clicked(bool)), editSubUrl, SLOT(setVisible(bool)));
    connect(buttonSubmit, SIGNAL(clicked()), this, SLOT(checkOverWrite()));
    connect(buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(comboQuality, SIGNAL(currentIndexChanged(int)), dataMapper, SLOT(setCurrentIndex(int)));
    connect(comboQuality, SIGNAL(currentIndexChanged(int)), this, SLOT(comboActivated(int)));
    connect(checkSubtitles, SIGNAL(clicked(bool)), editSubFileName, SLOT(setEnabled(bool)));
    connect(checkSubtitles, SIGNAL(clicked(bool)), editSubUrl, SLOT(setEnabled(bool)));
    connect(checkSubtitles, SIGNAL(clicked(bool)), this, SLOT(enableSubmit()));
    connect(editFileName, SIGNAL(pathValid(bool)), this, SLOT(enableSubmit()));
    connect(editFileName, SIGNAL(dirChanged(QString)), editSubFileName, SLOT(setDir(QString)));
    connect(editSubFileName, SIGNAL(pathValid(bool)), this, SLOT(enableSubmit()));
    connect(buttonPlay, SIGNAL(clicked()), this, SLOT(play()));

    QString filenameTemplate = settings.value("filename_template").toString();
    const QStringList templateVars = QStringList() << "title" << "name" << "time" << "season" << "description";
    for (int i=0; i<templateVars.count(); i++)
        filenameTemplate.replace("%" % templateVars.at(i) % "%", "%" % QString::number(i));
    QString fileName = filenameTemplate;
    QVariantMap metaData = model->metaData();
    for (int i=0; i<templateVars.count(); i++)
        fileName = fileName.arg(metaData.value(templateVars.at(i), "").toString());
    QString nullFileName = filenameTemplate;
    for (int i=0; i<templateVars.count(); i++)
        nullFileName = nullFileName.arg("");
    if (fileName != nullFileName) {
        QString suffix = model->data(model->index(0, StreamTableModel::SuffixHintColumn), Qt::UserRole).toString();
        suffix = !suffix.isEmpty() ? '.' % suffix : suffix;
        bool addSlash = !settings["start_dir"].toString().endsWith(QDir::separator());
        editFileName->setFilePath(settings["start_dir"].toString() % (addSlash ? (QString)QDir::separator() : "") % fileName % suffix);
        editSubFileName->setFilePath(settings["start_dir"].toString() % (addSlash ? (QString)QDir::separator() : "") % fileName);
    }
    else {
        editFileName->setFilePath(settings["start_dir"].toString());
        editSubFileName->setFilePath(settings["start_dir"].toString());
    }

    dataMapper->setModel(model);
    dataMapper->setCurrentIndex(comboQuality->currentIndex());
    dataMapper->addMapping(editStreamUrl, StreamTableModel::UrlColumn);
    dataMapper->addMapping(editSubUrl, StreamTableModel::SubtitlesColumn);
    dataMapper->addMapping(suffixHint, StreamTableModel::SuffixHintColumn, "text");

    comboQuality->setModel(model);

    formLayout->addRow("Ladda ner till:", editFileName);
    formLayout->addRow("", suffixHint);
    formLayout->addRow("Kvalitet:", comboQuality);
    formLayout->addRow("", editStreamUrl);
    formLayout->addRow(checkSubtitles, editSubFileName);
    formLayout->addRow("", editSubUrl);
    formLayout->addRow("", fileNameInfo);

    buttonLayout->addWidget(checkEdit);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(buttonPlay);
    buttonLayout->addWidget(buttonSubmit);
    buttonLayout->addWidget(buttonCancel);

    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(line);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    setWindowTitle(QString::fromUtf8("Spara ström - ") + streamTitle);
    setMinimumWidth(500);
}

SaveStreamDialog::~SaveStreamDialog() {
    ;
}

QString SaveStreamDialog::getFileName() {
    return editFileName->filePath();
}

QString SaveStreamDialog::getSubFileName() {
    return editSubFileName->filePath();
}

bool SaveStreamDialog::downloadSubs() {
    return checkSubtitles->isChecked();
}

QUrl SaveStreamDialog::getUrl() {
    return QUrl(editStreamUrl->text());
}

QUrl SaveStreamDialog::getSubUrl() {
    return QUrl(editSubUrl->text());
}

void SaveStreamDialog::comboActivated(int index = 0) {
    if (getSubUrl().toString() != "") {
        checkSubtitles->show();
        editSubFileName->show();
    }
}

void SaveStreamDialog::enableSubmit() {
    bool enable;
    if (checkSubtitles->isChecked())
        enable = editFileName->isValid() && editSubFileName->isValid();
    else
        enable = editFileName->isValid();

    buttonSubmit->setEnabled(enable);
    fileNameInfo->setVisible(!enable);
}

void SaveStreamDialog::play() {
    QProcess *cmd = new QProcess(this);

    bool ok = 0;
    QString cmdTemplate = QInputDialog::getText(this, tr("Ange uppspelningskommando"),
                                            QString::fromUtf8("Uppspelningskommando:\n    %0 ersätts med videoaddress.\n    %1 ersätts med undertextaddress."), QLineEdit::Normal,
                                            settings["player_cmd"].toString(), &ok);

    if (ok && !cmdTemplate.isEmpty())
        cmd->start(cmdTemplate.arg(editStreamUrl->text(), editSubUrl->text()));
}

void SaveStreamDialog::checkOverWrite() {
    QMessageBox messageBox;
    messageBox.setInformativeText(QString::fromUtf8("Vill du ändå spara till den och därmed skriva över den?"));
    messageBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
    messageBox.setDefaultButton(QMessageBox::Cancel);

    if (QFile::exists(getFileName())) {
        messageBox.setText("Filen " + QDir::toNativeSeparators(getFileName()) + " finns sedan tidigare.");
        int ret = messageBox.exec();
        if (ret == QMessageBox::Cancel)
            return;
    }
    if (downloadSubs() && QFile::exists(getSubFileName())) {
        messageBox.setText("Filen " + QDir::toNativeSeparators(getSubFileName()) + " finns sedan tidigare.");
        int ret = messageBox.exec();
        if (ret == QMessageBox::Cancel)
            return;
    }

    accept();
}
