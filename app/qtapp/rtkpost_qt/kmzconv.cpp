//---------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>

#include "postmain.h"
#include "kmzconv.h"
#include "rtklib.h"
#include "viewer.h"

#include <QShowEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#include <QtGlobal>
#include <QFileSystemModel>
#include <QCompleter>

extern MainForm *mainForm;

//---------------------------------------------------------------------------
ConvDialog::ConvDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    viewer = new TextViewer(this);

    QCompleter *fileCompleter = new QCompleter(this);
    QFileSystemModel *fileModel = new QFileSystemModel(fileCompleter);
    fileModel->setRootPath("");
    fileCompleter->setModel(fileModel);
    InputFile->setCompleter(fileCompleter);
    OutputFile->setCompleter(fileCompleter);
    GoogleEarthFile->setCompleter(fileCompleter);

    connect(BtnClose, SIGNAL(clicked(bool)), this, SLOT(BtnCloseClick()));
    connect(BtnConvert, SIGNAL(clicked(bool)), this, SLOT(BtnConvertClick()));
    connect(BtnGoogleEarthFile, SIGNAL(clicked(bool)), this, SLOT(BtnGoogleEarthFileClick()));
    connect(BtnView, SIGNAL(clicked(bool)), this, SLOT(BtnViewClick()));
    connect(AddOffset, SIGNAL(clicked(bool)), this, SLOT(AddOffsetClick()));
    connect(TimeSpan, SIGNAL(clicked(bool)), this, SLOT(TimeSpanClick()));
    connect(TimeIntF, SIGNAL(clicked(bool)), this, SLOT(TimeIntFClick()));
    connect(InputFile, SIGNAL(editingFinished()), this, SLOT(InputFileChange()));
    connect(Compress, SIGNAL(clicked(bool)), this, SLOT(CompressClick()));
    connect(GoogleEarthFile, SIGNAL(editingFinished()), this, SLOT(GoogleEarthFileChange()));
    connect(BtnInputFile, SIGNAL(clicked(bool)), this, SLOT(BtnInputFileClick()));
    connect(FormatKML, SIGNAL(clicked(bool)), this, SLOT(FormatKMLClick()));
    connect(FormatGPX, SIGNAL(clicked(bool)), this, SLOT(FormatGPXClick()));

	UpdateEnable();
}
//---------------------------------------------------------------------------
void ConvDialog::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) return;

    FormatGPX->setChecked(!FormatKML->isChecked());
    GoogleEarthFile->setText(mainForm->GoogleEarthFile);
}
//---------------------------------------------------------------------------
void ConvDialog::SetInput(const QString &File)
{
    InputFile->setText(File);
	UpdateOutFile();
}
//---------------------------------------------------------------------------
void ConvDialog::TimeSpanClick()
{
    UpdateEnable();
}
//---------------------------------------------------------------------------
void ConvDialog::AddOffsetClick()
{
    UpdateEnable();
}
//---------------------------------------------------------------------------
void ConvDialog::TimeIntFClick()
{
    UpdateEnable();
}
//---------------------------------------------------------------------------
void ConvDialog::BtnInputFileClick()
{
    InputFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open..."), QString(), tr("All (*.*)"))));
}
//---------------------------------------------------------------------------
void ConvDialog::BtnConvertClick()
{
    QString InputFile_Text = InputFile->text(), OutputFile_Text = OutputFile->text();
	int stat;
    QString cmd;
    QStringList opt;
    char file[1024], kmlfile[1024], *p;
    double offset[3] = { 0 }, tint = 0.0;
    gtime_t ts = { 0, 0 }, te = { 0, 0 };

	ShowMsg("");

    if (InputFile->text() == "" || OutputFile->text() == "") return;

    ShowMsg(tr("converting ..."));

    if (TimeSpan->isChecked()) {
        QDateTime start(dateTime1->dateTime());
        QDateTime end(dateTime2->dateTime());
        ts.time = start.toSecsSinceEpoch(); ts.sec = start.time().msec() / 1000;
        te.time = end.toSecsSinceEpoch(); te.sec = end.time().msec() / 1000;
	}

    if (AddOffset->isChecked()) {
        offset[0] = Offset1->value();
        offset[1] = Offset2->value();
        offset[2] = Offset3->value();
	}

    if (TimeIntF->isChecked()) tint = TimeInt->value();

    strcpy(file, qPrintable(InputFile_Text));

    if (FormatKML->isChecked()) {
        if (Compress->isChecked()) {
            strcpy(kmlfile, file);
            if (!(p = strrchr(kmlfile, '.'))) p = kmlfile + strlen(kmlfile);
            strcpy(p, ".kml");
        }
        stat = convkml(file, Compress->isChecked() ? kmlfile : qPrintable(OutputFile_Text),
                   ts, te, tint, QFlags->currentIndex(), offset,
                   TrackColor->currentIndex(), PointColor->currentIndex(),
                   OutputAlt->currentIndex(), OutputTime->currentIndex());
    } else {
        stat = convgpx(file, Compress->isChecked() ? kmlfile : qPrintable(OutputFile_Text),
                   ts, te, tint, QFlags->currentIndex(), offset,
                   TrackColor->currentIndex(), PointColor->currentIndex(),
                   OutputAlt->currentIndex(), OutputTime->currentIndex());
    }

    if (stat < 0) {
        if (stat == -1) ShowMsg(tr("error : read input file"));
        else if (stat == -2) ShowMsg(tr("error : input file format"));
        else if (stat == -3) ShowMsg(tr("error : no data in input file"));
        else ShowMsg(tr("error : write kml file"));
		return;
	}
    if (FormatKML->isChecked() && Compress->isChecked()) {
#ifdef Q_OS_WIN
        cmd = "zip.exe";
        opt << "-j"
        opt << QString("-m %1").arg(OutputFile->text()); //TODO: zip for other platforms
#endif
#ifdef Q_OS_LINUX
        cmd = "gzip";
        opt << QString("-3 %1 %2").arg(OutputFile->text());
#endif
        opt << kmlfile;
        if (!ExecCmd(cmd, opt)) {
            ShowMsg(tr("error : zip execution"));
			return;
		}
	}
	ShowMsg("done");
}
//---------------------------------------------------------------------------
void ConvDialog::BtnCloseClick()
{
    accept();
}
//---------------------------------------------------------------------------
void ConvDialog::CompressClick()
{
	UpdateOutFile();
}
//---------------------------------------------------------------------------
void ConvDialog::UpdateEnable(void)
{
    Offset1->setEnabled(AddOffset->isChecked());
    Offset2->setEnabled(AddOffset->isChecked());
    Offset3->setEnabled(AddOffset->isChecked());
    dateTime1->setEnabled(TimeSpan->isChecked());
    dateTime2->setEnabled(TimeSpan->isChecked());
    TimeInt->setEnabled(TimeIntF->isChecked());

    Compress->setVisible(FormatKML->isChecked());
    GoogleEarthFile->setEnabled(FormatKML->isChecked());
    BtnGoogleEarthFile->setEnabled(FormatKML->isChecked());

    Compress->setEnabled(false);
#ifdef Q_OS_WIN
    Compress->setEnabled(true);
#endif
#ifdef Q_OS_LINUX
    Compress->setEnabled(true);
#endif
}
//---------------------------------------------------------------------------
int ConvDialog::ExecCmd(const QString &cmd, const QStringList &opt)
{
    return QProcess::startDetached(cmd, opt);
}
//---------------------------------------------------------------------------
void ConvDialog::ShowMsg(const QString &msg)
{
    Message->setText(msg);
    if (msg.contains("error")) Message->setStyleSheet("QLabel {color : red}");
    else Message->setStyleSheet("QLabel {color : blue}");
}
//---------------------------------------------------------------------------
void ConvDialog::InputFileChange()
{
	UpdateOutFile();
}
//---------------------------------------------------------------------------
void ConvDialog::UpdateOutFile(void)
{
    QString InputFile_Text = InputFile->text();

    if (InputFile_Text == "") return;
    QFileInfo fi(InputFile_Text);
    if (fi.suffix().isNull()) return;
    InputFile_Text = QDir::toNativeSeparators(fi.absolutePath() + "/" + fi.baseName());
    InputFile_Text += FormatGPX->isChecked() ? ".gpx" : (Compress->isChecked() ? ".kmz" : ".kml");
    OutputFile->setText(InputFile_Text);
}
//---------------------------------------------------------------------------
void ConvDialog::GoogleEarthFileChange()
{
    mainForm->GoogleEarthFile = GoogleEarthFile->text();
}
//---------------------------------------------------------------------------
void ConvDialog::BtnGoogleEarthFileClick()
{
    GoogleEarthFile->setText(QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Google Earth Exe File"))));
}
//---------------------------------------------------------------------------
void ConvDialog::FormatKMLClick()
{
    UpdateOutFile();
    UpdateEnable();
}
//---------------------------------------------------------------------------
void ConvDialog::FormatGPXClick()
{
    UpdateOutFile();
    UpdateEnable();
}
//---------------------------------------------------------------------------
void ConvDialog::BtnViewClick()
{
    QString file = OutputFile->text();

    if (file == "") return;
    viewer->setWindowTitle(file);
    viewer->show();
    viewer->Read(file);
}
//---------------------------------------------------------------------------
