#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->progressBar->hide();
    isiKotakFile = ui->tempatFile->text();

    ruangKerja = QDir::homePath()+"/.alldeb"; //direktori untuk pengaturan dan penyimpanan temporer alldeb
    programTar = "tar"; //perintah tar untuk mengekstrak dan melihat properti file

    connect(ui->tempatFile,SIGNAL(textChanged(QString)),this,SLOT(memilihFile()));
    ekstrak = new QProcess(this);
    connect(ekstrak,SIGNAL(finished(int)),this,SLOT(bacaInfoFile()));
    daftarFile = new QProcess(this);
    connect(daftarFile,SIGNAL(finished(int)),this,SLOT(buatDaftarIsi()));
    buatPaketInfo = new QProcess(this);
    connect(buatPaketInfo,SIGNAL(finished(int)),this,SLOT(bacaBikinInfo()));
    apt_get1 = new QProcess(this);
    connect(apt_get1,SIGNAL(readyRead()),this,SLOT(bacaHasilAptget()));
    connect(apt_get1,SIGNAL(finished(int)),this,SLOT(instalPaket()));
    apt_get2 = new QProcess(this);
    connect(apt_get2,SIGNAL(readyRead()),this,SLOT(bacaHasilPerintah()));

    QFile polkit("/usr/bin/pkexec"); //perintah front-end untuk meminta hak administratif dengan PolicyKit
    QFile kdesudo("/usr/bin/kdesudo"); //front-end sudo di KDE
    if(polkit.exists())
    {
        sandiGui = "pkexec";
    }
    else if(kdesudo.exists())
    {
        sandiGui = "kdesudo";
    }
    else
    {
        sandiGui = "gksudo";
    }
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_btnCariFile_clicked()
{
    if(isiKotakFile.isEmpty()) {
        namaFile = QFileDialog::getOpenFileName(this,tr("Pilih file alldeb"),QDir::homePath(),tr("File Paket (*.alldeb *.gz)"));

    }
    else
    {
        QFile fael(isiKotakFile);
        QFileInfo info(fael);
        namaFile = QFileDialog::getOpenFileName(this,tr("Pilih file alldeb"),info.absolutePath(),tr("File Paket (*.alldeb *.gz)"));

    }

    profil.setFile(namaFile);
    namaProfil = profil.completeBaseName();

    QFile filePaket(namaFile);
    QCryptographicHash crypto(QCryptographicHash::Md5);
    filePaket.open(QFile::ReadOnly);
    while(!filePaket.atEnd()){
      crypto.addData(filePaket.readAll());
    }

    QByteArray sum = crypto.result();
    QString sums(sum.toHex());
    ui->labelMd5Nilai->setText("<html><head/><body><p><span style=\" font-weight:600;\">"+sums+"</span></p></body></html>");

    qint64 ukuran = 0;
    ukuran = filePaket.size();
    QString nilai = size_human(ukuran);
    ui->labelUkuranNilai->setText("<html><head/><body><p><span style=\" font-weight:600;\">"+nilai+"</span></p></body></html>");

    if(!QDir(ruangKerja).exists()){
        QDir().mkdir(ruangKerja);
    }

    if(!namaFile.isNull()){
        ui->tempatFile->setText(namaFile);
    }

    //ruangKerja.append();
    if(!QDir(ruangKerja+"/"+namaProfil).exists()){
        QDir().mkdir(ruangKerja+"/"+namaProfil);
    }
    if(!QDir(ruangKerja+"/config").exists()){
        QDir().mkdir(ruangKerja+"/config");
    }

    if(QFile(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt").exists())
    {
        bacaInfoFile();
    }
    else
    {

        QStringList argumen;
        argumen << "-xzf" << namaFile << "--directory="+ruangKerja+"/"+namaProfil << "keterangan_alldeb.txt";
        ekstrak->start(programTar, argumen);
        bacaInfoFile();
    }

    QStringList variabel;
    variabel << "-tf" << namaFile;
    daftarFile->start(programTar, variabel);
    daftarFile->setReadChannel(QProcess::StandardOutput);

    filePaket.close();
}

QString Dialog::size_human(qint64 jumlah)
{
    float num = jumlah;
    QStringList list;
    list << "KB" << "MB" << "GB" << "TB";

    QStringListIterator i(list);
    QString unit("bytes");

    while(num >= 1024.0 && i.hasNext())
    {
        unit = i.next();
        num /= 1024.0;
    }
    return QString().setNum(num,'f',2)+" "+unit;
}

QString Dialog::bacaTeks(QFile namaBerkas)
{
    QString isiBerkas;
    return isiBerkas;
}

void Dialog::bacaInfoFile()
{
//    profil = ui->tempatFile->text();
//    namaProfil = "/"+profil.completeBaseName();
    QFile infoFile(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt");
    if (infoFile.exists() && infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&infoFile);
        QString line;
        while (!stream.atEnd()){
            line = stream.readAll();
            ui->infoPaket->setPlainText(line);
            //qDebug() << "linea: "<<line;
        }

    }
    else
    {
        ui->infoPaket->appendPlainText(tr("Ada kesalahan"));
    }

    infoFile.close();
}

void Dialog::buatDaftarIsi()
{
    if(ui->daftarPaket->count() != 0)
    {
        ui->daftarPaket->clear();
    }

    QStringList daftarIsi;
    QString daftar(daftarFile->readAllStandardOutput());
    daftarIsi = daftar.split("\n");
    if(daftarIsi.contains("keterangan_alldeb.txt"))
    {
    daftarIsi.removeOne("keterangan_alldeb.txt");
    ui->daftarPaket->insertItems(0,daftarIsi);
    }
    else if(isiKotakFile == 0)
    {
        //ui->infoPaket->clear();
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Kesalahan"));
        msgBox.setText(tr("Waduh, ini bukan file alldeb. Jangan dilanjutkan ya!"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }

    int jml = ui->daftarPaket->count();
    ui->daftarPaket->takeItem(jml-1);
    ui->labelJumlahNilai->setText("<html><head/><body><p><span style=\" font-weight:600;\">"+QString::number(jml)+
                                  "</span></p></body></html>");

}

void Dialog::on_btnFolderApt_clicked()
{
    QString folderApt = QFileDialog::getExistingDirectory(this, tr("Buka folder"),
                                                           ui->tempatApt->text(),
                                                           QFileDialog::ShowDirsOnly
                                                           | QFileDialog::DontResolveSymlinks);
    ui->tempatApt->setText(folderApt);
}

void Dialog::bacaBikinInfo()
{
    QString output(buatPaketInfo->readAllStandardOutput());
    ui->infoPaket->appendPlainText(output);
    //qDebug() << output;
    //ruangKerja = QDir::homePath()+"/.alldeb";

    QStringList arg1;
//    arg1 << "-c" << "sudo -u "+userN+" apt-get -o dir::etc::sourcelist="+folderKerja1+
//            "/source_sementara.list -o dir::etc::sourceparts="+folderKerja1+
//            "/part.d -o dir::state::lists="+folderKerja1+"/lists update";
    arg1 << "-u" << "root" << "apt-get" << "-o" << "dir::etc::sourcelist="+ruangKerja+"/config/source_sementara.list"
         << "-o" << "dir::etc::sourceparts="+ruangKerja+"/part.d"
         << "-o" << "dir::state::lists="+ruangKerja+"/lists" << "update";
    apt_get1->setWorkingDirectory(ruangKerja);
    apt_get1->setProcessChannelMode(QProcess::MergedChannels);
    apt_get1->start(sandiGui,arg1,QIODevice::ReadWrite);
}

void Dialog::bacaHasilAptget()
{
    QString output(apt_get1->readAll());
    ui->infoPaket->appendPlainText(output);
    //qDebug() << output;
}

void Dialog::bacaHasilPerintah()
{
    QString output(apt_get2->readAll());
    ui->infoPaket->appendPlainText(output);
}

void Dialog::on_btnInstal_clicked()
{
    //ruangKerja = QDir::homePath()+"/.alldeb";
//    profil = ui->tempatFile->text();
//    namaProfil = profil.completeBaseName();

    if(!namaFile.isEmpty())
    {
        ui->infoPaket->appendPlainText("-----------------------\n");
        QProcess *ekstraksi = new QProcess(this);
        QStringList argumen3;
        argumen3 << "-xzf" << namaFile << "--directory="+ruangKerja+"/"+namaProfil
                 << "--skip-old-files" << "--keep-newer-files";
        ekstraksi->start(programTar, argumen3);

        QString filename=ruangKerja+"/config/source_sementara.list";
        QFile source( filename );
        if ( source.open(QIODevice::WriteOnly) )
        {
            QTextStream stream( &source );
            stream << "deb file:"+ruangKerja+"/"+namaProfil+" ./" << endl;
        }
    }
    //cek apt-ftparchive dan dpkg-scanpackages
    QFile ftpArchive("/usr/bin/apt-ftparchive");
    QFile scanPackages("/usr/bin/dpkg-scanpackages");
    if(ftpArchive.exists())
    {
        //apt-ftparchive packages . 2>/dev/null | gzip > ./Packages.gz
        QStringList arg4;
        arg4 << "-c" << "apt-ftparchive packages "+namaProfil+"/ 2>/dev/null | gzip > "+namaProfil+"/Packages.gz";

        buatPaketInfo->setWorkingDirectory(ruangKerja);
        buatPaketInfo->start("sh",arg4);
        //qDebug() << argumen5;


        if(!QDir(ruangKerja+"/part.d").exists()){
            QDir().mkdir(ruangKerja+"/part.d");
        }
        if(!QDir(ruangKerja+"/lists").exists()){
            QDir().mkdir(ruangKerja+"/lists");
        }
        if(!QDir(ruangKerja+"/lists/partial").exists()){
            QDir().mkdir(ruangKerja+"/lists/partial");
        }


    }
    else if(scanPackages.exists())
    {
        //BELUM ADA PROSES
    }
    else
    {
        QMessageBox::warning(this,tr("Tidak bisa memeriksa"),
                             tr("Tanpa program pemindai paket, proses ini tidak bisa dilanjutkan.\n"
                                "Program yang dimaksud adalah apt-ftparchive (dari paket apt-utils) atau dpkg-scan-packages"));
    }

}

void Dialog::on_btnSalin_clicked()
{
    //masih percobaan juga
//    QString pencarian = ui->infoPaket->toPlainText();
//    int pos = pencarian.indexOf("\"");
//    QStringRef cari(&pencarian,pos+1,pencarian.lastIndexOf("\"")-pos-1);
//    qDebug() << cari.toString();
}

void Dialog::on_btnSalinIns_clicked()
{
    //Masih percobaan. Nanti harus diubah.

//    QStringList arg2;
//    arg2 << "--user" << "root" << "apt-get" << "install" << "texmaker";
//    apt_get2->setProcessChannelMode(QProcess::MergedChannels);
//    apt_get2->start(sandiGui,arg2,QIODevice::ReadWrite);
}

void Dialog::instalPaket()
{
//    profil = ui->tempatFile->text();
//    namaProfil = profil.completeBaseName();
    QFile infoFile1(ruangKerja+"/"+namaProfil+"/keterangan_alldeb.txt");
    int pos1 = 0;
    //QStringRef cari1;
    if (infoFile1.exists() && infoFile1.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&infoFile1);
        QString line1;
        while (!stream.atEnd()){
            line1 = stream.readAll();

        }
        pos1 = line1.indexOf("\"");
        QStringRef cari1(&line1,pos1+1,line1.lastIndexOf("\"")-pos1-1);
        QString paketTermuat = cari1.toString();
        //ruangKerja = QDir::homePath()+"/.alldeb";

//        Menemukan nama pengguna ubuntu;
//        QStringList user(QString(QDir::homePath()).split("/"));
//        QString userN(user.at(2));

        QStringList arg2,arg5;
        if(paketTermuat.contains(" "))
        {
            arg5 << paketTermuat.split(" ");
        }
        else
        {
            arg5 << paketTermuat;
        }
        arg2 << "--user" << "root" << "apt-get" << "-o" << "dir::etc::sourcelist="+ruangKerja+
                "/config/source_sementara.list" << "-o" << "dir::etc::sourceparts="+ruangKerja+
                "/part.d" << "-o" << "dir::state::lists="+ruangKerja+"/lists" << "install" <<
                "--allow-unauthenticated" << "-y" << "-s";
        arg2.append(arg5);
        apt_get2->setWorkingDirectory(ruangKerja);
        apt_get2->setProcessChannelMode(QProcess::MergedChannels);
        apt_get2->start(sandiGui,arg2,QIODevice::ReadWrite);
        infoFile1.close();
        //qDebug() << arg2;
    }
    else
    {
        ui->infoPaket->appendPlainText(tr("Ada kesalahan"));
    }    
}

void Dialog::on_btnInfo_clicked()
{
    tentangProgram = new About(this);
    tentangProgram->show();
}

void Dialog::memilihFile()
{
    //mengumpulkan perintah jika tombol pilih file diklik dan ui->tempatFile berubah isinya
    ui->btnInstal->setDisabled(false);
    ui->btnSalin->setDisabled(false);
    ui->btnSalinIns->setDisabled(false);
}
